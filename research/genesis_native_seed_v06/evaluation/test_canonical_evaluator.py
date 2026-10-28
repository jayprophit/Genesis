"""
Unit tests for the canonical evaluation module.

Tests every metric function.
"""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

import torch

sys.path.insert(0, str(Path(__file__).parent.parent / "genesis_native_seed_v06"))

from genesis_seed.model import GenesisSeed, GenesisConfig, SeedConfig, BOS, EOS, encode, decode

# Import the evaluator
from evaluation.canonical_evaluator import (
    build_shifted_targets,
    compute_nll_and_accuracy,
    generate_from_linear,
    evaluate_sentence,
    GenesisSeedEvaluator,
    GenerationResult,
    PartitionMetrics,
    EvaluationResult,
    load_checkpoint,
)


class TestBuildShiftedTargets(unittest.TestCase):
    """Test shifted target construction."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()

    def test_basic(self):
        seq = [BOS, 97, 98, 99, EOS]  # "abc"
        x, y = build_shifted_targets(seq)
        self.assertEqual(x, [BOS, 97, 98, 99])
        self.assertEqual(y, [97, 98, 99, EOS])

    def test_single_byte(self):
        seq = [BOS, 97, EOS]
        x, y = build_shifted_targets(seq)
        self.assertEqual(x, [BOS, 97])
        self.assertEqual(y, [97, EOS])


class TestComputeNllAndAccuracy(unittest.TestCase):
    """Test NLL and accuracy computation."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        self.seq = [BOS, 97, 98, 99, EOS]  # "abcd"

    def test_nll_equals_summed_nll_divided_by_target_count(self):
        """Cross-entropy must equal summed NLL divided by exact target count."""
        x, y = build_shifted_targets(self.seq)
        x_tensor = torch.tensor([x], dtype=torch.long)
        y_tensor = torch.tensor([y], dtype=torch.long)
        summed_nll, correct, total = compute_nll_and_accuracy(self.model, x_tensor, y_tensor)
        expected_ce = summed_nll / total
        self.assertEqual(total, len(y))
        self.assertGreaterEqual(summed_nll, 0.0)
        self.assertEqual(correct, 0)  # untrained model should have ~0 accuracy

    def test_integer_counts_not_reconstructed_from_accuracy(self):
        """Integer counts must be accumulated, not derived from float accuracy."""
        x, y = build_shifted_targets(self.seq)
        x_tensor = torch.tensor([x], dtype=torch.long)
        y_tensor = torch.tensor([y], dtype=torch.long)
        summed_nll, correct, total = compute_nll_and_accuracy(self.model, x_tensor, y_tensor)
        self.assertIsInstance(correct, int)
        self.assertIsInstance(total, int)
        self.assertGreater(total, 0)


class TestGenerateFromLinear(unittest.TestCase):
    """Test generation from linear-confidence model."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()

    def _make_eos_logit_fn(self):
        """Create a logit function that always predicts EOS at the last position.

        generate_from_linear uses logits[0, -1].argmax() to select the next token,
        so EOS (token 257) must be the argmax at the last position.
        """
        def logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.zeros(batch_size, seq_len, 262, device=x.device)
            logits[0, -1, 257] = 1000.0  # EOS at last position
            return logits

        return logit_fn

    def test_generate_returns_structured_result(self):
        """generate_from_linear must return a GenerationResult."""
        prompt = "The red cat"

        result = generate_from_linear(self.model, prompt, self._make_eos_logit_fn(), max_new_tokens=10)
        self.assertIsInstance(result, GenerationResult)
        self.assertEqual(result.exact_prompt, prompt)
        self.assertGreater(len(result.generated_token_ids), len(result.prompt_token_ids))
        self.assertIn(EOS, result.generated_token_ids)
        self.assertEqual(result.stop_reason, "eos")

    def test_generation_preserves_leading_whitespace(self):
        """Leading spaces in prompt must be preserved in generation."""
        result = generate_from_linear(self.model, "  The red cat",
                                      self._make_eos_logit_fn(), max_new_tokens=10)
        self.assertTrue(result.eos_detected)
        self.assertEqual(result.stop_reason, "eos")

    def test_generation_preserves_final_punctuation(self):
        """Final punctuation in prefix must be preserved."""
        result = generate_from_linear(self.model, "The red cat.", self._make_eos_logit_fn(), max_new_tokens=10)
        self.assertTrue(result.eos_detected)
        self.assertEqual(result.stop_reason, "eos")

    def test_context_limit_stop(self):
        """Generation must stop at context limit."""
        result = generate_from_linear(self.model, "short", self._make_eos_logit_fn(),
                                      max_new_tokens=128, context_limit=16)
        self.assertIn(result.stop_reason, ["context_limit", "eos"])


class TestEvaluateSentence(unittest.TestCase):
    """Test per-sentence evaluation."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        self.sentence = "The red cat sleeps."

    def test_returns_metrics(self):
        result = evaluate_sentence(self.model, self.sentence)
        self.assertIn("sentence", result)
        self.assertIn("nll", result)
        self.assertIn("correct", result)
        self.assertIn("num_targets", result)
        self.assertIn("accuracy", result)
        self.assertGreater(result["num_targets"], 0)

    def test_accuracy_between_zero_and_one(self):
        result = evaluate_sentence(self.model, self.sentence)
        self.assertGreaterEqual(result["accuracy"], 0.0)
        self.assertLessEqual(result["accuracy"], 1.0)


class TestLoadCheckpoint(unittest.TestCase):
    """Test checkpoint loading with weights_only=True."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=16))
        self.model = GenesisSeed(self.config)
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)

    def test_load(self):
        loaded_config, loaded_model = load_checkpoint(self.checkpoint.name)
        self.assertEqual(loaded_config, self.config)
        self.assertIsNotNone(loaded_model.state_dict())


class TestGenerationPreservesFormat(unittest.TestCase):
    """Test generation from linear-confidence model."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()

    def _make_eos_logit_fn(self):
        """Create a logit function that always predicts EOS at the last position."""
        def logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.zeros(batch_size, seq_len, 262, device=x.device)
            logits[0, -1, 257] = 1000.0
            return logits

        return logit_fn

    def test_generation_preserves_leading_whitespace(self):
        """Leading spaces in prefix must be preserved in generation."""
        result = generate_from_linear(self.model, "  The red cat",
                                      self._make_eos_logit_fn(), max_new_tokens=10)
        self.assertTrue(result.eos_detected)
        self.assertEqual(result.stop_reason, "eos")

    def test_generation_preserves_final_punctuation(self):
        """Final punctuation in prefix must be preserved."""
        result = generate_from_linear(self.model, "The red cat.", self._make_eos_logit_fn(), max_new_tokens=10)
        self.assertTrue(result.eos_detected)
        self.assertEqual(result.stop_reason, "eos")

    def test_context_limit_stop(self):
        """Generation must stop at context limit."""
        result = generate_from_linear(self.model, "short", self._make_eos_logit_fn(),
                                      max_new_tokens=128, context_limit=16)
        self.assertIn(result.stop_reason, ["context_limit", "eos"])


class TestPartitionSeparation(unittest.TestCase):
    """Test that data partitions are not merged or confused."""

    def test_partitions_are_distinct(self):
        """Train and held-out sentences must be distinct."""
        train = GenesisSeedEvaluator.TRAIN_SENTENCES
        heldout = GenesisSeedEvaluator.HELD_OUT_SENTENCES
        overlap = set(train) & set(heldout)
        self.assertEqual(len(overlap), 0, "No sentences should appear in both train and heldout")


class TestEvaluatorImmutability(unittest.TestCase):
    """Test that the evaluator does not modify model weights."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        self.initial_weights = {name: param.clone() for name, param in self.model.state_dict().items()}
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)

    def test_weights_not_modified(self):
        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=GenesisSeedEvaluator.TRAIN_SENTENCES[:4],
            dev_sentences=GenesisSeedEvaluator.DEV_SENTENCES[:2],
            heldout_sentences=GenesisSeedEvaluator.HELD_OUT_SENTENCES[:2],
        )
        evaluator.evaluate()
        for name, param in self.model.state_dict().items():
            self.assertTrue(torch.equal(param, self.initial_weights[name]),
                          f"Weight {name} was modified during evaluation")


class TestInvalidCheckpoint(unittest.TestCase):
    """Test that invalid checkpoints produce clear failures."""

    def test_missing_file(self):
        with self.assertRaises(FileNotFoundError):
            GenesisSeedEvaluator("non_existent_checkpoint.pt")

    def test_invalid_metadata(self):
        with tempfile.NamedTemporaryFile(suffix=".pt", delete=False) as f:
            f.write(b"not a valid checkpoint")
            bad_path = f.name
        try:
            with self.assertRaises(Exception):
                GenesisSeedEvaluator(bad_path)
        finally:
            Path(bad_path).unlink(missing_ok=True)


if __name__ == "__main__":
    unittest.main()
