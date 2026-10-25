"""Real held-out evaluation for a Genesis checkpoint.

This replaces the earlier placeholder, which only described the split. It
loads the checkpoint, proves that no held-out file is present in the training
plan, and measures bits-per-byte and perplexity on text the model never saw.

Usage:
    python scripts/eval_heldout.py --checkpoint runs/local_1m_clean/genesis.pt --device cuda
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from genesis_model.checkpoint import load_checkpoint  # noqa: E402
from genesis_model.eval import decode_throughput, perplexity  # noqa: E402


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def training_plan(checkpoint: Path) -> dict | None:
    """The local_plan.json beside a checkpoint, when the run wrote one."""
    candidate = checkpoint.parent / "local_plan.json"
    if not candidate.exists():
        return None
    return json.loads(candidate.read_text(encoding="utf-8"))


def overlap_report(heldout: list[Path], plan: dict | None) -> dict:
    """Refuse to evaluate when a held-out book also appears in training.

    The comparison is by content digest, not by file name: the contamination
    this guards against was the same book copied into two folders under the
    same name, and a name-only check would have missed a renamed copy.
    """
    heldout_hashes = {path.name: sha256_file(path) for path in heldout}
    if plan is None:
        return {
            "checked": False,
            "reason": "no local_plan.json beside the checkpoint",
            "heldout": heldout_hashes,
        }
    trained = {entry["sha256"] for entry in plan.get("sources", []) if "sha256" in entry}
    overlaps = sorted(name for name, digest in heldout_hashes.items() if digest in trained)
    return {
        "checked": True,
        "overlaps": overlaps,
        "training_files": len(plan.get("sources", [])),
        "excluded_files": plan.get("excluded_files", []),
        "heldout": heldout_hashes,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", required=True)
    parser.add_argument("--heldout", default=str(ROOT / "data" / "local" / "gutenberg_heldout"))
    parser.add_argument("--device", default="cuda")
    parser.add_argument("--block-size", type=int, default=256)
    parser.add_argument("--max-blocks", type=int, default=64,
                        help="how many blocks to score; 0 scores the whole file")
    parser.add_argument("--tool-cases", action="store_true",
                        help="also measure tool-format compliance (slow on a small GPU)")
    parser.add_argument("--output", default=None)
    args = parser.parse_args()

    checkpoint = Path(args.checkpoint)
    heldout_dir = Path(args.heldout)
    if not checkpoint.exists():
        raise SystemExit(f"checkpoint not found: {checkpoint}")
    files = sorted(heldout_dir.glob("*.txt"))
    if not files:
        raise SystemExit(f"no held-out text files in {heldout_dir}")

    plan = training_plan(checkpoint)
    overlap = overlap_report(files, plan)
    if overlap.get("checked") and overlap["overlaps"]:
        raise SystemExit(
            "refusing to call this held-out: these files are in the training plan: "
            + ", ".join(overlap["overlaps"])
        )

    model, tokenizer, meta = load_checkpoint(checkpoint, map_location=args.device)
    model.to(args.device)
    max_blocks = args.max_blocks if args.max_blocks > 0 else 10**9

    per_file = {}
    for path in files:
        text = path.read_text(encoding="utf-8", errors="replace")
        per_file[path.name] = {
            "bytes": len(path.read_bytes()),
            "chars": len(text),
            **perplexity(
                model,
                tokenizer,
                text,
                block_size=args.block_size,
                max_blocks=max_blocks,
            ),
        }

    combined = "\n\n".join(path.read_text(encoding="utf-8", errors="replace") for path in files)
    combined_metrics = perplexity(
        model,
        tokenizer,
        combined,
        block_size=args.block_size,
        max_blocks=max_blocks,
    )

    report = {
        "checkpoint": str(checkpoint),
        "parameters": model.num_parameters(),
        "vocab_size": tokenizer.vocab_size,
        "device": args.device,
        "training": {
            "steps": meta.steps,
            "tokens_seen": meta.tokens_seen,
            "corpus_sha256": meta.corpus_sha256,
        },
        "overlap": overlap,
        "heldout_files": [path.name for path in files],
        "per_file": per_file,
        "combined": combined_metrics,
        "throughput": decode_throughput(model, tokenizer),
        "notes": [
            "Perplexity is comparable only on the same tokenizer and the same held-out text.",
            "The overlap check compares content digests, not file names.",
            "This is a small local model (see parameters above); the numbers are a baseline, not a product claim.",
        ],
    }

    if args.tool_cases:
        from genesis_model.eval import TOOL_CASES, tool_format_accuracy

        report["tool_format"] = tool_format_accuracy(model, tokenizer, TOOL_CASES)

    text = json.dumps(report, indent=2)
    print(text)
    if args.output:
        output = Path(args.output)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(text + "\n", encoding="utf-8")
        print(f"wrote {output}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
