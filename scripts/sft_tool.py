"""Thin driver over genesis_model.train.train() for a focused tool/instruction SFT
stage that resumes the base Gutenberg checkpoint.

Why this exists: scripts/local_train.py hardcodes sft_weight=0.05 and does not
expose the SFT weight or a clean resume-only entry point. This driver resumes the
base checkpoint, mixes the expanded data/sft/tool_use.jsonl corpus at a chosen
weight, and writes to a NEW run dir (the base is never overwritten).

Identity rule: resumption continues the from-scratch base; the SFT corpus is
synthetic first-party tool trajectories (see scripts/gen_sft_corpus.py). No
borrowed weights, no distillation.

Example:
  python scripts/sft_tool.py --steps 6 --out <scratch>/sft_smoke --device cpu
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from genesis_model.config import ModelConfig  # noqa: E402
from genesis_model.train import TrainConfig, train  # noqa: E402

BASE = ROOT / "runs" / "local_1m_gutenberg" / "genesis.pt"
SFT = ROOT / "data" / "sft" / "tool_use.jsonl"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--steps", type=int, default=2000)
    ap.add_argument("--out", type=str, required=True)
    ap.add_argument("--resume", type=str, default=str(BASE))
    ap.add_argument("--sft", type=str, default=str(SFT))
    ap.add_argument("--seq-len", type=int, default=256)
    ap.add_argument("--batch-size", type=int, default=8)
    ap.add_argument("--lr", type=float, default=1e-4)
    ap.add_argument("--sft-weight", type=float, default=0.5)
    ap.add_argument("--device", type=str, default="cpu")
    ap.add_argument("--shard", type=str, default=str(BASE.parent / "corpus.bin"))
    ap.add_argument("--tokenizer-path", type=str, default=str(BASE.parent / "tokenizer.json"))
    args = ap.parse_args()

    # train() treats config.steps as an ABSOLUTE target and resumes from the
    # checkpoint's meta.steps up to it. Read the base step count so --steps means
    # "N new steps" rather than an absolute target below the resume point.
    from genesis_model.checkpoint import load_checkpoint as _load_ckpt
    _m, _t, _meta = _load_ckpt(args.resume, map_location="cpu")
    start_step = max(0, int(_meta.steps))
    target_steps = start_step + args.steps

    # Match the base model architecture so resume loads cleanly.
    model = ModelConfig.from_dict({
        "d_model": 128, "n_layers": 4, "n_heads": 4, "n_kv_heads": 1,
        "vocab_size": 4096, "max_seq_len": args.seq_len, "weight_tying": None,
    })
    steps = target_steps
    config = TrainConfig(
        steps=steps,
        batch_size=args.batch_size,
        seq_len=args.seq_len,
        lr=args.lr,
        warmup_steps=max(1, steps // 20),
        grad_accum=1,
        log_every=max(1, min(20, steps // 20 or 1)),
        save_every=max(50, steps // 20),
        max_seconds=0.0,
        device=args.device,
        sft_weight=args.sft_weight,
        vision_weight=0.0,
        out_dir=args.out,
        shard_path=args.shard,
        tokenizer_path=args.tokenizer_path,
        model=model,
        notes="tool/instruction SFT resumed from local_1m_gutenberg",
    )
    print(json.dumps({
        "resume": args.resume, "sft": args.sft, "steps": steps,
        "seq_len": args.seq_len, "batch_size": args.batch_size,
        "lr": args.lr, "sft_weight": args.sft_weight,
        "shard": args.shard, "out": args.out,
    }), flush=True)
    manifest = train(config, sft_paths=[args.sft], resume_from=args.resume)
    print(json.dumps({
        "checkpoint": manifest["checkpoint"],
        "steps": manifest["steps"], "steps_this_run": manifest["steps_this_run"],
        "tokens_seen": manifest["tokens_seen"], "device": manifest["device"],
        "duration_s": manifest["duration_s"],
    }, indent=2), flush=True)


if __name__ == "__main__":
    main()
