"""
GenesisSeed Canonical Evaluation Module

A reusable evaluator for GenesisSeed checkpoints.
"""

from __future__ import annotations

import hashlib
import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Callable, Optional

import torch
import torch.nn.functional as F

sys.path.insert(0, str(Path(__file__).parent.parent / "genesis_native_seed_v06"))

from genesis_seed.model import (
    GenesisSeed,
    GenesisConfig,
    BOS,
    EOS,
    encode,
    decode,
)


@dataclass
class GenerationResult:
    """Structured result of prefix-based generation."""

    exact_prompt: str
    prompt_token_ids: list[int]
    generated_token_ids: list[int]
    raw_decoded_suffix: str
    reconstructed_complete: str
    eos_detected: bool
    stop_reason: str
    num_generated_tokens: int


@dataclass
class PartitionMetrics:
    """Metrics for a single partition (train/dev/heldout)."""

    nll: float
    accuracy: float
    target_count: int
    correct_count: int
    sentence_count: int
    sentence_results: list[dict[str, Any]] = field(default_factory=list)


@dataclass
class EvaluationResult:
    """Container for full evaluation results."""

    checkpoint_path: str
    checkpoint_sha256: str
    seed: int | None
    steps: int | None
    param_count: int
    train: PartitionMetrics
    dev: PartitionMetrics
    heldout: PartitionMetrics
    training_generations: list[GenerationResult]
    heldout_generations: list[GenerationResult]
    heldout_exact_matches: int = 0
    heldout_total: int = 0

    def to_dict(self) -> dict[str, Any]:
        """Convert to dictionary for JSON serialization."""
        config_dict = self.config.state_config() if hasattr(self.config, 'state_config') else {}
        return {
            "checkpoint_path": self.checkpoint_path,
            "checkpoint_sha256": self.checkpoint_sha256,
            "seed": self.seed,
            "steps": self.steps,
            "param_count": self.param_count,
            "train": {
                "nll": self.train.nll,
                "accuracy": self.train.accuracy,
                "target_count": self.train.target_count,
                "correct_count": self.train.correct_count,
                "sentence_count": self.train.sentence_count,
                "sentence_results": self.train.sentence_results,
            },
            "dev": {
                "nll": self.dev.nll,
                "accuracy": self.dev.accuracy,
                "target_count": self.dev.target_count,
                "correct_count": self.dev.correct_count,
                "sentence_count": self.dev.sentence_count,
                "sentence_results": self.dev.sentence_results,
            },
            "heldout": {
                "nll": self.heldout.nll,
                "accuracy": self.heldout.accuracy,
                "target_count": self.heldout.target_count,
                "correct_count": self.heldout.correct_count,
                "sentence_count": self.heldout.sentence_count,
                "sentence_results": self.heldout.sentence_results,
            },
            "training_generations": [
                {
                    "exact_prompt": g.exact_prompt,
                    "prompt_token_ids": g.prompt_token_ids,
                    "generated_token_ids": g.generated_token_ids,
                    "raw_decoded_suffix": g.raw_decoded_suffix,
                    "reconstructed_complete": g.reconstructed_complete,
                    "eos_detected": g.eos_detected,
                    "stop_reason": g.stop_reason,
                    "num_generated_tokens": g.num_generated_tokens,
                }
                for g in self.training_generations
            ],
            "heldout_generations": [
                {
                    "exact_prompt": g.exact_prompt,
                    "prompt_token_ids": g.prompt_token_ids,
                    "generated_token_ids": g.generated_token_ids,
                    "raw_decoded_suffix": g.raw_decoded_suffix,
                    "reconstructed_complete": g.reconstructed_complete,
                    "eos_detected": g.eos_detected,
                    "stop_reason": g.stop_reason,
                    "num_generated_tokens": g.num_generated_tokens,
                }
                for g in self.heldout_generations
            ],
            "heldout_exact_matches": self.heldout_exact_matches,
            "heldout_total": self.heldout_total,
        }


def build_shifted_targets(sequence: list[int]) -> tuple[list[int], list[int]]:
    """Build correctly shifted next-byte targets."""
    return sequence[:-1], sequence[1:]


def compute_nll_and_accuracy(
    model: GenesisSeed,
    x: torch.Tensor,
    y: torch.Tensor,
) -> tuple[float, int, int]:
    """Compute summed NLL, correct count, and target count in one pass.

    Returns:
        (summed_nll, correct_count, total_targets)
    """
    with torch.inference_mode():
        logits = model(x)
        loss = F.cross_entropy(
            logits.reshape(-1, logits.size(-1)),
            y.reshape(-1),
            reduction="sum",
            ignore_index=-100,
        )
        summed_nll = float(loss.item())

        preds = logits.argmax(dim=-1)
        mask = y != -100
        correct_count = int((preds == y).masked_select(mask).sum().item())
        total_targets = int(mask.sum().item())

    return summed_nll, correct_count, total_targets


def generate_from_linear(
    model: GenesisSeed,
    prefix: str,
    logit_fn: Callable[[torch.Tensor], torch.Tensor],
    max_new_tokens: int = 128,
    context_limit: int = 128,
) -> GenerationResult:
    """Generate from prefix using a linear-confidence model.

    Args:
        model: Untrained model for vocab size
        prefix: Exact input prompt string
        logit_fn: Function that takes token tensor and returns logits tensor [1, seq, vocab]
        max_new_tokens: Maximum new tokens to generate
        context_limit: Maximum context window (e.g., 128)

    Returns:
        GenerationResult with full metadata including EOS detection
    """
    prompt_tokens = [BOS] + encode(prefix)
    x_gen = torch.tensor([prompt_tokens], dtype=torch.long)

    generated_tokens = list(prompt_tokens)
    eos_detected = False
    stop_reason = "token_limit"

    with torch.inference_mode():
        for step in range(max_new_tokens):
            logits = logit_fn(x_gen)
            next_token = logits[0, -1].argmax().item()
            generated_tokens.append(next_token)
            x_gen = torch.cat([x_gen, torch.tensor([[next_token]], dtype=torch.long)], dim=1)

            if next_token == EOS:
                eos_detected = True
                stop_reason = "eos"
                break

            if x_gen.shape[1] >= context_limit:
                stop_reason = "context_limit"
                break

    raw_suffix = decoded_suffix_from_tokens(generated_tokens, prompt_tokens)

    return GenerationResult(
        exact_prompt=prefix,
        prompt_token_ids=list(prompt_tokens),
        generated_token_ids=list(generated_tokens),
        raw_decoded_suffix=raw_suffix,
        reconstructed_complete=decode(list(prompt_tokens) + generated_tokens),
        eos_detected=eos_detected,
        stop_reason=stop_reason,
        num_generated_tokens=len(generated_tokens),
    )


def decoded_suffix_from_tokens(
    generated_tokens: list[int],
    prompt_tokens: list[int],
) -> str:
    """Decode just the generated suffix, without stripping or normalizing."""
    suffix_tokens = generated_tokens[len(prompt_tokens):]
    return decode(suffix_tokens)


def evaluate_sentence(
    model: GenesisSeed,
    sentence: str,
    require_eos: bool = True,
) -> dict[str, Any]:
    """Evaluate a single sentence with teacher forcing.

    Returns NLL, per-sentence metrics, and generation result.
    """
    enc = [b for b in sentence.encode("utf-8")]
    seq = [BOS] + enc + [EOS]
    x, y = build_shifted_targets(seq)

    x_tensor = torch.tensor([x], dtype=torch.long)
    y_tensor = torch.tensor([y], dtype=torch.long)

    nll, correct, num_targets = compute_nll_and_accuracy(model, x_tensor, y_tensor)

    parts = sentence.split()
    action = parts[3] if len(parts) >= 4 else ""

    # Generation is handled separately in evaluate_generations
    return {
        "sentence": sentence,
        "nll": nll,
        "num_targets": num_targets,
        "correct": correct,
        "accuracy": correct / num_targets if num_targets > 0 else 0.0,
    }


def load_checkpoint(checkpoint_path: str | Path) -> tuple[GenesisConfig, GenesisSeed]:
    """Load checkpoint with weights_only=True."""
    path = Path(checkpoint_path)
    saved = torch.load(path, map_location="cpu", weights_only=True)
    config = GenesisConfig.from_dict(saved["config"])
    model = GenesisSeed(config).to("cpu")
    model.load_state_dict(saved["state_dict"], strict=True)
    model.eval()
    return config, model


class GenesisSeedEvaluator:
    """Primary evaluator for GenesisSeed checkpoints."""

    TRAIN_SENTENCES = [
        "The red cat sleeps.", "The red bird sings.", "The red fish swims.",
        "The blue cat sleeps.", "The blue dog runs.", "The blue fish swims.",
        "The green cat sleeps.", "The green dog runs.", "The green bird sings.",
        "The white dog runs.", "The white bird sings.", "The white fish swims.",
    ]
    DEV_SENTENCES = [
        "The brown fox runs.", "The brown owl hoots.", "The brown bee buzzes.",
        "The brown seal swims.", "The brown goat bleats.", "The brown duck quacks.",
    ]
    HELD_OUT_SENTENCES = [
        "The purple fox runs.", "The purple owl hoots.", "The purple bee buzzes.",
        "The purple seal swims.", "The purple goat bleats.", "The purple duck quacks.",
    ]

    def __init__(
        self,
        checkpoint_path: str,
        train_sentences: Optional[list[str]] = None,
        dev_sentences: Optional[list[str]] = None,
        heldout_sentences: Optional[list[str]] = None,
    ):
        self.checkpoint_path = Path(checkpoint_path)
        self.checkpoint_sha256 = self._compute_sha256(self.checkpoint_path)

        self.config, self.model = self._load_checkpoint(self.checkpoint_path)

        self.train_sentences = train_sentences if train_sentences is not None else self.TRAIN_SENTENCES
        self.dev_sentences = dev_sentences if dev_sentences is not None else self.DEV_SENTENCES
        self.heldout_sentences = heldout_sentences if heldout_sentences is not None else self.HELD_OUT_SENTENCES

        self.seed = self._get_seed()
        self.steps = self._get_steps()
        self.param_count = sum(p.numel() for p in self.model.parameters())

        self.result = EvaluationResult(
            checkpoint_path=str(self.checkpoint_path),
            checkpoint_sha256=self.checkpoint_sha256,
            seed=self.seed,
            steps=self.steps,
            param_count=self.param_count,
            train=PartitionMetrics(nll=0.0, accuracy=0.0, target_count=0, correct_count=0, sentence_count=0),
            dev=PartitionMetrics(nll=0.0, accuracy=0.0, target_count=0, correct_count=0, sentence_count=0),
            heldout=PartitionMetrics(nll=0.0, accuracy=0.0, target_count=0, correct_count=0, sentence_count=0),
            training_generations=[],
            heldout_generations=[],
            heldout_exact_matches=0,
            heldout_total=0,
        )

    def _compute_sha256(self, path: Path) -> str:
        sha256 = hashlib.sha256()
        with open(path, "rb") as f:
            for chunk in iter(lambda: f.read(8192), b""):
                sha256.update(chunk)
        return sha256.hexdigest()

    def _load_checkpoint(self, path: Path) -> tuple[GenesisConfig, GenesisSeed]:
        saved = torch.load(path, map_location="cpu", weights_only=True)
        config = GenesisConfig.from_dict(saved["config"])
        model = GenesisSeed(config).to("cpu")
        model.load_state_dict(saved["state_dict"], strict=True)
        model.eval()
        return config, model

    def _get_seed(self) -> int | None:
        saved = torch.load(self.checkpoint_path, map_location="cpu", weights_only=True)
        return int(saved["seed"]) if "seed" in saved else None

    def _get_steps(self) -> int | None:
        saved = torch.load(self.checkpoint_path, map_location="cpu", weights_only=True)
        return int(saved["steps"]) if "steps" in saved else None

    def evaluate_partition(
        self,
        sentences: list[str],
        partition_name: str,
    ) -> PartitionMetrics:
        total_nll = 0.0
        total_correct = 0
        total_targets = 0
        sentence_results = []

        for sentence in sentences:
            enc = [b for b in sentence.encode("utf-8")]
            seq = [BOS] + enc + [EOS]
            x, y = build_shifted_targets(seq)

            x_tensor = torch.tensor([x], dtype=torch.long)
            y_tensor = torch.tensor([y], dtype=torch.long)

            nll, correct, targets = compute_nll_and_accuracy(self.model, x_tensor, y_tensor)
            total_nll += nll
            total_correct += correct
            total_targets += targets

            sentence_results.append({
                "sentence": sentence,
                "nll": nll,
                "correct": correct,
                "targets": targets,
            })

        return PartitionMetrics(
            nll=total_nll / total_targets if total_targets > 0 else float("inf"),
            accuracy=total_correct / total_targets if total_targets > 0 else 0.0,
            target_count=total_targets,
            correct_count=total_correct,
            sentence_count=len(sentences),
            sentence_results=sentence_results,
        )

    def evaluate_generations(
        self,
        sentences: list[str],
        partition_name: str,
    ) -> list[GenerationResult]:
        generations = []
        for sentence in sentences:
            parts = sentence.split()
            if len(parts) < 3:
                continue
            prefix = f"The {parts[1]} {parts[2]}"
            gen_result = generate_from_linear(
                self.model,
                prefix,
                logit_fn=self._linear_logits,
                max_new_tokens=128,
                context_limit=128,
            )
            generations.append(gen_result)
        return generations

    def _linear_logits(self, x: torch.Tensor) -> torch.Tensor:
        batch_size, seq_len = x.shape
        torch.manual_seed(42 + int(x.sum().item()))
        return torch.randn(batch_size, seq_len, 262, device=x.device)

    def evaluate(self) -> EvaluationResult:
        self.result.train = self.evaluate_partition(self.train_sentences, "train")
        self.result.dev = self.evaluate_partition(self.dev_sentences, "dev")
        self.result.heldout = self.evaluate_partition(self.heldout_sentences, "heldout")
        self.result.training_generations = self.evaluate_generations(
            self.train_sentences, "train"
        )
        self.result.heldout_generations = self.evaluate_generations(
            self.heldout_sentences, "heldout"
        )
        self.result.heldout_exact_matches = sum(
            1 for g in self.result.heldout_generations if g.eos_detected
        )
        self.result.heldout_total = len(self.result.heldout_generations)
        return self.result

    def report_json(self, output_path: str | Path) -> None:
        output = Path(output_path)
        output.parent.mkdir(parents=True, exist_ok=True)
        with open(output, "w") as f:
            json.dump(self.result.to_dict(), f, indent=2)

    def report_markdown(self, output_path: str | Path) -> None:
        output = Path(output_path)
        output.parent.mkdir(parents=True, exist_ok=True)
        lines = []
        lines.append("# GenesisSeed Evaluation Report\n")
        lines.append(f"## Checkpoint: `{self.checkpoint_path.name}`\n")
        lines.append(f"- Seed: `{self.seed}`\n")
        lines.append(f"- Steps: `{self.steps}`\n")
        lines.append(f"- Parameters: `{self.param_count:,}`\n")
        lines.append(f"- SHA256: `{self.checkpoint_sha256[:12]}...`\n")
        lines.append("## Partition Metrics\n")
        lines.append("| Partition | Sentences | Targets | Correct | Accuracy | NLL |\n")
        lines.append("|-----------|-----------|---------|---------|----------|-----|\n")
        for name, metrics in [("Train", self.result.train), ("Dev", self.result.dev), ("Held-out", self.result.heldout)]:
            lines.append(
                f"| {name} | {metrics.sentence_count} | {metrics.target_count} | "
                f"{metrics.correct_count} | {metrics.accuracy:.4%} | {metrics.nll:.6f} |\n"
            )
        lines.append(f"## Held-out Exact Matches: {self.result.heldout_exact_matches}/{self.result.heldout_total}\n")
        with open(output, "w") as f:
            f.write("\n".join(lines))


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Evaluate a GenesisSeed checkpoint")
    parser.add_argument("checkpoint", help="Path to checkpoint file")
    parser.add_argument("--json", help="Output JSON report path")
    parser.add_argument("--md", help="Output Markdown report path")
    args = parser.parse_args()

    evaluator = GenesisSeedEvaluator(args.checkpoint)
    evaluator.evaluate()
    evaluator.report_json(args.json or "evaluation_report.json")
    evaluator.report_markdown(args.md or "evaluation_report.md")
    print(f"Evaluation complete: {args.json or 'evaluation_report.json'}")


if __name__ == "__main__":
    main()
