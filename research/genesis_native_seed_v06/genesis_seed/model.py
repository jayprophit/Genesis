from __future__ import annotations
from dataclasses import asdict, dataclass, field
import torch
from torch import nn
import torch.nn.functional as F

# Byte-level text encoding: no borrowed tokenizer or vocabulary files.
BOS, EOS, IMG, PAD = 256, 257, 258, 259
VOCAB_SIZE = 260


def encode(text: str) -> list[int]:
    return list(text.encode('utf-8'))


def decode(ids: list[int]) -> str:
    return bytes(x for x in ids if 0 <= x <= 255).decode('utf-8', errors='replace')

# Special control tokens (v0.3): instruction/response markup.
INSTRUCTION, RESPONSE = 260, 261
VOCAB_SIZE = 262


@dataclass(frozen=True)
class SeedConfig:
    width: int = 64
    heads: int = 4
    layers: int = 2
    context: int = 128
    image_size: int = 16
    patch_size: int = 4
    dropout: float = 0.0


@dataclass(frozen=True)
class GenesisConfig:
    """Versioned, serializable configuration for Genesis v0.3 and later.

    Uses 'genesis.config.v0.3' format for new checkpoints and falls back to the
    legacy 'genesis.config.v0.2' / raw SeedConfig format for compatibility.
    """
    version: str = 'genesis.config.v0.3'
    model: str = 'GenesisSeed'
    seed_config: SeedConfig = field(default_factory=SeedConfig)

    def state_config(self) -> dict:
        if self.version == 'genesis.config.v0.3':
            return {'version': self.version, 'model': self.model, 'seed_config': asdict(self.seed_config)}
        # v0.2 / legacy format: plain SeedConfig fields (backward compatible).
        return asdict(self.seed_config)

    @staticmethod
    def from_dict(d: dict) -> 'GenesisConfig':
        if d.get('version') == 'genesis.config.v0.3':
            # seed_config may have been serialized as a plain dict by state_config();
            # reconstruct it as a SeedConfig object.
            raw = d.get('seed_config')
            seed_config = SeedConfig(**raw) if isinstance(raw, dict) else raw
            return GenesisConfig(version=d['version'], model=d['model'],
                                 seed_config=seed_config)
        # Legacy v0.2 checkpoint: no version key; treat as v0.2.
        return GenesisConfig(version='genesis.config.v0.2', model='GenesisSeed',
                             seed_config=SeedConfig(**d))


class GenesisSeed(nn.Module):
    """Causal byte-token generator with optional visual-prefix input.

    Visual signal enters as original RGB patches projected into the SAME transformer
    workspace as language. All parameters start random and are trainable; no weights
    or tokenizer are downloaded. Small prototype, not a general language/vision model.
    """
    def __init__(self, config=None):
        if config is None:
            config = GenesisConfig()
        elif isinstance(config, dict):
            config = GenesisConfig.from_dict(config)
        elif not isinstance(config, GenesisConfig):
            config = GenesisConfig(seed_config=config)
        super().__init__()
        cfg = config.seed_config if isinstance(config, GenesisConfig) else config
        if cfg.width % cfg.heads:
            raise ValueError('width must be divisible by heads')
        if cfg.image_size % cfg.patch_size:
            raise ValueError('image_size must be divisible by patch_size')
        self.config = config
        self.seed_config = cfg
        self.token_embedding = nn.Embedding(VOCAB_SIZE, cfg.width)
        self.patch_embedding = nn.Conv2d(3, cfg.width,
                                         kernel_size=cfg.patch_size,
                                         stride=cfg.patch_size)
        self.position = nn.Embedding(cfg.context, cfg.width)
        block = nn.TransformerEncoderLayer(d_model=cfg.width, nhead=cfg.heads,
                      dim_feedforward=4*cfg.width, dropout=cfg.dropout,
                      batch_first=True, activation='gelu', norm_first=True)
        self.blocks = nn.TransformerEncoder(block, num_layers=cfg.layers,
                                           enable_nested_tensor=False)
        self.norm = nn.LayerNorm(cfg.width)
        self.output = nn.Linear(cfg.width, VOCAB_SIZE)
        self.apply(self._initialize)

    @staticmethod
    def _initialize(m: nn.Module) -> None:
        if isinstance(m, (nn.Linear, nn.Embedding, nn.Conv2d)):
            nn.init.normal_(m.weight, mean=0, std=0.02)
            if getattr(m, 'bias', None) is not None:
                nn.init.zeros_(m.bias)

    def forward(self, tokens: torch.Tensor, image: torch.Tensor | None = None) -> torch.Tensor:
        if tokens.ndim != 2 or tokens.dtype != torch.long:
            raise ValueError('tokens must be int64 [batch, sequence]')
        cfg = self.seed_config
        pieces = []
        prefix = 0
        if image is not None:
            n = cfg.image_size
            if image.shape != (tokens.shape[0], 3, n, n):
                raise ValueError(f'image must have shape [batch, 3, {n}, {n}]')
            # The IMG control token acts as a learned modality boundary.
            marker = torch.full((tokens.shape[0],1), IMG,
                                device=tokens.device, dtype=torch.long)
            pieces.append(self.token_embedding(marker))
            patches = self.patch_embedding(image).flatten(2).transpose(1, 2)
            pieces.append(patches)
            prefix = 1 + patches.shape[1]
        pieces.append(self.token_embedding(tokens))
        stream = torch.cat(pieces, dim=1)
        size = stream.shape[1]
        if size > cfg.context:
            raise ValueError(f'context overflow: {size} > {cfg.context}')
        pos = torch.arange(size, device=tokens.device)
        stream = stream + self.position(pos)[None]
        mask = torch.triu(torch.ones(size, size, dtype=torch.bool, device=tokens.device), diagonal=1)
        hidden = self.norm(self.blocks(stream, mask=mask))
        return self.output(hidden[:, prefix:, :])

    @torch.no_grad()
    def generate(self, image: torch.Tensor | None = None, prompt: str = '',
                 max_new_tokens: int = 48, temperature: float = 0) -> str:
        self.eval()
        device = next(self.parameters()).device
        if image is not None:
            image = image.to(device)
        cfg = self.seed_config
        prefix = [BOS] + encode(prompt)
        tokens = torch.tensor([prefix], dtype=torch.long, device=device)
        generated = []
        max_count = max(0, cfg.context - tokens.shape[1] -
                        (1 + (cfg.image_size//cfg.patch_size)**2 if image is not None else 0))
        for _ in range(min(max_new_tokens, max_count)):
            logits = self(tokens, image=image)[0,-1].clone()
            logits[[BOS, IMG, PAD]] = -float('inf')
            if temperature <= 0:
                next_id = logits.argmax().item()
            else:
                probs = F.softmax(logits / temperature, dim=-1)
                next_id = torch.multinomial(probs, 1).item()
            if next_id == EOS:
                break
            generated.append(next_id)
            tokens = torch.cat([tokens, torch.tensor([[next_id]], device=device)], dim=1)
        return decode(generated)

    def state_config(self) -> dict:
        # Return the serialized config so checkpoints carry their format version.
        sc = self.config
        return sc.state_config() if hasattr(sc, 'state_config') else asdict(sc)
