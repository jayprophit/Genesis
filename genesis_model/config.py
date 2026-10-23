"""Genesis model configuration.

All sizes are explicit so a checkpoint can never be loaded into a differently
shaped network by accident. Nothing here downloads weights or references a
third-party model: Genesis is initialised from random and trained by us.
"""
from __future__ import annotations

import json
from dataclasses import asdict, dataclass, field
from pathlib import Path


@dataclass(frozen=True)
class VisionConfig:
    image_size: int = 64
    patch_size: int = 8
    width: int = 128
    layers: int = 2
    heads: int = 4
    dropout: float = 0.0

    def __post_init__(self) -> None:
        if self.image_size % self.patch_size:
            raise ValueError("image_size must be divisible by patch_size")
        if self.width % self.heads:
            raise ValueError("vision width must be divisible by heads")


@dataclass(frozen=True)
class ModelConfig:
    # Vocabulary is set by the tokenizer that is trained with the model.
    vocab_size: int = 4096
    d_model: int = 256
    n_layers: int = 6
    n_heads: int = 8
    n_kv_heads: int = 2
    d_ff: int = 688
    max_seq_len: int = 1024
    rope_theta: float = 10000.0
    dropout: float = 0.0
    tie_embeddings: bool = True
    norm_eps: float = 1e-6
    # Multimodal is optional. A text-only checkpoint sets vision=None.
    vision: VisionConfig | None = None

    def __post_init__(self) -> None:
        if self.d_model % self.n_heads:
            raise ValueError("d_model must be divisible by n_heads")
        if self.n_heads % self.n_kv_heads:
            raise ValueError("n_heads must be a multiple of n_kv_heads")
        if self.d_model % 2:
            raise ValueError("d_model must be even for rotary embeddings")

    @property
    def head_dim(self) -> int:
        return self.d_model // self.n_heads

    def to_dict(self) -> dict:
        data = asdict(self)
        if self.vision is not None:
            data["vision"] = asdict(self.vision)
        return data

    @classmethod
    def from_dict(cls, data: dict) -> "ModelConfig":
        data = dict(data)
        vision = data.get("vision")
        if isinstance(vision, dict):
            data["vision"] = VisionConfig(**vision)
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})

    def save(self, path: str | Path) -> None:
        Path(path).write_text(json.dumps(self.to_dict(), indent=2), encoding="utf-8")

    @classmethod
    def load(cls, path: str | Path) -> "ModelConfig":
        return cls.from_dict(json.loads(Path(path).read_text(encoding="utf-8")))


# Named presets. "tiny" is for correctness tests on CPU, not for product quality.
PRESETS: dict[str, ModelConfig] = {
    "tiny": ModelConfig(
        vocab_size=4096, d_model=128, n_layers=2, n_heads=4, n_kv_heads=1,
        d_ff=344, max_seq_len=256, vision=VisionConfig(width=64, layers=1, heads=4),
    ),
    "seed_30m": ModelConfig(
        vocab_size=16384, d_model=512, n_layers=8, n_heads=8, n_kv_heads=2,
        d_ff=1408, max_seq_len=2048, vision=VisionConfig(width=256, layers=4, heads=8),
    ),
    "seed_120m": ModelConfig(
        vocab_size=32768, d_model=768, n_layers=12, n_heads=12, n_kv_heads=4,
        d_ff=2048, max_seq_len=4096, vision=VisionConfig(width=384, layers=6, heads=6),
    ),
    "genesis_1b": ModelConfig(
        vocab_size=65536, d_model=2048, n_layers=24, n_heads=16, n_kv_heads=4,
        d_ff=5632, max_seq_len=8192, vision=VisionConfig(width=768, layers=12, heads=12),
    ),
}
