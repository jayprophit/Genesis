"""One-command local Genesis training.

This is the local-first entry point: point it at a folder of text, choose a
model that fits the machine, and it tokenizes, trains, checkpoints and prints
the exact commands to chat with the result. No API keys, no rented GPU.

Examples:
    # A small CPU run (roughly 20M tokens)
    python scripts/local_train.py --corpus C:\\books --preset local_1m --tokens 20000000

    # A 4 GB GPU run (roughly 120M tokens)
    python scripts/local_train.py --corpus C:\\books --preset local_6m --tokens 120000000

    # Continue an interrupted run (same --tokens / --steps)
    python scripts/local_train.py --corpus C:\\books --preset local_6m ^
        --tokens 120000000 --resume runs/local_6m/genesis.pt
"""
from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from genesis_model.config import PRESETS, ModelConfig  # noqa: E402
from genesis_model.corpus import (  # noqa: E402
    corpus_manifest,
    document_sample,
    expand_paths,
    iter_documents,
)
from genesis_model.data import tokenize_to_shard  # noqa: E402
from genesis_model.tokenizer import Tokenizer  # noqa: E402
from genesis_model.train import TrainConfig, train  # noqa: E402

# Token budgets are ~20x parameters, capped where a local run would become a
# multi-week project. They are targets, not claims about quality.
TOKEN_BUDGETS = {
    "local_1m": 20_000_000,
    "local_3m": 60_000_000,
    "local_6m": 120_000_000,
    "local_15m": 300_000_000,
    "local_30m": 300_000_000,
}

DEFAULT_BATCH = {
    "local_1m": 32,
    "local_3m": 32,
    "local_6m": 16,
    "local_15m": 8,
    "local_30m": 4,
}


def _learning_rate(preset: str) -> float:
    if preset == "local_30m":
        return 6e-4
    if preset == "local_15m":
        return 1e-3
    return 3e-3


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--corpus", action="append", required=True,
                        help="file or folder; repeat for multiple roots")
    parser.add_argument("--preset", choices=sorted(TOKEN_BUDGETS), default="local_6m")
    parser.add_argument("--tokens", type=int, default=0,
                        help="target training tokens; 0 uses the preset budget")
    parser.add_argument("--steps", type=int, default=0,
                        help="override the computed number of optimizer steps")
    parser.add_argument("--batch-size", type=int, default=0)
    parser.add_argument("--seq-len", type=int, default=0)
    parser.add_argument("--grad-accum", type=int, default=1)
    parser.add_argument("--lr", type=float, default=0.0)
    parser.add_argument("--vocab-size", type=int, default=0,
                        help="0 uses the preset vocabulary size")
    parser.add_argument("--tokenizer-chars", type=int, default=250_000)
    parser.add_argument("--out", default="runs/local")
    parser.add_argument("--device", default="auto")
    parser.add_argument("--max-minutes", type=float, default=0.0)
    parser.add_argument("--resume", default=None)
    parser.add_argument("--reuse", action="store_true",
                        help="reuse tokenizer.json and corpus.bin if present")
    args = parser.parse_args()

    started = time.time()
    corpus = expand_paths(args.corpus)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    tokenizer_path = out / "tokenizer.json"
    shard_path = out / "corpus.bin"
    preset = PRESETS[args.preset]
    vocab_size = args.vocab_size or preset.vocab_size

    if tokenizer_path.exists() and (args.reuse or args.resume):
        tokenizer = Tokenizer.load(tokenizer_path)
    elif args.resume:
        # The checkpoint is the authority on the tokenizer. Recover it rather
        # than training a different vocabulary against an existing shard.
        from genesis_model.checkpoint import load_checkpoint

        _, tokenizer, _ = load_checkpoint(args.resume, map_location="cpu")
        tokenizer.save(tokenizer_path)
    else:
        sample = document_sample(corpus, max_chars=args.tokenizer_chars)
        if not sample:
            raise SystemExit("no documents found")
        chars = sum(len(document) for document in sample)
        print(f"tokenizer: training on {chars:,} characters...", flush=True)
        tokenizer = Tokenizer.train(sample, vocab_size=vocab_size, verbose=True)
        tokenizer.save(tokenizer_path)
    print(f"tokenizer: vocab={tokenizer.vocab_size} -> {tokenizer_path}", flush=True)

    if shard_path.exists() and (args.reuse or args.resume):
        token_count = shard_path.stat().st_size // 4
        print(f"shard: reusing {shard_path} ({token_count:,} tokens)", flush=True)
    else:
        print(f"shard: tokenizing {len(corpus)} files...", flush=True)
        token_count = tokenize_to_shard(
            iter_documents(corpus), tokenizer, shard_path, dtype="uint32"
        )
        print(f"shard: {token_count:,} tokens -> {shard_path}", flush=True)

    seq_len = args.seq_len or preset.max_seq_len
    if token_count < seq_len + 1:
        raise SystemExit(
            f"corpus has {token_count} tokens but seq_len={seq_len}; "
            "add more text or use a smaller --seq-len"
        )
    batch_size = args.batch_size or DEFAULT_BATCH[args.preset]
    max_batch = max(1, token_count // seq_len)
    if batch_size > max_batch:
        print(f"batch: reducing {batch_size} -> {max_batch} for this corpus size", flush=True)
        batch_size = max_batch
    target_tokens = args.tokens or TOKEN_BUDGETS[args.preset]
    tokens_per_step = batch_size * seq_len * args.grad_accum
    steps = args.steps or max(1, target_tokens // tokens_per_step)
    model = ModelConfig.from_dict({**preset.to_dict(), "max_seq_len": max(seq_len, preset.max_seq_len)})

    sft_path = ROOT / "data" / "sft" / "tool_use.jsonl"
    sft_paths = [sft_path] if sft_path.exists() else None

    config = TrainConfig(
        steps=steps,
        batch_size=batch_size,
        seq_len=seq_len,
        lr=args.lr or _learning_rate(args.preset),
        warmup_steps=max(1, min(200, steps // 100)),
        grad_accum=args.grad_accum,
        log_every=max(1, min(20, steps // 20 or 1)),
        save_every=max(50, steps // 20),
        max_seconds=args.max_minutes * 60.0,
        out_dir=str(out),
        device=args.device,
        sft_weight=0.05 if sft_paths else 0.0,
        vision_weight=0.0,
        shard_path=str(shard_path),
        tokenizer_path=str(tokenizer_path),
        model=model,
        notes=f"local-first run: {args.preset}",
    )

    plan = {
        "preset": args.preset,
        "d_model": preset.d_model,
        "corpus_files": len(corpus),
        "corpus_bytes": sum(path.stat().st_size for path in corpus),
        "corpus_tokens": token_count,
        "target_tokens": target_tokens,
        "tokens_per_step": tokens_per_step,
        "steps": steps,
        "batch_size": batch_size,
        "seq_len": seq_len,
        "grad_accum": args.grad_accum,
        "device": args.device,
        "vocab_size": tokenizer.vocab_size,
        "sft": bool(sft_paths),
        "sources": corpus_manifest(corpus),
    }
    (out / "local_plan.json").write_text(json.dumps(plan, indent=2), encoding="utf-8")
    print(json.dumps({k: plan[k] for k in (
        "preset", "corpus_bytes", "corpus_tokens", "target_tokens",
        "tokens_per_step", "steps", "batch_size", "seq_len", "device", "vocab_size",
    )}, indent=2), flush=True)

    manifest = train(config, sft_paths=sft_paths, resume_from=args.resume)
    summary = {
        "checkpoint": manifest["checkpoint"],
        "parameters": manifest["parameters"],
        "steps": manifest["steps"],
        "tokens_seen": manifest["tokens_seen"],
        "device": manifest["device"],
        "device_name": manifest["device_name"],
        "peak_vram_mb": manifest["peak_vram_mb"],
        "duration_s": manifest["duration_s"],
        "timed_out": manifest["timed_out"],
        "last_loss": (manifest["history"][-1]["text_ce"] if manifest["history"] else None),
        "tok_s": (manifest["history"][-1].get("tok_s") if manifest["history"] else None),
        "checkpoint_command": (
            f"python -m genesis_model.cli chat --checkpoint \"{manifest['checkpoint']}\" --device {args.device}"
        ),
        "eval_command": (
            f"python -m genesis_model.cli eval --checkpoint \"{manifest['checkpoint']}\""
        ),
    }
    print(json.dumps(summary, indent=2), flush=True)
    if manifest["timed_out"]:
        print("time limit reached; re-run the same command with --resume "
              f"\"{manifest['checkpoint']}\"", flush=True)
    print(f"wall time: {time.time() - started:.1f}s", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
