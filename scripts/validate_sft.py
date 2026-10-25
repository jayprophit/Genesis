"""Validate an SFT checkpoint against the Week 1 baseline:

1. Tool-format validity (same 4 bare-prompt TOOL_CASES as Week 1) -> format_valid / correct_tool.
   Week 1 baseline on local_1m_gutenberg = 0/4. This is the consistent before/after gate for DoD #3.
2. Realistic tool-call check: system prompt + one tool schema + task, expect a valid <|tool_call|>.
3. Held-out perplexity regression gate: bpB on the 3 disjoint Gutenberg books. SFT must not
   catastrophically forget the language baseline (Week 1 bpB: pg34566 2.12, pg5 2.50, pg753 2.87).

Usage: python scripts/validate_sft.py --checkpoint runs/sft_a_tool_seq256/genesis.pt
"""
from __future__ import annotations
import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from genesis_model.checkpoint import load_checkpoint  # noqa: E402
from genesis_model.eval import run_evaluation, tool_format_accuracy, TOOL_CASES  # noqa: E402
from genesis_model.generate import generate_text  # noqa: E402
from genesis_model.tokenizer import Tokenizer  # noqa: E402

HELDOUT = {
    "pg34566": r"C:/Users/jpowe/AppData/Local/hermes/cache/scratch/genesis_work/heldout_clean/pg34566.txt",
    "pg5": r"C:/Users/jpowe/AppData/Local/hermes/cache/scratch/genesis_work/heldout_clean/pg5.txt",
    "pg753": r"C:/Users/jpowe/AppData/Local/hermes/cache/scratch/genesis_work/heldout_clean/pg753.txt",
}

SYSTEM = (
    "You are Genesis, a from-scratch native AI assistant. When a tool is needed, emit exactly one "
    "tool call as <|tool_call|>{\"name\": ..., \"arguments\": {...}}<|/tool_call|>."
)


def realistic_tool_check(model, tokenizer, device):
    """Feed a realistic agent-style prompt (system + one tool schema) and check a valid call."""
    tools = [{"name": "calculator", "description": "Evaluate an arithmetic expression."}]
    messages = [
        {"role": "system", "content": SYSTEM},
        {"role": "user", "content": "What is 23 * 17?"},
    ]
    ids = tokenizer.apply_chat_template(messages, tools=tools, add_generation_prompt=True)
    out = generate_text(model, tokenizer, ids, max_new_tokens=64, temperature=0.0, top_k=None)
    text = tokenizer.decode(out)
    has_call = "<|tool_call|>" in text and "<|/tool_call|>" in text
    return {"has_tool_call": has_call, "output_tail": text[-160:]}


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--checkpoint", required=True)
    ap.add_argument("--device", default="cpu")
    args = ap.parse_args()

    model, tokenizer, meta = load_checkpoint(args.checkpoint, map_location=args.device)
    model.to(args.device)
    model.eval()

    print("=== checkpoint ===", flush=True)
    print(json.dumps({"steps": meta.steps, "tokens_seen": meta.tokens_seen,
                      "vocab": tokenizer.vocab_size}, indent=2), flush=True)

    # 1. Consistent before/after tool-format validity (bare prompts, greedy).
    print("\n=== tool-format validity (bare prompts, Week 1 baseline = 0/4) ===", flush=True)
    rep = run_evaluation(args.checkpoint, device=args.device, include_tool_cases=True)
    tm = rep.metrics.get("tool_format", {})
    print(json.dumps({"format_valid": tm.get("format_valid"),
                      "correct_tool": tm.get("correct_tool"),
                      "format_rate": tm.get("format_rate"),
                      "tool_accuracy": tm.get("tool_accuracy")}, indent=2), flush=True)
    for d in tm.get("details", []):
        print("  case:", d["prompt"], "| expected:", d["expected"],
              "| parsed:", d["parsed"], "| out_tail:", d["output"][-80:].replace("\n", " "), flush=True)

    # 2. Realistic agent-style tool call.
    print("\n=== realistic tool-call (system + schema) ===", flush=True)
    print(json.dumps(realistic_tool_check(model, tokenizer, args.device), indent=2), flush=True)

    # 3. Held-out perplexity regression gate.
    print("\n=== held-out perplexity regression (Week 1 bpB: 2.12 / 2.50 / 2.87) ===", flush=True)
    from genesis_model.eval import perplexity
    for name, path in HELDOUT.items():
        text = Path(path).read_text(encoding="utf-8", errors="replace")[:20000]
        r = perplexity(model, tokenizer, text, block_size=256, max_blocks=16)
        print(f"  {name}: loss={r.get('loss')} ppl={r.get('perplexity')} bpB={r.get('bits_per_byte')}", flush=True)


if __name__ == "__main__":
    main()
