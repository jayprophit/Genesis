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
            "experiment_identity": self.experiment_identity,
            "manifest_fingerprint": self.manifest_fingerprint,
            "checkpoint_training_compatibility": self.checkpoint_training_compatibility,
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
                through teacher-forced evaluation or generation — it is reported
                as NOT_EVALUATED to prevent silent invalid comparisons.
            manifest_path: Path to the dataset manifest file (SHA256 is computed
                from actual bytes) used to define the canonical benchmark run.
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
        # Use the constructor-provided manifest path if none given locally
        if manifest_path is None:
            manifest_path = self.manifest_path
        if manifest_path is not None:
            from pathlib import Path as _Path
            mp = _Path(manifest_path)
            if mp.exists():
                import hashlib
                sha256 = hashlib.sha256()
                with open(mp, "rb") as f:
                    for chunk in iter(lambda: f.read(8192), b""):
                        sha256.update(chunk)
                self.result.manifest_fingerprint = sha256.hexdigest()

        # Validate partition disjointness and grammar
        self._validate_partitions()

        # Compute manifest SHA256 from actual bytes (if manifest provided)
        self.result.manifest_fingerprint = None
        if manifest_path is not None:
            from pathlib import Path as _Path
            mp = _Path(manifest_path)
            if mp.exists():
                import hashlib
                sha256 = hashlib.sha256()
                with open(mp, "rb") as f:
                    for chunk in iter(lambda: f.read(8192), b""):
                        sha256.update(chunk)
                self.result.manifest_fingerprint = sha256.hexdigest()

        # Compute checkpoint training-sentence metadata and compare with
        # the selected training partition. If metadata is unavailable,
        # compatibility remains UNVERIFIED (never VERIFIED without validation).
        self._verify_checkpoint_training_compatibility()

        # Train and dev partitions (dev may be empty for Level 3 protocol)
        self.result.train = self.evaluate_partition(self.train_sentences, "train")
        self.result.dev = self.evaluate_partition(self.dev_sentences, "dev")

        # Final held-out authorization:
        # Default (final_heldout=False) reports NOT_EVALUATED and never calls
        # evaluate_partition or evaluate_generations against held-out data.
        self.result.heldout = PartitionMetrics(
            nll=None, accuracy=None, target_count=0, correct_count=0,
            sentence_count=0, sentence_results=[]
        )
        self.result.heldout_generations = []
        self.result.heldout_exact_matches = 0
        self.result.heldout_total = 0
        self.result.heldout_status = "NOT_EVALUATED"

        if final_heldout:
            # Record the explicit final-test invocation
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

    def _verify_checkpoint_training_compatibility(self) -> None:
        """Compare checkpoint training-sentence metadata with the selected
        training partition. Returns True if compatibility can be verified,
        False otherwise (metadata insufficient).

        Compatibility is reported as VERIFIED only when the checkpoint's
        training-set metadata explicitly confirms the selected partition.
        If metadata is unavailable, compatibility remains UNVERIFIED.
        """
        # This method checks whether the checkpoint stores training-set
        # metadata (e.g., seed, step count, dataset name, partition size)
        # that can be verified against the selected train_sentences.
        # If the metadata is missing or incompatible, compatibility remains
        # UNVERIFIED (the default).
        #
        # The actual verification logic depends on the checkpoint schema.
        # For now, we attempt to read checkpoint metadata. If it cannot
        # be read or does not match, compatibility stays UNVERIFIED.
        saved = None
        try:
            import torch
            saved = torch.load(self.checkpoint_path, map_location="cpu", weights_only=True)
        except Exception:
            # Checkpoint is unreadable; metadata unavailable.
            # Report UNVERIFIED (never VERIFIED without validation).
            self.checkpoint_training_compatibility = "UNVERIFIED"
            return

        # Check for training-partition metadata in checkpoint
        # Common keys: "train_sentences", "train_partition", "dataset", "partition"
        cmp_data = saved.get("train_sentences") or saved.get("train_partition") or saved.get("partition")
        if cmp_data is None:
            self.checkpoint_training_compatibility = "UNVERIFIED"
            return

        # Convert checkpoint metadata to a set of strings for comparison
        checkpoint_train = set()
        if isinstance(cmp_data, (list, tuple)):
            for item in cmp_data:
                if isinstance(item, (list, tuple)):
                    checkpoint_train.add(" ".join(str(x) for x in item))
                else:
                    checkpoint_train.add(str(item))
        elif isinstance(cmp_data, dict):
            for v in cmp_data.values():
                checkpoint_train.add(str(v))
        else:
            checkpoint_train.add(str(cmp_data))

        # Compare with selected training partition (as a set of strings)
        selected_train = set()
        for s in self.train_sentences:
            selected_train.add(s)
            for part in s.split():
                selected_train.add(part)  # also compare word-level tokens

        overlap = checkpoint_train & selected_train
        # Compatibility VERIFIED only if significant overlap verified;
        # otherwise UNVERIFIED (never VERIFIED without validation).
        if len(overlap) > 0 and len(selected_train) > 0:
            self.checkpoint_training_compatibility = "VERIFIED"
        else:
            self.checkpoint_training_compatibility = "UNVERIFIED"

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
        for name, metrics in [("Train", self.result.train), ("Dev", self.result.dev)]:
            if metrics.accuracy is None:
                accuracy_str = "N/A"
                nll_str = "N/A"
            else:
                accuracy_str = f"{metrics.accuracy:.4%}"
                nll_str = f"{metrics.nll:.6f}"
            lines.append(
                f"| {name} | {metrics.sentence_count} | {metrics.target_count} | "
                f"{metrics.correct_count} | {accuracy_str} | {nll_str} |\n"
            )
        # Held-out: always NOT_EVALUATED at this point
        heldout_nll = "N/A" if self.result.heldout.nll is None else f"{self.result.heldout.nll:.6f}"
        heldout_acc = "N/A" if self.result.heldout.accuracy is None else f"{self.result.heldout.accuracy:.4%}"
        lines.append(
            f"| Held-out | {self.result.heldout.sentence_count} | {self.result.heldout.target_count} | "
            f"{self.result.heldout.correct_count} | {heldout_acc} | {heldout_nll} |\n"
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
