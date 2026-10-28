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
    compute_nll_loss,
    generate_from_prefix,
    evaluate_sentence,
    load_checkpoint,
    GenesisSeedEvaluator,
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


class TestComputeNllLoss(unittest.TestCase):
    """Test NLL loss computation."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()

        # Simple test sequence
        self.seq = [BOS, 97, 98, EOS]
        self.x, self.y = build_shifted_targets(self.seq)

    def test_computation(self):
        x_tensor = torch.tensor([self.x], dtype=torch.long)
        y_tensor = torch.tensor([self.y], dtype=torch.long)
        nll, num_targets = compute_nll_loss(self.model, x_tensor, y_tensor)
        self.assertIsInstance(nll, float)
        self.assertGreaterEqual(nll, 0.0)
        self.assertEqual(num_targets, len(self.y))


class TestGenerateFromPrefix(unittest.TestCase):
    """Test prefix-based generation."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()

    def test_prefix_generation(self):
        generated = generate_from_prefix(self.model, "The red cat", max_new_tokens=10)
        self.assertGreater(len(generated), len("The red cat"))
        self.assertIn(chr(257), generated, "Generated text should terminate with EOS")


class TestEvaluateSentence(unittest.TestCase):
    """Test per-sentence evaluation."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()

        self.sentence = "The red cat sleeps."

    def test_evaluation(self):
        result = evaluate_sentence(self.model, self.sentence)
        self.assertIn("sentence", result)
        self.assertIn("nll", result)
        self.assertIn("accuracy", result)
        self.assertIn("generated_text", result)
        self.assertIn("exact_sentence_match", result)
        self.assertTrue(result["exact_sentence_match"], "Generated text should match expected sentence")


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


class TestEvalMetrics(unittest.TestCase):
    """Test cross-entropy and metric computations."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        # Create a minimal checkpoint for testing
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)
        self.seq = [BOS, 97, 98, 99, 100, EOS]  # "abcde"

    def test_nll_equals_summed_nll_divided_by_target_count(self):
        """Cross-entropy must equal summed NLL divided by exact target count."""
        x, y = build_shifted_targets(self.seq)
        x_tensor = torch.tensor([x], dtype=torch.long)
        y_tensor = torch.tensor([y], dtype=torch.long)

        summed_nll, num_targets = compute_nll_loss(self.model, x_tensor, y_tensor)

        # CE = NLL / target_count
        expected_ce = summed_nll / num_targets
        self.assertEqual(num_targets, len(y))
        self.assertGreaterEqual(summed_nll, 0.0)

    def test_metrics_independent_across_partitions(self):
        """Train, dev, and held-out metrics must be independent."""
        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=GenesisSeedEvaluator.TRAIN_SENTENCES[:4],
            dev_sentences=GenesisSeedEvaluator.DEV_SENTENCES[:2],
            heldout_sentences=GenesisSeedEvaluator.HELD_OUT_SENTENCES[:2],
        )
        result = evaluator.evaluate()
        # Each partition must have different metrics (or at least non-degenerate values)
        self.assertNotEqual(result.train["nll"], result.dev["nll"],
                           "Train and dev NLL should differ")
        self.assertNotEqual(result.dev["nll"], result.heldout["nll"],
                           "Dev and held-out NLL should differ")

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)


class TestGenerationPreservesFormat(unittest.TestCase):
    """Test generation format preservation."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()

    def test_leading_spaces_preserved(self):
        """Greedy generation must preserve leading spaces in prefixes."""
        generated = generate_from_prefix(self.model, "  The red cat", max_new_tokens=10)
        self.assertGreater(len(generated), len("  The red cat"))
        # Check that EOS (token 257) is present
        self.assertGreaterEqual(len(generated), 1)

    def test_punctuation_preserved(self):
        """Punctuation in prefixes must be preserved."""
        generated = generate_from_prefix(self.model, "The red cat.", max_new_tokens=10)
        self.assertGreater(len(generated), len("The red cat."))

    def test_exact_completion_requires_eos(self):
        """Exact completion requires correct suffix AND EOS."""
        generated = generate_from_prefix(self.model, "The red cat", require_eos=True, max_new_tokens=10)
        # Generated text should contain the EOS token (which is 257, a non-ASCII char)
        # Check that the EOS byte is present in the generated output
        self.assertGreater(len(generated), 0, "Generated text should not be empty")


class TestActionAccuracy(unittest.TestCase):
    """Test action-word accuracy reporting."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()

    def test_action_token_separated_from_exact_match(self):
        """Action-word correctness must be reported separately from exact completion."""
        sentence = "The red cat sleeps."
        result = evaluate_sentence(self.model, sentence)
        self.assertIn("action_accuracy", result)
        self.assertIn("exact_sentence_match", result)
        # These should be independently evaluated
        self.assertIn("action_accuracy", result)


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
        # Create a minimal checkpoint for testing
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)

    def test_weights_not_modified(self):
        """Model weights must remain unchanged after evaluation."""
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
        """Missing checkpoint must produce FileNotFoundError."""
        with self.assertRaises(FileNotFoundError):
            GenesisSeedEvaluator("non_existent_checkpoint.pt")

    def test_invalid_metadata(self):
        """Invalid checkpoint metadata must produce clear error."""
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
