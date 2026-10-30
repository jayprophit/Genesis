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
import os
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import torch

from .config import ModelConfig
from .model import GenesisModel
from .tokenizer import Tokenizer, SPECIAL_TOKENS, SPECIAL_BASE

FORMAT_VERSION = 1


class TokenizerCompatibilityError(Exception):
    """Raised when a checkpoint's tokenizer cannot be verified as trusted."""


def _file_sha256(path: str | Path) -> str:
    """Raw checkpoint file bytes sha256 -- the trust anchor for provenance.

    The loader hashes the checkpoint FILE (not caller-controlled metadata) and
    looks up the trusted record by this digest, so a checkpoint cannot present a
    meta.base that selects a different (or absent) trust record.
    """
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def _canonical_tokenizer_sha(path: Path | str) -> str:
    """sha256 over the NORMALIZED tokenizer json dict (provenance proof).

    Differs from a raw file-byte sha256: identical tokenizers serialized with
    different whitespace compare equal, while a single changed merge breaks the
    digest. This is an integrity/provenance check, NOT cryptographic protection
    against an attacker who can rewrite both the manifest and the artifact.
    """
    data = json.loads(Path(path).read_bytes())
    return hashlib.sha256(
        json.dumps(data, sort_keys=True, ensure_ascii=False).encode("utf-8")
    ).hexdigest()


def _load_tokenizer_provenance() -> dict:
    """Trusted registry: checkpoint file sha256 -> trust record.

    Operators may relocate the manifest via GENESIS_TOKENIZER_REGISTRY (defaults
    to the in-package tokenizer_provenance.json). The LOCATION is a deployment
    config knob; TRUST derives from the registry contents, never from
    caller-controlled checkpoint metadata.
    """
    env = os.environ.get("GENESIS_TOKENIZER_REGISTRY")
    reg_path = Path(env) if env else (
        Path(__file__).resolve().parent / "tokenizer_provenance.json"
    )
    if not reg_path.exists():
        return {}
    try:
        return json.loads(reg_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise TokenizerCompatibilityError(
            f"tokenizer provenance registry at {reg_path} is malformed: {exc}"
        ) from exc


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
    trained: str = ""
    shift_fixed: bool = False
    tool_call_acc: float = 0.0
    base: str = ""

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
            "trained": self.trained,
            "shift_fixed": self.shift_fixed,
            "tool_call_acc": self.tool_call_acc,
            "base": self.base,
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
    fmt = payload.get("format_version")
    checkpoint_path = Path(path)
    
    if fmt == FORMAT_VERSION:
        config = ModelConfig.from_dict(payload["config"])
        tokenizer = Tokenizer.from_dict(payload["tokenizer"])
    elif fmt == "genesis-ckpt-v1":
        # Legacy compact checkpoint: n_head/n_embd/block_size keys, no tokenizer
        raw = payload["config"]
        n_head = raw.get("n_head", 8)
        config = ModelConfig(
            vocab_size=raw.get("vocab_size", 4096),
            d_model=raw.get("n_embd", 256),
            n_layers=raw.get("n_layer", 6),
            n_heads=n_head,
            n_kv_heads=1,
            d_ff=raw.get("d_ff", 344),
            max_seq_len=raw.get("block_size", 256),
            rope_theta=10000.0,
            tie_embeddings=True,
            norm_eps=1e-6,
        )
        # --- Provenance: anchor on the CHECKPOINT FILE (bytes), never metadata ---
        ckpt_sha = _file_sha256(checkpoint_path)
        registry = _load_tokenizer_provenance()
        entry = registry.get("by_checkpoint_sha256", {}).get(ckpt_sha)
        if not entry:
            raise TokenizerCompatibilityError(
                f"no trusted provenance record for checkpoint sha {ckpt_sha[:16]}; "
                f"refusing to load legacy checkpoint {checkpoint_path.name} "
                f"(caller-controlled metadata cannot select a trust record)."
            )
        if entry.get("format") != "genesis-ckpt-v1":
            raise TokenizerCompatibilityError(
                f"registry format {entry.get('format')!r} != 'genesis-ckpt-v1' "
                f"for checkpoint sha {ckpt_sha[:16]}."
            )
        if entry.get("vocab_size") != config.vocab_size:
            raise TokenizerCompatibilityError(
                f"registry vocab {entry.get('vocab_size')} != config vocab "
                f"{config.vocab_size}."
            )
        arch = entry.get("architecture") or {}
        if arch and (arch.get("d_model") != config.d_model
                     or arch.get("n_layers") != config.n_layers
                     or arch.get("n_heads") != config.n_heads
                     or arch.get("d_ff") != config.d_ff):
            raise TokenizerCompatibilityError(
                f"registry architecture {arch} != payload architecture "
                f"(d_model={config.d_model}, n_layers={config.n_layers}, "
                f"n_heads={config.n_heads}, d_ff={config.d_ff})."
            )
        meta_base = (payload.get("meta") or {}).get("base")
        if entry.get("meta_base") and meta_base != entry.get("meta_base"):
            raise TokenizerCompatibilityError(
                f"checkpoint meta.base {meta_base!r} != trusted record base "
                f"{entry.get('meta_base')!r} for checkpoint sha {ckpt_sha[:16]}."
            )
        # --- Fail-closed tokenizer resolution (co-located, verified) ---
        tokenizer_path = checkpoint_path.parent / "tokenizer.json"
        if tokenizer_path.is_symlink():
            raise TokenizerCompatibilityError(
                f"co-located tokenizer.json at {tokenizer_path} is a symlink; "
                f"refusing to follow path-escape / unsafe artifact references."
            )
        if not tokenizer_path.exists():
            raise TokenizerCompatibilityError(
                f"legacy checkpoint {checkpoint_path.name} has no co-located "
                f"tokenizer.json; refusing to train a fallback tokenizer "
                f"(token IDs would not match the original training tokenizer)."
            )
        try:
            tokenizer = Tokenizer.load(tokenizer_path)
        except Exception as exc:
            raise TokenizerCompatibilityError(
                f"tokenizer.json at {tokenizer_path} is unreadable or has an "
                f"unrecognized special-token layout: {exc}"
            ) from exc
        if tokenizer.vocab_size != config.vocab_size:
            raise TokenizerCompatibilityError(
                f"tokenizer vocab {tokenizer.vocab_size} != config vocab "
                f"{config.vocab_size} at {tokenizer_path}."
            )
        special_ids = [tokenizer.id_of(t) for t in SPECIAL_TOKENS]
        if special_ids != list(range(SPECIAL_BASE, SPECIAL_BASE + len(SPECIAL_TOKENS))):
            raise TokenizerCompatibilityError(
                f"reserved-token IDs {special_ids} are not "
                f"{SPECIAL_BASE}..{SPECIAL_BASE + len(SPECIAL_TOKENS) - 1}; "
                f"ID-space/lineage mismatch."
            )
        expected_canon = entry.get("tokenizer_canonical_sha256")
        if not expected_canon:
            raise TokenizerCompatibilityError(
                f"registry entry for checkpoint sha {ckpt_sha[:16]} has no "
                f"tokenizer_canonical_sha256; refusing to accept an unverified tokenizer."
            )
        if _canonical_tokenizer_sha(tokenizer_path) != expected_canon:
            raise TokenizerCompatibilityError(
                f"tokenizer canonical sha does not match the trusted fingerprint "
                f"for checkpoint sha {ckpt_sha[:16]} (same "
                f"{tokenizer.vocab_size}-vocab, different ordered merges)."
            )
    else:
        raise ValueError(f"unsupported checkpoint format {fmt}")
    
    model = GenesisModel(config)
    model.load_state_dict(payload["state_dict"], strict=True)
    model.eval()
    meta_raw = payload.get("meta", {}) or {}
    # Ignore unknown/spoofed meta keys (e.g. a forged meta.tokenizer_path):
    # the loader NEVER honors a checkpoint-supplied artifact path, and unknown
    # keys are dropped rather than permitted to crash or redirect loading.
    meta = CheckpointMeta(**{
        k: v for k, v in meta_raw.items() if k in CheckpointMeta.__dataclass_fields__
    })
    return model, tokenizer, meta


def _train_fallback_tokenizer(vocab_size: int) -> Tokenizer:
    """Disabled in the hardened loader; kept only for backward compatibility."""
    raise TokenizerCompatibilityError(
        "_train_fallback_tokenizer is disabled; co-locate a verified tokenizer.json "
        "for genesis-ckpt-v1 checkpoints."
    )
    from pathlib import Path
    import json
    
    root = Path(__file__).resolve().parents[1]
    texts = []
    
    # Collect all project text
    for f in root.rglob("*.txt"):
        if any(p in str(f) for p in [".venv", ".git", "build", "node_modules"]):
            continue
        try:
            content = f.read_text(encoding="utf-8", errors="replace")
            if len(content) > 10:
                texts.append(content)
        except Exception:
            pass
    
    for f in root.rglob("*.jsonl"):
        if any(p in str(f) for p in [".venv", ".git"]):
            continue
        try:
            content = f.read_text(encoding="utf-8", errors="replace")
            if len(content) > 10:
                texts.append(content)
        except Exception:
            pass
    
    if not texts:
        raise RuntimeError("No corpus text found for fallback tokenizer training")
    
    import random
    random.seed(42)
    # Expand corpus with synthetic variations
    base = texts[0] if texts else "Genesis"
    words = base.split()
    for _ in range(500):
        shuffled = words.copy()
        random.shuffle(shuffled)
        texts.append(" ".join(shuffled[:random.randint(5, min(30, len(shuffled)))]))
    
    return Tokenizer.train(texts, vocab_size=vocab_size, verbose=False)


def load_optimizer_state(
    path: str | Path, map_location: str | torch.device = "cpu"
) -> dict[str, Any] | None:
    """Load only the optimiser state from a checkpoint.

    ``weights_only=True`` is kept here as well: optimizer state is tensors and
    primitive containers, so a checkpoint still cannot execute code.
    """
    payload = torch.load(path, map_location=map_location, weights_only=True)
    state = payload.get("optimizer")
    return state if isinstance(state, dict) else None


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
