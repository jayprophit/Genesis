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

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

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
    expected_suffix: str = ""
    expected_action: str = ""
    exact_match: bool = False
    action_accuracy: bool = False


@dataclass
class PartitionMetrics:
    """Metrics for a single partition (train/dev/heldout)."""

    nll: float
    accuracy: float
    target_count: int
    correct_count: int
    sentence_count: int
    sentence_results: list[dict[str, Any]] = field(default_factory=list)
    # Partition status: "APPLICABLE" for non-empty partitions,
    # "NOT_APPLICABLE" for explicitly empty partitions (e.g. historical Level 3 dev partition).
    status: str = "APPLICABLE"

    def to_dict(self) -> dict[str, Any]:
        """Convert to dictionary for JSON serialization, including status."""
        return {
            "nll": self.nll,
            "accuracy": self.accuracy,
            "target_count": self.target_count,
            "correct_count": self.correct_count,
            "sentence_count": self.sentence_count,
            "sentence_results": self.sentence_results,
            "status": self.status,
        }


@dataclass
class EvaluationResult:
    """Container for full evaluation results."""

    checkpoint_path: str
    checkpoint_sha256: str
    seed: int | None
    steps: int | None
    param_count: int
    context_limit: int
    train: PartitionMetrics
    dev: PartitionMetrics
    heldout: PartitionMetrics
    training_generations: list[GenerationResult]
    heldout_generations: list[GenerationResult]
    heldout_exact_matches: int = 0
    heldout_total: int = 0
    heldout_status: str = "NOT_EVALUATED"
    # Authorization timestamp (UTC) recorded when a final test (final_heldout=True)
    # succeeds. None if no final test has been performed.
    authorization_timestamp: str | None = None

    # Experiment identity recorded from explicit protocol selection (not from
    # default examples). Values: "Level3" (12 train / 0 dev / 4 held-out),
    # "Level3Dv2" (24 train / 6 dev / 6 held-out), or a callable-provided label.
    experiment_identity: str = "Unknown"
    # Manifest fingerprint (SHA256 of the manifest file that defines the
    # partitions). None if no manifest was provided.
    manifest_fingerprint: str | None = None
    # Manifest path: the actual manifest file path used to compute the
    # fingerprint. None if no manifest was provided.
    manifest_path: str | Path | None = None
    # Checkpoint training-set compatibility status: "VERIFIED" if checkpoint
    # metadata explicitly identifies the training partition (e.g. seed, dataset
    # name) and it is compatible with the selected experiment; "UNVERIFIED" if
    # metadata is unavailable. Never silently assume compatibility.
    checkpoint_training_compatibility: str = "UNVERIFIED"
    # Manifest schema adapter indicator: True if the manifest was adapted
    # (e.g. Level3Dv2 uses 'held_out' which was normalized to 'heldout').
    manifest_schema_adapter_applied: bool = False

    def to_dict(self) -> dict[str, Any]:
        """Convert to dictionary for JSON serialization."""
        return {
            "checkpoint_path": self.checkpoint_path,
            "checkpoint_sha256": self.checkpoint_sha256,
            "seed": self.seed,
            "steps": self.steps,
            "param_count": self.param_count,
            "context_limit": self.context_limit,
            "heldout_status": self.heldout_status,
            "authorization_timestamp": self.authorization_timestamp,
            "experiment_identity": self.experiment_identity,
            "manifest_fingerprint": self.manifest_fingerprint,
            "checkpoint_training_compatibility": self.checkpoint_training_compatibility,
            "manifest_schema_adapter_applied": self.manifest_schema_adapter_applied,
            "train": self.train.to_dict(),
            "dev": self.dev.to_dict(),
            "heldout": self.heldout.to_dict(),
            "training_generations": [
                {
                    "exact_prompt": g.exact_prompt,
                    "prompt_token_ids": g.prompt_token_ids,
                    "generated_token_ids": g.generated_token_ids,
                    "raw_decoded_suffix": g.raw_decoded_suffix,
                    "reconstructed_complete": g.reconstructed_complete,
                    "expected_suffix": g.expected_suffix,
                    "exact_match": g.exact_match,
                    "action_accuracy": g.action_accuracy,
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
                    "expected_suffix": g.expected_suffix,
                    "exact_match": g.exact_match,
                    "action_accuracy": g.action_accuracy,
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
    expected_suffix: str = "",
    expected_action: str = "",
) -> GenerationResult:
    """Generate from prefix using a linear-confidence model.

    Args:
        model: Untrained model for vocab size
        prefix: Exact input prompt string
        logit_fn: Function that takes token tensor and returns logits tensor [1, seq, vocab]
        max_new_tokens: Maximum new tokens to generate
        context_limit: Maximum context window (e.g., 128)
        expected_suffix: The exact bytes that the generated suffix should match
        expected_action: The expected action word (for action_accuracy comparison)

    Returns:
        GenerationResult with full metadata including EOS detection
    """
    prompt_token_ids = [BOS] + encode(prefix)
    x_gen = torch.tensor([prompt_token_ids], dtype=torch.long)

    generated_token_ids = []
    eos_detected = False
    stop_reason = "token_limit"

    with torch.inference_mode():
        for step in range(max_new_tokens):
            # Check context before inference
            if x_gen.shape[1] >= context_limit:
                stop_reason = "context_limit"
                break

            logits = logit_fn(x_gen)
            next_token = logits[0, -1].argmax().item()
            generated_token_ids.append(next_token)
            x_gen = torch.cat([x_gen, torch.tensor([[next_token]], dtype=torch.long)], dim=1)

            if next_token == EOS:
                eos_detected = True
                stop_reason = "eos"
                break

    # Decode generated suffix (only newly generated tokens)
    raw_suffix_tokens = generated_token_ids
    raw_decoded_suffix = decode(raw_suffix_tokens)
    # Reconstructed complete: prompt exactly once + generated suffix
    reconstructed_complete = decode(prompt_token_ids + generated_token_ids)

    # Exact match: expected suffix bytes match generated suffix AND EOS is correct
    # Generate token IDs for expected suffix + EOS
    expected_suffix_ids = encode(expected_suffix) + [EOS] if expected_suffix else [EOS]
    exact_match = (generated_token_ids == expected_suffix_ids) and eos_detected

    # Action accuracy: parse generated suffix according to declared grammar.
    # Independent of punctuation and EOS correctness; full-sentence exact matching
    # remains byte-exact. Use only the generated tokens (excluding EOS) to decode
    # the suffix, avoiding the EOS (257) byte being decoded as a non-whitespace
    # character that would incorrectly concatenate words (e.g. 'dog' + 'runs' +
    # 'ā' = 'dogrunsā', breaking word counts).
    action_accuracy = False
    if expected_action:
        generated_without_eos = [t for t in generated_token_ids if t != EOS]
        decoded_suffix = decode(generated_without_eos) if generated_without_eos else ""
        # Action is the first word of the generated suffix
        stripped = decoded_suffix.lstrip(" ")
        action_word = stripped.split()[0] if stripped else ""
        # Remove trailing punctuation from the action word (e.g. 'runs.' -> 'runs')
        while action_word and action_word[-1] in ".,!?;:\"'":
            action_word = action_word[:-1]
        action_accuracy = action_word == expected_action

    return GenerationResult(
        exact_prompt=prefix,
        prompt_token_ids=list(prompt_token_ids),
        generated_token_ids=list(generated_token_ids),
        raw_decoded_suffix=raw_decoded_suffix,
        reconstructed_complete=reconstructed_complete,
        expected_suffix=expected_suffix,
        expected_action=expected_action,
        exact_match=exact_match,
        action_accuracy=action_accuracy,
        eos_detected=eos_detected,
        stop_reason=stop_reason,
        num_generated_tokens=len(generated_token_ids),
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
        experiment_identity: str = "Level3D",
        manifest_fingerprint: str | None = None,
        checkpoint_training_compatibility: str = "UNVERIFIED",
        manifest_path: str | Path | None = None,
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

        # Schema adapter flag: False by default; set to True when Level3Dv2
        # manifest (held_out key) is normalized to heldout schema.
        self.manifest_schema_adapter_applied = False

        # Validate dataset partitions for consistency
        self._validate_partitions()

        self.result = EvaluationResult(
            checkpoint_path=str(self.checkpoint_path),
            checkpoint_sha256=self.checkpoint_sha256,
            seed=self.seed,
            steps=self.steps,
            param_count=self.param_count,
            context_limit=self.config.seed_config.context,
            train=PartitionMetrics(nll=0.0, accuracy=0.0, target_count=0, correct_count=0, sentence_count=0),
            dev=PartitionMetrics(nll=0.0, accuracy=0.0, target_count=0, correct_count=0, sentence_count=0),
            heldout=PartitionMetrics(nll=0.0, accuracy=0.0, target_count=0, correct_count=0, sentence_count=0),
            training_generations=[],
            heldout_generations=[],
            heldout_exact_matches=0,
            heldout_total=0,
            experiment_identity=experiment_identity,
            manifest_fingerprint=manifest_fingerprint,
            checkpoint_training_compatibility=checkpoint_training_compatibility,
            heldout_status="NOT_EVALUATED",
            manifest_path=manifest_path,
        )

        # Store manifest path as an instance attribute for access by evaluate()
        self.manifest_path = manifest_path

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

    def _validate_partitions(self) -> None:
        """Enforce explicit dataset partition validation.

        Validates:
        - train/dev/held-out disjointness
        - declared grammar (all sentences match the template)
        - dataset identity (all sentences use the same structure)
        - complete partition coverage (partition counts are positive)

        Raises:
            ValueError: if any partition check fails.
        """
        train = self.train_sentences
        dev = self.dev_sentences
        heldout = self.heldout_sentences

        # Check disjointness
        train_set = set(train)
        dev_set = set(dev)
        heldout_set = set(heldout)

        if train_set & dev_set:
            raise ValueError("Train and dev partitions overlap")
        if train_set & heldout_set:
            raise ValueError("Train and held-out partitions overlap")
        if dev_set & heldout_set:
            raise ValueError("Dev and held-out partitions overlap")

        # Check grammar: all sentences must match "The {color} {animal} {action}."
        import re
        grammar_pattern = re.compile(r"^The \w+ \w+ \w+\.$")
        for partition_name, partition in [
            ("train", train),
            ("dev", dev),
            ("heldout", heldout),
        ]:
            for sentence in partition:
                if not grammar_pattern.match(sentence):
                    raise ValueError(
                        f"Invalid grammar in {partition_name} partition: {sentence!r}"
                    )

        # Check complete partition coverage (positive counts)
        # Train partition must not be empty
        if len(train) == 0:
            raise ValueError("Train partition must not be empty")
        # Dev partition is allowed to be empty (e.g. Level 3 historical:
        # 12 train / 0 dev / 4 held-out)
        if len(heldout) == 0:
            raise ValueError("Held-out partition must not be empty")

    def evaluate_partition(
        self,
        sentences: list[str],
        partition_name: str,
    ) -> PartitionMetrics:
        total_nll = 0.0
        total_correct = 0
        total_targets = 0
        sentence_results = []
        status = "APPLICABLE"

        for sentence in sentences:
            enc = [b for b in sentence.encode("utf-8")]
            seq = [BOS] + enc + [EOS]
            x, y = build_shifted_targets(seq)

            # Enforce context boundary: reject sequences exceeding model context window
            if len(seq) > self.config.seed_config.context:
                raise ValueError(
                    f"Sequence in {partition_name} partition exceeds context window: "
                    f"{len(seq)} bytes > {self.config.seed_config.context}"
                )

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

        # Handle empty partition
        if total_targets == 0:
            status = "NOT_APPLICABLE"
            # Return null values, never inf or 0.0
            return PartitionMetrics(
                nll=None,
                accuracy=None,
                target_count=0,
                correct_count=0,
                sentence_count=0,
                sentence_results=[],
                status="NOT_APPLICABLE",
            )

        return PartitionMetrics(
            nll=total_nll / total_targets,
            accuracy=total_correct / total_targets,
            target_count=total_targets,
            correct_count=total_correct,
            sentence_count=len(sentences),
            sentence_results=sentence_results,
            status=status,
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
            # Parse the sentence using the declared grammar:
            # "The {color} {animal} {action_word}."
            # Determine the distinguishing prefix (color + animal) so each
            # candidate sentence maps to a unique prompt for generation.
            prefix = f"The {parts[1]} {parts[2]}"
            # Extract the action word from the final token WITHOUT punctuation.
            # The sentence's final token includes the trailing period (e.g.
            # "runs."), so strip it before using the action word.
            action_word = parts[3].rstrip(".").rstrip("!").rstrip("?").rstrip(",")
            # Derive expected_suffix from the exact original sentence after the
            # distinguishing prefix, preserving the leading space and final period.
            # Do NOT duplicate punctuation (e.g. " runs." + "." would produce
            # " runs.."). The action word already excludes punctuation, so the
            # period is appended here once.
            expected_suffix = f" {action_word}."
            expected_action = action_word
            gen_result = generate_from_linear(
                self.model,
                prefix,
                logit_fn=self._model_logits,
                max_new_tokens=128,
                context_limit=self.config.seed_config.context,
                expected_suffix=expected_suffix,
                expected_action=expected_action,
            )
            generations.append(gen_result)
        return generations

    def _model_logits(self, x: torch.Tensor) -> torch.Tensor:
        """Use the loaded model directly for production evaluation.

        Uses torch.inference_mode() to ensure no gradient tracking and no
        modification of model weights. This is the production inference
        pathway — no random logits, no manual seed reset.
        """
        with torch.inference_mode():
            return self.model(x)

    def _linear_logits(self, x: torch.Tensor) -> torch.Tensor:
        """Deliberately broken — removed from production use.

        This method is retained ONLY for backwards compatibility with unit tests
        that require deterministic mock logits. The production evaluator MUST
        not call this method. Use _model_logits instead.
        """
        batch_size, seq_len = x.shape
        torch.manual_seed(42 + int(x.sum().item()))
        return torch.randn(batch_size, seq_len, 262, device=x.device)

    def report_json(self, path: str | Path) -> str:
        """Write the evaluation result as JSON to the given path.

        Args:
            path: File path to write the JSON report to.

        Returns:
            The JSON string written.
        """
        import json
        from pathlib import Path as _Path
        path = _Path(path)
        data = self.result.to_dict()
        json_str = json.dumps(data, indent=2, allow_nan=False, sort_keys=False)
        path.write_text(json_str, encoding="utf-8")
        return json_str

    def report_markdown(self, path: str | Path) -> str:
        """Write the evaluation result as Markdown to the given path.

        Args:
            path: File path to write the Markdown report to.

        Returns:
            The Markdown string written.
        """
        from pathlib import Path as _Path
        path = _Path(path)
        data = self.result.to_dict()

        lines = [
            "# GenesisSeed Evaluation Report",
            "",
            f"**Checkpoint:** `{data['checkpoint_path']}`",
            f"**Checkpoint SHA256:** `{data['checkpoint_sha256']}`",
            f"**Seed:** `{data['seed']}`",
            f"**Steps:** `{data['steps']}`",
            f"**Param count:** `{data['param_count']}`",
            f"**Context limit:** `{data['context_limit']}`",
            f"**Experiment identity:** `{data['experiment_identity']}`",
            f"**Manifest fingerprint:** `{data['manifest_fingerprint']}`",
            f"**Checkpoint training compatibility:** `{data['checkpoint_training_compatibility']}`",
            "",
            "## Partition Metrics",
            "",
            "### Train",
            "",
            f"| Metric | Value |",
            "|---|---|",
            f"| Sentence count | `{data['train']['sentence_count']}` |",
            f"| Target count | `{data['train']['target_count']}` |",
            f"| Correct count | `{data['train']['correct_count']}` |",
            f"| NLL | `{data['train']['nll']}` |",
            f"| Accuracy | `{data['train']['accuracy']}` |",
            "",
            "### Dev",
            "",
            f"| Metric | Value |",
            "|---|---|",
            f"| Sentence count | `{data['dev']['sentence_count']}` |",
            f"| Target count | `{data['dev']['target_count']}` |",
            f"| Correct count | `{data['dev']['correct_count']}` |",
            f"| NLL | `{data['dev']['nll']}` |",
            f"| Accuracy | `{data['dev']['accuracy']}` |",
            "",
            "### Held-out",
            "",
            f"| Metric | Value |",
            "|---|---|",
            f"| Sentence count | `{data['heldout']['sentence_count']}` |",
            f"| Target count | `{data['heldout']['target_count']}` |",
            f"| Correct count | `{data['heldout']['correct_count']}` |",
            f"| NLL | `{data['heldout']['nll']}` |",
            f"| Accuracy | `{data['heldout']['accuracy']}` |",
            f"| Status | `{data['heldout_status']}` |",
            "",
            "## Generations",
            "",
        ]

        for i, gen in enumerate(data['training_generations'], 1):
            lines.extend([
                f"### Generation {i}",
                "",
                f"- **Exact prompt:** `{gen['exact_prompt']}`",
                f"- **Expected suffix:** `{gen['expected_suffix']}`",
                f"- **Exact match:** `{gen['exact_match']}`",
                f"- **Action accuracy:** `{gen['action_accuracy']}`",
                f"- **EOS detected:** `{gen['eos_detected']}`",
                f"- **Stop reason:** `{gen['stop_reason']}`",
                f"- **Num generated tokens:** `{gen['num_generated_tokens']}`",
                f"- **Generated token IDs:** `{gen['generated_token_ids']}`",
                f"- **Raw decoded suffix:** `{gen['raw_decoded_suffix']}`",
                f"- **Reconstructed complete:** `{gen['reconstructed_complete']}`",
                "",
            ])

        path.write_text("\n".join(lines), encoding="utf-8")
        return "\n".join(lines)
    def evaluate(
        self,
        final_heldout: bool = False,
        manifest_path: str | Path | None = None,
        experiment_identity: str = "Level3D",
        checkpoint_training_compatibility: str = "UNVERIFIED",
    ) -> EvaluationResult:
        """Run full evaluation.

        Args:
            final_heldout: If True, evaluate the final held-out set (authorized
                final test). If False (default), held-out data is NOT evaluated
                through teacher-forced evaluation or generation -- it is reported
                as NOT_EVALUATED to prevent silent invalid comparisons.
            manifest_path: Path to the dataset manifest file used to define the
                canonical benchmark run. If provided, the manifest is parsed as
                JSON, validated against the declared experiment identity and
                partition counts, and its SHA256 is computed from the validated
                bytes. If omitted, the constructor-provided manifest_path is
                used.
            experiment_identity: Explicit experiment protocol identifier:
                "Level3" (12/0/4), "Level3Dv2" (24/6/6), or a callable-provided label.
            checkpoint_training_compatibility: Labeled after validating that the
                checkpoint's training-sentence metadata matches the selected
                training partition. If the checkpoint lacks sufficient metadata,
                this must remain "UNVERIFIED".

        Returns:
            EvaluationResult with partition metrics, generation metadata, and
            experiment identity. Unmeasured held-out results are reported as
            null (None), never 0.0, when final_heldout=False.
        """
        # Validate experiment identity and partition integrity
        self.experiment_identity = experiment_identity
        if manifest_path is None:
            manifest_path = self.manifest_path
        if manifest_path is not None:
            # Convert to Path object if needed
            if isinstance(manifest_path, str):
                manifest_path = Path(manifest_path)
            if not manifest_path.exists():
                raise FileNotFoundError(f"Manifest not found: {manifest_path}")
            self._validate_manifest(manifest_path)
            # Compute SHA256 of actual manifest bytes and store in result
            import hashlib
            sha256 = hashlib.sha256()
            with open(manifest_path, "rb") as f:
                for chunk in iter(lambda: f.read(8192), b""):
                    sha256.update(chunk)
            self.result.manifest_fingerprint = sha256.hexdigest()
            self.result.manifest_path = str(manifest_path)
            # Sync manifest_schema_adapter_applied from evaluator to result
            self.result.manifest_schema_adapter_applied = self.manifest_schema_adapter_applied

        # Validate partition disjointness and grammar
        self._validate_partitions()

        # Check for repeated final_heldout=True on the same evaluator instance
        # Once a final_heldout=True evaluation succeeds, reject any later
        # final_heldout=True call on the same instance, regardless of
        # whether ordinary (final_heldout=False) evaluations occur between them.
        if final_heldout and getattr(self, "_final_heldout_done", False):
            raise ValueError(
                "final_heldout=True already evaluated on this evaluator instance. "
                "Final-test authorization is per-instance only to prevent accidental repetition."
            )
        # Compute checkpoint training-sentence metadata and compare with
        # the selected training partition.
        self._verify_checkpoint_training_compatibility()
        # Also update the result's compatibility status (not just the instance attribute)
        self.result.checkpoint_training_compatibility = self.checkpoint_training_compatibility

        # Train and dev partitions (dev may be empty for Level 3 protocol)
        self.result.train = self.evaluate_partition(self.train_sentences, "train")
        self.result.dev = self.evaluate_partition(self.dev_sentences, "dev")

        # Final held-out authorization:
        # - Default (final_heldout=False) reports NOT_EVALUATED and never calls
        #   evaluate_partition or evaluate_generations against held-out data.
        self.result.heldout = PartitionMetrics(
            nll=None, accuracy=None, target_count=0, correct_count=0,
            sentence_count=0, sentence_results=[], status="NOT_APPLICABLE"
        )
        self.result.heldout_generations = []
        self.result.heldout_exact_matches = 0
        self.result.heldout_total = 0
        self.result.heldout_status = "NOT_EVALUATED"

        # Only set the per-instance authorization flag AFTER the held-out
        # evaluation has successfully completed. A failed final evaluation
        # must not consume authorization.
        if final_heldout:
            self.result.heldout_status = "EVALUATED"
            self.result.heldout = self.evaluate_partition(
                self.heldout_sentences, "heldout"
            )
            self.result.heldout_generations = self.evaluate_generations(
                self.heldout_sentences, "heldout"
            )
            self.result.heldout_exact_matches = sum(
                1 for g in self.result.heldout_generations if g.exact_match
            )
            self.result.heldout_total = len(self.result.heldout_generations)

            # Record actual UTC authorization timestamp (ISO 8601)
            from datetime import datetime, timezone
            self.result.authorization_timestamp = datetime.now(timezone.utc).isoformat()

            # Per-instance authorization: only set the flag once the final
            # evaluation has successfully completed (not before)
            self._final_heldout_done = True
        else:
            self.result.heldout_status = "NOT_EVALUATED"
            # Unmeasured results: null, never 0.0
            self.result.heldout.nll = None
            self.result.heldout.accuracy = None
            self.result.heldout.correct_count = 0
            self.result.heldout.target_count = 0

        # Training generations
        self.result.training_generations = self.evaluate_generations(
            self.train_sentences, "train"
        )
        return self.result

    def _validate_manifest(self, manifest_path: Path) -> None:
        """Parse and validate the dataset manifest JSON.

        Checks:
        - Manifest is valid JSON (reject invalid or nonexistent).
        - Schema matches expected structure with train/dev/heldout partitions.
        - Declared experiment identity and expected partition counts match.
        - Partitions match the actual evaluation inputs (train/dev/heldout lists).
        - SHA256 is computed from the validated manifest bytes.

        Raises:
            ValueError: If manifest is invalid, missing, has wrong schema,
                or partition mismatch.
        """
        if not manifest_path.exists():
            raise ValueError(f"Manifest does not exist: {manifest_path}")

        # Parse manifest as JSON
        import json
        try:
            with open(manifest_path, "r", encoding="utf-8") as f:
                manifest = json.load(f)
        except (json.JSONDecodeError, UnicodeDecodeError) as e:
            raise ValueError(f"Invalid JSON in manifest {manifest_path}: {e}")

        # Validate manifest structure: must be a dict
        if not isinstance(manifest, dict):
            raise ValueError("Manifest must be a JSON object")

        # Reject manifests specifying both keys ambiguously
        if "held_out" in manifest and "heldout" in manifest:
            raise ValueError(
                "Manifest specifies both 'held_out' and 'heldout' keys; "
                "ambiguous dataset schema. Use exactly one."
            )

        # Schema normalization: Level3Dv2 uses 'held_out', normalize to 'heldout'
        # BEFORE checking required partition fields
        if "held_out" in manifest:
            manifest["heldout"] = manifest.pop("held_out")
            self.manifest_schema_adapter_applied = True

        # Now validate required partition fields
        for partition in ("train", "dev", "heldout"):
            if partition not in manifest:
                raise ValueError(f"Manifest missing required partition: {partition}")
            if not isinstance(manifest[partition], list):
                raise ValueError(f"Manifest partition '{partition}' must be an array")

        # Verify partition counts against expected totals
        expected_counts = {
            "Level3": (12, 0, 4),
            "Level3Dv2": (24, 6, 6),
        }
        if self.experiment_identity in expected_counts:
            exp_train, exp_dev, exp_heldout = expected_counts[self.experiment_identity]
            actual_train = len(manifest["train"])
            actual_dev = len(manifest["dev"])
            actual_heldout = len(manifest["heldout"])
            if (actual_train, actual_dev, actual_heldout) != (exp_train, exp_dev, exp_heldout):
                raise ValueError(
                    f"Manifest partition counts ({actual_train}/{actual_dev}/{actual_heldout}) do not match "
                    f"expected for '{self.experiment_identity}' ({exp_train}/{exp_dev}/{exp_heldout})"
                )

        # Normalize manifest lists to canonical sentence strings
        manifest_train = self._normalize_manifest_list(manifest["train"])
        manifest_dev = self._normalize_manifest_list(manifest["dev"])
        manifest_heldout = self._normalize_manifest_list(manifest["heldout"])

        # Check no overlap within manifest
        if manifest_train & manifest_dev:
            overlap = manifest_train & manifest_dev
            raise ValueError(f"Train/dev overlap detected: {overlap}")
        if manifest_train & manifest_heldout:
            overlap = manifest_train & manifest_heldout
            raise ValueError(f"Train/heldout overlap detected: {overlap}")

        # Check manifest partitions match the evaluator's partitions
        if manifest_train != set(self.train_sentences):
            raise ValueError("Manifest train partition does not match evaluator train partition")
        if manifest_dev != set(self.dev_sentences):
            raise ValueError("Manifest dev partition does not match evaluator dev partition")
        if manifest_heldout != set(self.heldout_sentences):
            raise ValueError("Manifest held-out partition does not match evaluator held-out partition")

    def _normalize_manifest_list(self, items: list[Any]) -> set[str]:
        """Normalize manifest list items to canonical sentence strings."""
        result = set()
        for item in items:
            if isinstance(item, str):
                result.add(item)
            elif isinstance(item, (list, tuple)):
                result.add(" ".join(str(x) for x in item))
            else:
                result.add(str(item))
        return result

    def _verify_checkpoint_training_compatibility(self) -> None:
        """Compare checkpoint training-sentence metadata with the selected
        training partition. Derives status only from actual checkpoint metadata.

        Uses three distinct outcomes:
        - VERIFIED: checkpoint metadata exactly identifies the selected training
          partition (no shared tokens used as evidence alone).
        - MISMATCH: checkpoint metadata identifies a different training partition.
        - UNVERIFIED: checkpoint metadata is missing or insufficient to verify.

        Raises ValueError if compatibility is VERIFIED but the selected
        checkpoint is known-invalid (e.g. metadata identifies a different
        dataset than the one used for evaluation).
        """
        self.checkpoint_training_compatibility = "UNVERIFIED"

        # Read checkpoint metadata
        saved = None
        try:
            import torch
            saved = torch.load(self.checkpoint_path, map_location="cpu", weights_only=True)
        except Exception:
            self.checkpoint_training_compatibility = "UNVERIFIED"
            return

        # Extract training-partition metadata from checkpoint
        cmp_data = None
        for key in ("train_sentences", "train_partition", "dataset_partition", "partition"):
            if key in saved:
                cmp_data = saved[key]
                break

        if cmp_data is None:
            self.checkpoint_training_compatibility = "UNVERIFIED"
            return

        # Normalize checkpoint training metadata to a set of canonical sentences
        checkpoint_train: set[str] = set()
        if isinstance(cmp_data, (list, tuple)):
            for item in cmp_data:
                if isinstance(item, (list, tuple)):
                    checkpoint_train.add(" ".join(str(x) for x in item))
                else:
                    checkpoint_train.add(str(item))
        elif isinstance(cmp_data, dict):
            # Also check for dataset name or explicit partition size
            dataset_name = str(cmp_data.get("name", ""))
            if dataset_name:
                checkpoint_train.add(dataset_name)
            for v in cmp_data.values():
                if isinstance(v, (list, tuple)):
                    for item in v:
                        if isinstance(item, (list, tuple)):
                            checkpoint_train.add(" ".join(str(x) for x in item))
                        else:
                            checkpoint_train.add(str(item))
                else:
                    checkpoint_train.add(str(v))
        else:
            checkpoint_train.add(str(cmp_data))

        # Normalize selected training partition to a set of canonical sentences
        selected_train: set[str] = set()
        for s in self.train_sentences:
            selected_train.add(s)

        # Require EXACT equivalence: every selected training sentence must be
        # matched by a checkpoint training entry AND every checkpoint training
        # entry must be in the selected partition. One shared sentence, word,
        # or token is NOT sufficient.
        if checkpoint_train and selected_train:
            if checkpoint_train == selected_train:
                self.checkpoint_training_compatibility = "VERIFIED"
            else:
                self.checkpoint_training_compatibility = "MISMATCH"
        elif not checkpoint_train and selected_train:
            self.checkpoint_training_compatibility = "UNVERIFIED"
        else:
            self.checkpoint_training_compatibility = "VERIFIED"

        # Reject a verified mismatch before producing a canonical benchmark report.
        if self.checkpoint_training_compatibility == "MISMATCH":
            raise ValueError(
                "Checkpoint training-partition metadata does not match the "
                f"selected training partition. Checkpoint: {self.checkpoint_path}"
            )
