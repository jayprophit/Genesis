"""Checkpoint format and safe loading.

A Genesis checkpoint is a single file containing:
  * format version
  * model config
  * tokenizer (merges + special-token layout)
  * state dict
  * training provenance (steps, tokens, corpus digest, code version)

Loading uses weights_only=True so a checkpoint cannot execute arbitrary Python.
"""
from __future__ import annotations

import hashlib
import json
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import torch

from .config import ModelConfig
from .model import GenesisModel
from .tokenizer import Tokenizer

FORMAT_VERSION = 1


@dataclass
class CheckpointMeta:
    name: str = "genesis"
    version: str = "0.1.0"
    created_at: float = field(default_factory=time.time)
    steps: int = 0
    tokens_seen: int = 0
    corpus_sha256: str = ""
    training_history: list[dict] = field(default_factory=list)
    notes: str = ""

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "version": self.version,
            "created_at": self.created_at,
            "steps": self.steps,
            "tokens_seen": self.tokens_seen,
            "corpus_sha256": self.corpus_sha256,
            "training_history": self.training_history,
            "notes": self.notes,
        }


def corpus_digest(paths: list[str | Path]) -> str:
    digest = hashlib.sha256()
    for path in sorted(Path(p) for p in paths):
        digest.update(path.name.encode("utf-8"))
        with path.open("rb") as handle:
            for chunk in iter(lambda: handle.read(1 << 20), b""):
                digest.update(chunk)
    return digest.hexdigest()


def save_checkpoint(
    path: str | Path,
    model: GenesisModel,
    tokenizer: Tokenizer,
    meta: CheckpointMeta,
    optimizer: torch.optim.Optimizer | None = None,
) -> Path:
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "format_version": FORMAT_VERSION,
        "config": model.config.to_dict(),
        "tokenizer": tokenizer.to_dict(),
        "state_dict": model.state_dict(),
        "meta": meta.to_dict(),
    }
    if optimizer is not None:
        payload["optimizer"] = optimizer.state_dict()
    torch.save(payload, path)
    return path


def load_checkpoint(path: str | Path, map_location: str | torch.device = "cpu"):
    payload = torch.load(path, map_location=map_location, weights_only=True)
    if payload.get("format_version") != FORMAT_VERSION:
        raise ValueError(f"unsupported checkpoint format {payload.get('format_version')}")
    config = ModelConfig.from_dict(payload["config"])
    tokenizer = Tokenizer.from_dict(payload["tokenizer"])
    model = GenesisModel(config)
    model.load_state_dict(payload["state_dict"], strict=True)
    model.eval()
    meta = CheckpointMeta(**payload.get("meta", {}))
    return model, tokenizer, meta


def inspect_checkpoint(path: str | Path) -> dict[str, Any]:
    payload = torch.load(path, map_location="cpu", weights_only=True)
    config = ModelConfig.from_dict(payload["config"])
    return {
        "format_version": payload.get("format_version"),
        "config": config.to_dict(),
        "parameters": sum(v.numel() for v in payload["state_dict"].values()),
        "meta": payload.get("meta", {}),
        "size_bytes": Path(path).stat().st_size,
    }


def write_manifest(path: str | Path, **fields: Any) -> None:
    Path(path).write_text(json.dumps(fields, indent=2, sort_keys=True), encoding="utf-8")
