"""Evaluation: honest measurements, no invented scores.

Metrics implemented here:
  * bits-per-byte / perplexity on held-out text;
  * tool-call format validity and tool-name accuracy;
  * decode throughput on the actual device.

A high score on the synthetic shape set is NOT evidence of real visual
understanding; the report labels every metric with its dataset.
"""
from __future__ import annotations

import json
import math
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence

import torch

from .checkpoint import load_checkpoint
from .generate import generate_text
from .model import GenesisModel
from .tokenizer import EOS, Tokenizer
from .tools import parse_tool_calls


@dataclass
class EvalReport:
    checkpoint: str
    parameters: int
    vocab_size: int
    metrics: dict
    notes: list[str]

    def to_dict(self) -> dict:
        return {
            "checkpoint": self.checkpoint,
            "parameters": self.parameters,
            "vocab_size": self.vocab_size,
            "metrics": self.metrics,
            "notes": self.notes,
        }


@torch.inference_mode()
def perplexity(
    model: GenesisModel,
    tokenizer: Tokenizer,
    text: str,
    *,
    block_size: int = 256,
    max_blocks: int = 16,
) -> dict:
    ids = tokenizer.encode(text)
    if len(ids) < 3:
        return {"tokens": len(ids), "loss": None, "perplexity": None, "bits_per_byte": None}
    device = next(model.parameters()).device
    losses = []
    total_nats = 0.0
    total_tokens = 0
    bytes_count = 0
    for start in range(0, min(len(ids) - 1, block_size * max_blocks), block_size):
        chunk = ids[start: start + block_size + 1]
        if len(chunk) < 2:
            continue
        x = torch.tensor([chunk[:-1]], dtype=torch.long, device=device)
        y = torch.tensor([chunk[1:]], dtype=torch.long, device=device)
        _, loss = model(x, targets=y)
        value = float(loss)
        losses.append(value)
        total_nats += value * y.numel()
        total_tokens += y.numel()
        bytes_count += len(tokenizer.decode(chunk))
    mean_loss = total_nats / total_tokens if total_tokens else None
    return {
        "tokens": len(ids),
        "blocks": len(losses),
        "scored_tokens": total_tokens,
        "bytes": bytes_count,
        "loss": round(mean_loss, 6) if mean_loss is not None else None,
        "perplexity": round(math.exp(mean_loss), 4) if mean_loss is not None else None,
        # Cross-entropy is nats per token. Bits per byte needs the byte count of
        # the same text, otherwise the number is not comparable across tokenizers.
        "bits_per_byte": (
            round(total_nats / math.log(2) / bytes_count, 6)
            if mean_loss is not None and bytes_count
            else None
        ),
    }


@torch.inference_mode()
def tool_format_accuracy(
    model: GenesisModel,
    tokenizer: Tokenizer,
    cases: Sequence[dict],
    *,
    max_cases: int = 8,
    max_new_tokens: int = 96,
) -> dict:
    valid = 0
    named = 0
    attempted = 0
    details = []
    for case in cases[:max_cases]:
        prompt = case["prompt"]
        expected = case["expected_tool"]
        output = generate_text(
            model, tokenizer, prompt, max_new_tokens=max_new_tokens,
            temperature=0.0, top_k=None,
        )
        calls = parse_tool_calls(output)
        attempted += 1
        if calls:
            valid += 1
            if calls[0].name == expected:
                named += 1
        details.append({"prompt": prompt, "expected": expected, "output": output,
                        "parsed": [c.name for c in calls]})
    return {
        "cases": attempted,
        "format_valid": valid,
        "correct_tool": named,
        "format_rate": round(valid / attempted, 4) if attempted else None,
        "tool_accuracy": round(named / attempted, 4) if attempted else None,
        "details": details,
    }


@torch.inference_mode()
def decode_throughput(
    model: GenesisModel, tokenizer: Tokenizer, *, new_tokens: int = 32
) -> dict:
    prompt = "Genesis is"
    ids = tokenizer.encode(prompt)
    start = time.perf_counter()
    output = model.generate_ids(ids, max_new_tokens=new_tokens, temperature=0.0, top_k=None)
    elapsed = time.perf_counter() - start
    return {
        "generated_tokens": len(output),
        "seconds": round(elapsed, 4),
        "tokens_per_second": round(len(output) / elapsed, 2) if elapsed else None,
        "device": str(next(model.parameters()).device),
    }


TOOL_CASES = [
    {"prompt": "What is 12 * 9?", "expected_tool": "calculator"},
    {"prompt": "List the files in the current folder.", "expected_tool": "list_dir"},
    {"prompt": "Read README.md.", "expected_tool": "read_file"},
    {"prompt": "What time is it in UTC?", "expected_tool": "now"},
]


def run_evaluation(
    checkpoint: str | Path,
    *,
    corpus: str | None = None,
    device: str = "cpu",
    include_tool_cases: bool = True,
) -> EvalReport:
    model, tokenizer, meta = load_checkpoint(checkpoint, map_location=device)
    model.to(device)
    metrics: dict = {}
    notes = [
        "Perplexity is only comparable on the same tokenizer and held-out corpus.",
        "Tool-format accuracy measures format compliance and tool selection, not task success.",
        "Synthetic-shape caption scores do not establish real-world visual understanding.",
    ]
    if corpus:
        metrics["text"] = perplexity(model, tokenizer, Path(corpus).read_text(encoding="utf-8"))
    if include_tool_cases:
        metrics["tool_format"] = tool_format_accuracy(model, tokenizer, TOOL_CASES)
    metrics["throughput"] = decode_throughput(model, tokenizer)
    metrics["training"] = {
        "steps": meta.steps,
        "tokens_seen": meta.tokens_seen,
        "corpus_sha256": meta.corpus_sha256,
    }
    return EvalReport(
        checkpoint=str(checkpoint),
        parameters=model.num_parameters(),
        vocab_size=tokenizer.vocab_size,
        metrics=metrics,
        notes=notes,
    )
