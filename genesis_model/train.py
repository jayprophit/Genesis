"""Training loop for Genesis.

Three objectives share one backbone:
  1. next-token prediction on packed text;
  2. grounded captioning on image/text pairs (vision tower trained jointly);
  3. chat/tool-use SFT with prompt tokens masked.

This trainer is intentionally small and auditable. It runs on CPU for
correctness tests and on one consumer GPU for the seed models. Large runs use
the same step semantics with sharded data and distributed launch.
"""
from __future__ import annotations

import json
import math
import random
import time
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Sequence

import torch
import torch.nn.functional as F
from torch.utils.data import DataLoader

from .checkpoint import CheckpointMeta, corpus_digest, save_checkpoint
from .config import ModelConfig, VisionConfig
from .data import (
    ChatSFTDataset,
    MultimodalCaptionDataset,
    PackedTextDataset,
    SyntheticShapes,
    collate_multimodal,
    collate_text,
    read_jsonl,
)
from .model import GenesisModel
from .tokenizer import Tokenizer

DEFAULT_CORPUS = """Genesis is a digital organism built as one continuous identity.
It learns language, images, tools and memory from its own experience.
A language model predicts the next token; an agent decides what to do next.
A tool call is a proposal, never an authorisation and never an execution.
The runtime records the request, the approval, the result and the evidence.
Vision begins with pixels, patches, and a trainable projection into the same workspace.
Speech begins with text, phonemes, prosody and a waveform decoder trained from scratch.
Memory is not a prompt. It is a record with provenance, time and a retrieval policy.
Learning is not a claim. It is a measured change in weights or behaviour.
Safety is not a refusal string. It is a gate that can name why it refused.
"""


@dataclass
class TrainConfig:
    steps: int = 200
    batch_size: int = 4
    seq_len: int = 128
    lr: float = 3e-3
    min_lr_ratio: float = 0.1
    weight_decay: float = 0.1
    warmup_steps: int = 20
    grad_accum: int = 1
    clip_grad: float = 1.0
    log_every: int = 10
    save_every: int = 0
    seed: int = 42
    device: str = "auto"
    vision_weight: float = 0.5
    sft_weight: float = 0.5
    out_dir: str = "runs/tiny"
    checkpoint_name: str = "genesis.pt"
    model: ModelConfig = field(default_factory=lambda: ModelConfig())
    notes: str = ""


def _resolve_device(requested: str) -> torch.device:
    if requested != "auto":
        return torch.device(requested)
    if torch.cuda.is_available():
        return torch.device("cuda")
    if getattr(torch.backends, "mps", None) and torch.backends.mps.is_available():
        return torch.device("mps")
    return torch.device("cpu")


def _lr_at(step: int, config: TrainConfig) -> float:
    if step < config.warmup_steps:
        return config.lr * (step + 1) / max(1, config.warmup_steps)
    progress = (step - config.warmup_steps) / max(1, config.steps - config.warmup_steps)
    cosine = 0.5 * (1.0 + math.cos(math.pi * min(1.0, progress)))
    return config.lr * (config.min_lr_ratio + (1 - config.min_lr_ratio) * cosine)


def _build_tokenizer(texts: list[str], target_vocab: int) -> Tokenizer:
    tokenizer = Tokenizer.train(texts, vocab_size=target_vocab)
    # A vocabulary smaller than requested is normal on a small corpus. The
    # model is sized to the tokenizer that actually exists.
    return tokenizer


def train(
    config: TrainConfig,
    *,
    corpus_paths: Sequence[str | Path] | None = None,
    sft_paths: Sequence[str | Path] | None = None,
    resume_from: str | Path | None = None,
) -> dict:
    random.seed(config.seed)
    torch.manual_seed(config.seed)
    device = _resolve_device(config.device)
    out_dir = Path(config.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # -- corpus -------------------------------------------------------------
    texts: list[str] = [DEFAULT_CORPUS]
    resolved_paths = [Path(p) for p in (corpus_paths or [])]
    for path in resolved_paths:
        texts.append(path.read_text(encoding="utf-8", errors="replace"))
    digest = corpus_digest(resolved_paths) if resolved_paths else "builtin"

    # Captions are part of the tokenizer corpus so image words are not byte-fragments.
    shape_words = ["a red circle", "a green square", "a blue triangle", "a yellow circle"]
    texts.extend(shape_words)

    if resume_from:
        from .checkpoint import load_checkpoint

        model, tokenizer, _ = load_checkpoint(resume_from, map_location=device)
        model.to(device)
        model.train()
    else:
        tokenizer = _build_tokenizer(texts, config.model.vocab_size)
        model_config = ModelConfig.from_dict(
            {**config.model.to_dict(), "vocab_size": tokenizer.vocab_size}
        )
        model = GenesisModel(model_config).to(device)
        model.train()

    if model.config.vision is None:
        vision_weight = 0.0
    else:
        vision_weight = config.vision_weight

    # -- datasets -----------------------------------------------------------
    all_token_ids: list[int] = []
    for text in texts:
        all_token_ids.extend(tokenizer.encode(text))
        all_token_ids.append(tokenizer.id_of("<|eos|>"))
    text_dataset = PackedTextDataset(all_token_ids, config.seq_len + 1)
    if len(text_dataset) == 0:
        all_token_ids = all_token_ids * 4
        text_dataset = PackedTextDataset(all_token_ids, config.seq_len + 1)

    vision_dataset = None
    if model.config.vision is not None:
        vision_dataset = MultimodalCaptionDataset(
            SyntheticShapes(size=model.config.vision.image_size, length=2048, seed=config.seed),
            tokenizer,
        )

    sft_dataset = None
    sft_records: list[dict] = []
    for path in sft_paths or []:
        sft_records.extend(read_jsonl(path))
    if not sft_records:
        sft_records = demo_sft_records()
    sft_dataset = ChatSFTDataset(sft_records, tokenizer, config.seq_len)

    text_loader = DataLoader(
        text_dataset, batch_size=config.batch_size, shuffle=True, drop_last=True,
        collate_fn=collate_text,
    )
    vision_loader = (
        DataLoader(
            vision_dataset, batch_size=config.batch_size, shuffle=True, drop_last=True,
            collate_fn=collate_multimodal,
        )
        if vision_dataset is not None else None
    )
    sft_loader = DataLoader(
        sft_dataset, batch_size=min(config.batch_size, max(1, len(sft_dataset))),
        shuffle=True, drop_last=False, collate_fn=collate_text,
    )

    optimizer = torch.optim.AdamW(
        model.parameters(), lr=config.lr, betas=(0.9, 0.95),
        weight_decay=config.weight_decay, fused=False,
    )
    amp_enabled = device.type == "cuda"
    scaler = torch.amp.GradScaler("cuda", enabled=amp_enabled)

    history: list[dict] = []
    tokens_seen = 0
    started = time.time()
    text_iter = _cycle(text_loader)
    vision_iter = _cycle(vision_loader) if vision_loader is not None else None
    sft_iter = _cycle(sft_loader)

    model.train()
    for step in range(config.steps):
        lr = _lr_at(step, config)
        for group in optimizer.param_groups:
            group["lr"] = lr

        optimizer.zero_grad(set_to_none=True)
        totals = {"text": 0.0, "vision": 0.0, "sft": 0.0, "total": 0.0}

        for _ in range(config.grad_accum):
            x, y = next(text_iter)
            x, y = x.to(device), y.to(device)
            with torch.autocast(device_type=device.type, dtype=torch.bfloat16, enabled=amp_enabled):
                _, text_loss = model(x, targets=y)
                loss = text_loss
                vision_loss = torch.zeros((), device=device)
                if vision_iter is not None and config.vision_weight > 0:
                    vx, vy, images = next(vision_iter)
                    vx, vy, images = vx.to(device), vy.to(device), images.to(device)
                    _, vision_loss = model(vx, images=images, targets=vy)
                    loss = loss + config.vision_weight * vision_loss
                sft_loss = torch.zeros((), device=device)
                if sft_dataset is not None and config.sft_weight > 0:
                    sx, sy = next(sft_iter)
                    sx, sy = sx.to(device), sy.to(device)
                    _, sft_loss = model(sx, targets=sy)
                    loss = loss + config.sft_weight * sft_loss
                loss = loss / config.grad_accum

            scaler.scale(loss).backward()
            totals["text"] += float(text_loss.detach())
            totals["vision"] += float(vision_loss.detach())
            totals["sft"] += float(sft_loss.detach())
            totals["total"] += float(loss.detach()) * config.grad_accum
            tokens_seen += x.numel()

        scaler.unscale_(optimizer)
        torch.nn.utils.clip_grad_norm_(model.parameters(), config.clip_grad)
        scaler.step(optimizer)
        scaler.update()

        if step == 0 or (step + 1) % config.log_every == 0 or step == config.steps - 1:
            record = {
                "step": step + 1,
                "lr": round(lr, 8),
                "text_ce": round(totals["text"] / config.grad_accum, 4),
                "vision_ce": round(totals["vision"] / config.grad_accum, 4),
                "sft_ce": round(totals["sft"] / config.grad_accum, 4),
                "total": round(totals["total"] / config.grad_accum, 4),
                "tokens": tokens_seen,
            }
            history.append(record)
            print(json.dumps(record), flush=True)

        if config.save_every and (step + 1) % config.save_every == 0:
            _save(model, tokenizer, config, out_dir, history, tokens_seen, digest,
                  name=f"step_{step + 1}.pt", optimizer=optimizer)

    checkpoint = _save(
        model, tokenizer, config, out_dir, history, tokens_seen, digest,
        name=config.checkpoint_name, optimizer=optimizer,
    )
    manifest = {
        "checkpoint": str(checkpoint),
        "parameters": model.num_parameters(),
        "parameters_non_embedding": model.num_parameters(non_embedding=True),
        "vocab_size": tokenizer.vocab_size,
        "tokens_seen": tokens_seen,
        "steps": config.steps,
        "device": str(device),
        "duration_s": round(time.time() - started, 2),
        "corpus_sha256": digest,
        "config": asdict(config),
        "history": history,
    }
    (out_dir / "training_manifest.json").write_text(
        json.dumps(manifest, indent=2), encoding="utf-8"
    )
    return manifest


def _save(model, tokenizer, config, out_dir, history, tokens_seen, digest, *, name, optimizer):
    meta = CheckpointMeta(
        steps=config.steps,
        tokens_seen=tokens_seen,
        corpus_sha256=digest,
        training_history=history,
        notes=config.notes or "Genesis seed training run",
    )
    return save_checkpoint(out_dir / name, model, tokenizer, meta, optimizer)


def _cycle(loader):
    while True:
        for batch in loader:
            yield batch


def demo_sft_records() -> list[dict]:
    """A tiny, fully authored tool-use curriculum.

    It teaches the wire format only. Real capability comes from a much larger,
    licensed and evaluated instruction corpus.
    """
    return [
        {
            "messages": [
                {"role": "system", "content": "You are Genesis."},
                {"role": "user", "content": "What is 17 * 24?"},
                {"role": "assistant", "content": '<|tool_call|>{"name":"calculator","arguments":{"expression":"17 * 24"}}<|/tool_call|>'},
                {"role": "tool", "content": '{"ok":true,"value":408}'},
                {"role": "assistant", "content": "17 * 24 is 408."},
            ],
            "tools": [{"name": "calculator", "description": "Evaluate arithmetic."}],
        },
        {
            "messages": [
                {"role": "system", "content": "You are Genesis."},
                {"role": "user", "content": "List the files in the project root."},
                {"role": "assistant", "content": '<|tool_call|>{"name":"list_dir","arguments":{"path":"."}}<|/tool_call|>'},
                {"role": "tool", "content": '{"ok":true,"entries":["README.md","genesis_model"]}'},
                {"role": "assistant", "content": "The root contains README.md and the genesis_model package."},
            ],
            "tools": [{"name": "list_dir", "description": "List a directory inside the workspace."}],
        },
    ]
