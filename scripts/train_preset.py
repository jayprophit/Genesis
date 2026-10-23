"""Train a named preset with an explicit, reproducible command-line record."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from genesis_model.config import ModelConfig  # noqa: E402
from genesis_model.train import TrainConfig, train  # noqa: E402


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", default="configs/tiny.json")
    parser.add_argument("--steps", type=int, default=100)
    parser.add_argument("--batch-size", type=int, default=4)
    parser.add_argument("--seq-len", type=int, default=128)
    parser.add_argument("--lr", type=float, default=3e-3)
    parser.add_argument("--out", default="runs/tiny")
    parser.add_argument("--corpus", action="append", default=[])
    parser.add_argument("--sft", action="append", default=[])
    parser.add_argument("--device", default="auto")
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()

    raw = json.loads((ROOT / args.config).read_text(encoding="utf-8"))
    raw.pop("name", None)
    raw.pop("description", None)
    model = ModelConfig.from_dict(raw)
    config = TrainConfig(
        steps=args.steps,
        batch_size=args.batch_size,
        seq_len=args.seq_len,
        lr=args.lr,
        out_dir=str(ROOT / args.out),
        device=args.device,
        seed=args.seed,
        model=model,
        notes=f"preset={args.config}",
    )
    manifest = train(config, corpus_paths=args.corpus or None, sft_paths=args.sft or None)
    print(json.dumps({k: v for k, v in manifest.items() if k != "history"}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
