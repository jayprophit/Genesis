"""Genesis decoder: a modern dense transformer written from first principles.

Components: RMSNorm, rotary position embeddings, grouped-query attention,
SwiGLU feed-forward, pre-norm residual blocks, tied embeddings, and an optional
from-scratch vision encoder for early image/text fusion.

No pretrained weights, tokenizer, or architecture file is loaded. Initialisation
is random; every parameter is trained by this project.
"""
from __future__ import annotations

import math
from dataclasses import dataclass

import torch
from torch import nn
import torch.nn.functional as F

from .config import ModelConfig, VisionConfig


class RMSNorm(nn.Module):
    def __init__(self, dim: int, eps: float = 1e-6) -> None:
        super().__init__()
        self.weight = nn.Parameter(torch.ones(dim))
        self.eps = eps

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        dtype = x.dtype
        x = x.float()
        x = x * torch.rsqrt(x.pow(2).mean(-1, keepdim=True) + self.eps)
        return (x.to(dtype)) * self.weight


def build_rope(
    head_dim: int, max_seq_len: int, theta: float = 10000.0, device=None, dtype=torch.float32
) -> tuple[torch.Tensor, torch.Tensor]:
    if head_dim % 2:
        raise ValueError("head_dim must be even for rotary embeddings")
    inv_freq = 1.0 / (theta ** (torch.arange(0, head_dim, 2, device=device).float() / head_dim))
    positions = torch.arange(max_seq_len, device=device).float()
    freqs = torch.outer(positions, inv_freq)
    emb = torch.cat((freqs, freqs), dim=-1)
    return emb.cos().to(dtype), emb.sin().to(dtype)


def rotate_half(x: torch.Tensor) -> torch.Tensor:
    half = x.shape[-1] // 2
    return torch.cat((-x[..., half:], x[..., :half]), dim=-1)


def apply_rope(
    q: torch.Tensor, k: torch.Tensor, cos: torch.Tensor, sin: torch.Tensor
) -> tuple[torch.Tensor, torch.Tensor]:
    # q,k: [B, H, T, D]; cos,sin: [T, D]
    cos = cos[None, None, :, :]
    sin = sin[None, None, :, :]
    return q * cos + rotate_half(q) * sin, k * cos + rotate_half(k) * sin


class KVCache:
    """Per-layer key/value cache for incremental decoding."""

    def __init__(self, n_layers: int) -> None:
        self.keys: list[torch.Tensor | None] = [None] * n_layers
        self.values: list[torch.Tensor | None] = [None] * n_layers

    @property
    def length(self) -> int:
        first = self.keys[0]
        return 0 if first is None else int(first.shape[2])

    def append(self, layer: int, k: torch.Tensor, v: torch.Tensor) -> tuple[torch.Tensor, torch.Tensor]:
        if self.keys[layer] is None:
            self.keys[layer] = k
            self.values[layer] = v
        else:
            self.keys[layer] = torch.cat((self.keys[layer], k), dim=2)  # type: ignore[arg-type]
            self.values[layer] = torch.cat((self.values[layer], v), dim=2)  # type: ignore[arg-type]
        return self.keys[layer], self.values[layer]  # type: ignore[return-value]

    def reset(self) -> None:
        self.keys = [None] * len(self.keys)
        self.values = [None] * len(self.values)


class Attention(nn.Module):
    def __init__(self, config: ModelConfig) -> None:
        super().__init__()
        self.n_heads = config.n_heads
        self.n_kv_heads = config.n_kv_heads
        self.head_dim = config.head_dim
        self.dropout = config.dropout
        self.q_proj = nn.Linear(config.d_model, self.n_heads * self.head_dim, bias=False)
        self.k_proj = nn.Linear(config.d_model, self.n_kv_heads * self.head_dim, bias=False)
        self.v_proj = nn.Linear(config.d_model, self.n_kv_heads * self.head_dim, bias=False)
        self.o_proj = nn.Linear(self.n_heads * self.head_dim, config.d_model, bias=False)

    def forward(
        self,
        x: torch.Tensor,
        cos: torch.Tensor,
        sin: torch.Tensor,
        cache: KVCache | None = None,
        layer_index: int = 0,
    ) -> torch.Tensor:
        batch, seq_len, _ = x.shape
        q = self.q_proj(x).view(batch, seq_len, self.n_heads, self.head_dim).transpose(1, 2)
        k = self.k_proj(x).view(batch, seq_len, self.n_kv_heads, self.head_dim).transpose(1, 2)
        v = self.v_proj(x).view(batch, seq_len, self.n_kv_heads, self.head_dim).transpose(1, 2)

        q, k = apply_rope(q, k, cos, sin)

        if cache is not None:
            k, v = cache.append(layer_index, k, v)

        # Repeat KV heads for grouped-query attention.
        if self.n_kv_heads != self.n_heads:
            repeat = self.n_heads // self.n_kv_heads
            k = k.repeat_interleave(repeat, dim=1)
            v = v.repeat_interleave(repeat, dim=1)

        if seq_len == 1:
            context = v.shape[2]
            if context > 1:
                out = F.scaled_dot_product_attention(
                    q, k, v, dropout_p=self.dropout if self.training else 0.0,
                    is_causal=False,
                )
            else:
                out = F.scaled_dot_product_attention(
                    q, k, v, dropout_p=self.dropout if self.training else 0.0,
                    is_causal=False,
                )
        else:
            out = F.scaled_dot_product_attention(
                q, k, v, dropout_p=self.dropout if self.training else 0.0,
                is_causal=True,
            )
        out = out.transpose(1, 2).contiguous().view(batch, seq_len, -1)
        return self.o_proj(out)


class SwiGLU(nn.Module):
    def __init__(self, config: ModelConfig) -> None:
        super().__init__()
        self.gate = nn.Linear(config.d_model, config.d_ff, bias=False)
        self.up = nn.Linear(config.d_model, config.d_ff, bias=False)
        self.down = nn.Linear(config.d_ff, config.d_model, bias=False)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.down(F.silu(self.gate(x)) * self.up(x))


class Block(nn.Module):
    def __init__(self, config: ModelConfig) -> None:
        super().__init__()
        self.norm1 = RMSNorm(config.d_model, config.norm_eps)
        self.attn = Attention(config)
        self.norm2 = RMSNorm(config.d_model, config.norm_eps)
        self.mlp = SwiGLU(config)

    def forward(
        self, x: torch.Tensor, cos: torch.Tensor, sin: torch.Tensor,
        cache: KVCache | None = None, layer_index: int = 0,
    ) -> torch.Tensor:
        x = x + self.attn(self.norm1(x), cos, sin, cache, layer_index)
        x = x + self.mlp(self.norm2(x))
        return x


class VisionEncoder(nn.Module):
    """Small from-scratch ViT. Trained jointly with the language model."""

    def __init__(self, config: VisionConfig) -> None:
        super().__init__()
        self.config = config
        self.patch_embed = nn.Conv2d(
            3, config.width, kernel_size=config.patch_size, stride=config.patch_size
        )
        grid = config.image_size // config.patch_size
        self.n_patches = grid * grid
        self.position = nn.Parameter(torch.zeros(1, self.n_patches, config.width))
        block = nn.TransformerEncoderLayer(
            d_model=config.width, nhead=config.heads, dim_feedforward=4 * config.width,
            dropout=config.dropout, batch_first=True, activation="gelu", norm_first=True,
        )
        self.blocks = nn.TransformerEncoder(block, num_layers=config.layers, enable_nested_tensor=False)
        self.norm = nn.LayerNorm(config.width)

    def forward(self, images: torch.Tensor) -> torch.Tensor:
        if images.ndim != 4 or images.shape[1] != 3:
            raise ValueError("images must be [B, 3, H, W]")
        if images.shape[-1] != self.config.image_size or images.shape[-2] != self.config.image_size:
            images = F.interpolate(
                images, size=(self.config.image_size, self.config.image_size),
                mode="bilinear", align_corners=False,
            )
        patches = self.patch_embed(images).flatten(2).transpose(1, 2)
        patches = patches + self.position[:, : patches.shape[1]]
        return self.norm(self.blocks(patches))


class GenesisModel(nn.Module):
    def __init__(self, config: ModelConfig) -> None:
        super().__init__()
        self.config = config
        self.token_embedding = nn.Embedding(config.vocab_size, config.d_model)
        self.blocks = nn.ModuleList([Block(config) for _ in range(config.n_layers)])
        self.norm = RMSNorm(config.d_model, config.norm_eps)
        self.lm_head = nn.Linear(config.d_model, config.vocab_size, bias=False)
        if config.tie_embeddings:
            self.lm_head.weight = self.token_embedding.weight

        self.vision: VisionEncoder | None = None
        self.vision_proj: nn.Linear | None = None
        if config.vision is not None:
            self.vision = VisionEncoder(config.vision)
            self.vision_proj = nn.Linear(config.vision.width, config.d_model)

        cos, sin = build_rope(config.head_dim, config.max_seq_len, config.rope_theta)
        self.register_buffer("rope_cos", cos, persistent=False)
        self.register_buffer("rope_sin", sin, persistent=False)
        self.apply(self._init_weights)

    @staticmethod
    def _init_weights(module: nn.Module) -> None:
        if isinstance(module, (nn.Linear, nn.Embedding, nn.Conv2d)):
            nn.init.normal_(module.weight, mean=0.0, std=0.02)
            if getattr(module, "bias", None) is not None:
                nn.init.zeros_(module.bias)
        elif isinstance(module, nn.LayerNorm) and module.bias is not None:
            nn.init.zeros_(module.bias)
            nn.init.ones_(module.weight)

    def num_parameters(self, non_embedding: bool = False) -> int:
        total = sum(p.numel() for p in self.parameters())
        if non_embedding:
            total -= self.token_embedding.weight.numel()
        return total

    def _image_tokens(self, images: torch.Tensor) -> torch.Tensor:
        if self.vision is None or self.vision_proj is None:
            raise ValueError("checkpoint has no vision tower")
        return self.vision_proj(self.vision(images))

    def forward(
        self,
        tokens: torch.Tensor,
        images: torch.Tensor | None = None,
        targets: torch.Tensor | None = None,
        cache: KVCache | None = None,
        position_offset: int = 0,
    ) -> torch.Tensor | tuple[torch.Tensor, torch.Tensor]:
        if tokens.ndim != 2 or tokens.dtype not in (torch.long, torch.int64):
            raise ValueError("tokens must be an int64 [B, T] tensor")

        x = self.token_embedding(tokens)
        prefix_len = 0
        if images is not None:
            visual = self._image_tokens(images)
            prefix_len = visual.shape[1]
            x = torch.cat((visual, x), dim=1)

        total_len = x.shape[1]
        if position_offset + total_len > self.config.max_seq_len:
            raise ValueError(
                f"sequence {position_offset + total_len} exceeds max_seq_len {self.config.max_seq_len}"
            )
        cos = self.rope_cos[position_offset: position_offset + total_len]
        sin = self.rope_sin[position_offset: position_offset + total_len]

        for index, block in enumerate(self.blocks):
            x = block(x, cos, sin, cache, index)

        logits = self.lm_head(self.norm(x))
        if prefix_len:
            logits = logits[:, prefix_len:, :]
        if targets is None:
            return logits

        # Shift is handled by the caller: targets align with tokens.
        loss = F.cross_entropy(
            logits.reshape(-1, logits.shape[-1]), targets.reshape(-1), ignore_index=-100
        )
        return logits, loss

    @torch.inference_mode()
    def generate_ids(
        self,
        prompt_ids: list[int],
        *,
        max_new_tokens: int = 128,
        temperature: float = 0.8,
        top_k: int | None = 50,
        top_p: float = 0.95,
        eos_id: int = 2,
        images: torch.Tensor | None = None,
        forbidden_ids: list[int] | None = None,
    ) -> list[int]:
        device = next(self.parameters()).device
        cache = KVCache(self.config.n_layers)
        tokens = torch.tensor([prompt_ids], dtype=torch.long, device=device)
        out = tokens

        logits = self(tokens, images=images, cache=cache)
        # The cache length includes any visual prefix, so positions stay aligned.
        offset = cache.length
        generated: list[int] = []
        for _ in range(max_new_tokens):
            if offset + 1 > self.config.max_seq_len:
                break
            next_logits = logits[:, -1, :].clone()
            if forbidden_ids:
                next_logits[:, forbidden_ids] = -float("inf")
            if temperature <= 0:
                next_id = int(next_logits.argmax(dim=-1).item())
            else:
                next_logits = next_logits / temperature
                if top_k is not None:
                    k = min(top_k, next_logits.shape[-1])
                    cutoff = torch.topk(next_logits, k).values[:, -1].unsqueeze(-1)
                    next_logits = next_logits.masked_fill(next_logits < cutoff, -float("inf"))
                probs = F.softmax(next_logits, dim=-1)
                if top_p < 1.0:
                    sorted_probs, sorted_idx = torch.sort(probs, descending=True, dim=-1)
                    cumulative = torch.cumsum(sorted_probs, dim=-1)
                    keep = cumulative - sorted_probs < top_p
                    keep[..., 0] = True
                    filtered = torch.full_like(probs, -float("inf"))
                    filtered.scatter_(1, sorted_idx, torch.where(keep, sorted_probs, torch.full_like(sorted_probs, -float("inf"))))
                    probs = filtered
                next_id = int(torch.multinomial(probs, 1).item())

            generated.append(next_id)
            out = torch.cat((out, torch.tensor([[next_id]], dtype=torch.long, device=device)), dim=1)
            if next_id == eos_id:
                break
            logits = self(
                torch.tensor([[next_id]], dtype=torch.long, device=device),
                cache=cache, position_offset=offset,
            )
            offset += 1
        return generated
