"""
GenesisSeed Canonical Evaluation Module

A reusable evaluator for GenesisSeed checkpoints.
"""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

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
class EvaluationResult:
    """Container for evaluation metrics."""

    checkpoint_path: str
    seed: int
    steps: int
    config: dict
    param_count: int
    train: dict = field(default_factory=dict)
    dev: dict = field(default_factory=dict)
    heldout: dict = field(default_factory=dict)
    training_generations: list = field(default_factory=list)
    heldout_generations: list = field(default_factory=list)
    train_per_sentence: list = field(default_factory=list)
    dev_per_sentence: list = field(default_factory=list)
    heldout_per_sentence: list = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        """Convert to dictionary for JSON serialization."""
        # Convert config to a plain dict if needed
        config_dict = self.config.state_config() if hasattr(self.config, 'state_config') else {}
        seed_val = int(config_dict.get("seed", 42)) if config_dict else 42
        steps_val = int(config_dict.get("steps", 0)) if config_dict else 0
        return {
            "checkpoint_path": self.checkpoint_path,
            "seed": seed_val,
            "steps": steps_val,
            "config": config_dict,
            "param_count": self.param_count,
            "train": self.train,
            "dev": self.dev,
            "heldout": self.heldout,
            "training_generations": self.training_generations,
            "heldout_generations": self.heldout_generations,
            "train_per_sentence": self.train_per_sentence,
            "dev_per_sentence": self.dev_per_sentence,
            "heldout_per_sentence": self.heldout_per_sentence,
        }


def build_shifted_targets(sequence: list[int]) -> tuple[list[int], list[int]]:
    """Build correctly shifted next-byte targets."""
    return sequence[:-1], sequence[1:]


def compute_nll_loss(model: GenesisSeed, x: torch.Tensor, y: torch.Tensor) -> tuple[float, int]:
    """Compute summed NLL and target count."""
    with torch.inference_mode():
        logits = model(x)
        loss = F.cross_entropy(
            logits.reshape(-1, logits.size(-1)),
            y.reshape(-1),
            reduction="sum",
            ignore_index=-100,
        )
        num_targets = (y != -100).sum().item()
    return float(loss.item()), num_targets


def generate_from_prefix(
    model: GenesisSeed,
    prefix: str,
    max_new_tokens: int = 128,
    require_eos: bool = True,
) -> str:
    """Generate from an exact distinguishing prefix."""
    prompt_tokens = [BOS] + encode(prefix)
    x_gen = torch.tensor([prompt_tokens], dtype=torch.long)
    generated = list(prompt_tokens)

    with torch.inference_mode():
        for _ in range(max_new_tokens):
            logits = model(x_gen)
            next_token = logits[0, -1].argmax().item()
            generated.append(next_token)
            x_gen = torch.cat([x_gen, torch.tensor([[next_token]], dtype=torch.long)], dim=1)
            if next_token == EOS:
                break
    return decode(generated)


def evaluate_sentence(
    model: GenesisSeed,
    sentence: str,
    require_eos: bool = True,
) -> dict[str, Any]:
    """Evaluate a single sentence."""
    enc = [b for b in sentence.encode("utf-8")]
    seq = [BOS] + enc + [EOS]
    x, y = build_shifted_targets(seq)

    x_tensor = torch.tensor([x], dtype=torch.long)
    y_tensor = torch.tensor([y], dtype=torch.long)

    nll, num_targets = compute_nll_loss(model, x_tensor, y_tensor)

    with torch.inference_mode():
        logits = model(x_tensor)
        preds = logits.argmax(dim=-1)
        correct = (preds.squeeze(0) == y_tensor.squeeze(0)).float().sum().item()
        accuracy = correct / num_targets if num_targets > 0 else 0.0

    # Extract action
    parts = sentence.split()
    action = parts[3] if len(parts) >= 4 else ""

    generated = generate_from_prefix(model, f"The {parts[1]} {parts[2]}" if len(parts) >= 3 else sentence)
    action_in_gen = action in generated.lower()
    exact_match = generated.strip() == sentence.strip()

    return {
        "sentence": sentence,
        "nll": nll,
        "num_targets": num_targets,
        "accuracy": accuracy,
        "action_accuracy": action_in_gen,
        "exact_sentence_match": exact_match,
        "generated_text": generated,
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
    """Canonical evaluator for GenesisSeed checkpoints."""

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
        checkpoint_path: str | Path,
        train_sentences: list[str] | None = None,
        dev_sentences: list[str] | None = None,
        heldout_sentences: list[str] | None = None,
    ):
        self.checkpoint_path = str(checkpoint_path)
        self.config, self.model = load_checkpoint(self.checkpoint_path)
        self.train_sentences = train_sentences or self.TRAIN_SENTENCES
        self.dev_sentences = dev_sentences or self.DEV_SENTENCES
        self.heldout_sentences = heldout_sentences or self.HELD_OUT_SENTENCES

        self.result = EvaluationResult(
            checkpoint_path=self.checkpoint_path,
            seed=int(self.config.state_config().get("seed", 42)) if hasattr(self.config, 'state_config') else 42,
            steps=int(self.config.state_config().get("steps", 0)) if hasattr(self.config, 'state_config') else 0,
            config=self.config,
            param_count=sum(p.numel() for p in self.model.parameters()),
        )

    def teacher_forced_evaluate(self, sentences: list[str], partition_name: str) -> dict[str, Any]:
        total_nll = 0.0
        total_correct = 0
        total_targets = 0
        per_sentence = []

        for sentence in sentences:
            result = evaluate_sentence(self.model, sentence)
            total_nll += result["nll"]
            total_correct += int(result["accuracy"] * result["num_targets"])
            total_targets += result["num_targets"]
            per_sentence.append(result)

        accuracy = total_correct / total_targets if total_targets > 0 else 0.0
        nll = total_nll / total_targets if total_targets > 0 else float("inf")

        return {
            "nll": nll,
            "accuracy": accuracy,
            "target_count": total_targets,
            "sentences": len(sentences),
            "per_sentence": per_sentence,
        }

    def generate_and_evaluate(self, sentences: list[str]) -> list[dict[str, Any]]:
        generations = []
        for sentence in sentences:
            parts = sentence.split()
            if len(parts) < 3:
                continue
            prefix = f"The {parts[1]} {parts[2]}"
            generated = generate_from_prefix(self.model, prefix, require_eos=True)
            action = parts[3] if len(parts) >= 4 else ""
            action_in_gen = action in generated.lower()
            exact_match = generated.strip() == sentence.strip()
            generations.append({
                "sentence": sentence,
                "prefix": prefix,
                "generated_text": generated,
                "expected_action": action,
                "action_in_generation": action_in_gen,
                "exact_sentence_match": exact_match,
            })
        return generations

    def evaluate(self) -> EvaluationResult:
        self.result.train = self.teacher_forced_evaluate(self.train_sentences, "train")
        self.result.dev = self.teacher_forced_evaluate(self.dev_sentences, "dev")
        self.result.heldout = self.teacher_forced_evaluate(self.heldout_sentences, "heldout")
        self.result.training_generations = self.generate_and_evaluate(self.train_sentences)
        self.result.heldout_generations = self.generate_and_evaluate(self.heldout_sentences)
        self.result.train_per_sentence = self.result.train["per_sentence"]
        self.result.dev_per_sentence = self.result.dev["per_sentence"]
        self.result.heldout_per_sentence = self.result.heldout["per_sentence"]
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
        lines.append(f"## Checkpoint: `{self.checkpoint_path}`\n")
        lines.append(f"- Seed: `{self.result.seed}`\n")
        lines.append(f"- Steps: `{self.result.steps}`\n")
        lines.append(f"- Parameters: `{self.result.param_count:,}`\n")
        lines.append("## Teacher-Forced Metrics\n")
        lines.append("| Partition | NLL | Accuracy | Target Count | Sentences |\n")
        lines.append("|-----------|-----|----------|--------------|-----------|\n")
        lines.append(f"| Train | {self.result.train['nll']:.6f} | {self.result.train['accuracy']:.4%} | {self.result.train['target_count']} | {self.result.train['sentences']} |\n")
        lines.append(f"| Dev | {self.result.dev['nll']:.6f} | {self.result.dev['accuracy']:.4%} | {self.result.dev['target_count']} | {self.result.dev['sentences']} |\n")
        lines.append(f"| Held-out | {self.result.heldout['nll']:.6f} | {self.result.heldout['accuracy']:.4%} | {self.result.heldout['target_count']} | {self.result.heldout['sentences']} |\n")
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
