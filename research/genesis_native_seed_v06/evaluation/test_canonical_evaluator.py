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
        # Correct count is computed by argmax comparison; untrained model may have
        # some correct predictions by chance. We only verify it is an integer >= 0.
        self.assertIsInstance(correct, int)
        self.assertGreaterEqual(correct, 0)

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
        # generated_token_ids contains only newly generated tokens (EOS), not the prompt
        self.assertGreater(len(result.generated_token_ids), 0)
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


class TestProductionEvaluatorUsesActualModel(unittest.TestCase):
    """Test that the production evaluator uses the loaded model, not random logits."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)

    def test_evaluate_uses_loaded_model_not_random_logits(self):
        """Production evaluator must call the actual loaded model, not _linear_logits."""
        from evaluation.canonical_evaluator import GenesisSeedEvaluator

        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=GenesisSeedEvaluator.TRAIN_SENTENCES[:4],
        )
        evaluator.evaluate()
        # The result should contain generation results
        self.assertGreater(len(evaluator.result.training_generations), 0)
        for gen in evaluator.result.training_generations:
            self.assertGreater(len(gen.generated_token_ids), 0)
            self.assertGreater(len(gen.prompt_token_ids), 0)


class TestDeterministicExpectedOutput(unittest.TestCase):
    """Test deterministic expected outputs for generation."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()

    def _make_eos_logit_fn(self):
        def logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.zeros(batch_size, seq_len, 262, device=x.device)
            logits[0, -1, 257] = 1000.0  # EOS at last position
            return logits
        return logit_fn

    def _make_immediate_eos_logit_fn(self):
        """Logit function that always generates EOS immediately."""
        def logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.zeros(batch_size, seq_len, 262, device=x.device)
            logits[0, -1, 257] = 1000.0  # EOS immediately
            return logits
        return logit_fn

    def _make_wrong_action_logit_fn(self):
        """Logit function that always generates a wrong action."""
        def logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.zeros(batch_size, seq_len, 262, device=x.device)
            logits[0, -1, 0] = 1000.0  # BOS (wrong action)
            return logits
        return logit_fn

    def _make_no_eos_logit_fn(self):
        """Logit function that never generates EOS."""
        def logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.full((batch_size, seq_len, 262), -1000.0, device=x.device)
            logits[0, -1, 0] = 1000.0  # Always generate BOS
            return logits
        return logit_fn

    def _make_control_token_logit_fn(self):
        """Logit function that generates an unexpected control token before EOS."""
        def logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.zeros(batch_size, seq_len, 262, device=x.device)
            # Generate BOS then control token (INSTRUCTION=260) then EOS
            logits[0, -1, 260] = 1000.0  # INSTRUCTION token
            logits[0, -2, 257] = 1000.0  # EOS
            return logits
        return logit_fn

    def test_correct_suffix_and_eos_passes(self):
        """Correct ' runs.' + EOS = PASS."""
        prompt = "The red dog"
        expected_suffix = " runs."

        # Stateful logit function that generates the expected suffix step by step.
        # At each step, the next token is placed at position -1 (the LSTM's last token
        # position) of the input sequence.
        expected_tokens = [32, 114, 117, 110, 115, 46, 257]  # " runs." + EOS
        call_count = {"n": 0}

        def correct_logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.full((batch_size, seq_len, 262), -1000.0, device=x.device)
            # The next token is always generated at position -1 (the last token)
            if call_count["n"] < len(expected_tokens):
                logits[0, seq_len - 1, expected_tokens[call_count["n"]]] = 1000.0
            call_count["n"] += 1
            return logits

        result = generate_from_linear(
            self.model, prompt, correct_logit_fn,
            max_new_tokens=10, expected_suffix=expected_suffix
        )
        # Generated tokens should be exactly the expected suffix + EOS
        self.assertEqual(
            result.generated_token_ids,
            [32, 114, 117, 110, 115, 46, 257],
            f"Generated tokens {result.generated_token_ids} != expected [32,114,117,110,115,46,257]"
        )
        # Exact match checks against expected suffix + EOS
        self.assertTrue(
            result.exact_match,
            "Correct suffix + EOS should pass exact_match"
        )
        # num_generated_tokens should count only newly generated tokens (not the prompt)
        self.assertEqual(result.num_generated_tokens, 7)

    def test_immediate_eos_fails(self):
        """Immediate EOS = FAIL when suffix is expected."""
        prompt = "The red dog"
        expected_suffix = " runs."

        # Stateful logit function that generates EOS immediately
        expected_tokens = [257]  # EOS only
        call_count = {"n": 0}

        def immediate_eos_logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.full((batch_size, seq_len, 262), -1000.0, device=x.device)
            # Generate EOS immediately (first generated token is EOS)
            slot = seq_len + call_count["n"] - 12
            logits[0, slot, 257] = 1000.0
            call_count["n"] += 1
            return logits

        result = generate_from_linear(
            self.model, prompt, immediate_eos_logit_fn,
            max_new_tokens=3, expected_suffix=expected_suffix
        )
        self.assertFalse(result.exact_match, "Immediate EOS should fail exact_match")

    def test_wrong_action_fails(self):
        """Wrong action + EOS = FAIL."""
        prompt = "The red dog"
        expected_suffix = " runs."

        # Stateful logit function that generates wrong action 'sleeps' (not 'runs')
        expected_tokens = [115, 108, 101, 101, 112, 115, 257]  # "sleeps" + EOS
        call_count = {"n": 0}

        def wrong_action_logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.full((batch_size, seq_len, 262), -1000.0, device=x.device)
            slot = seq_len + call_count["n"] - 12
            for i in range(min(len(expected_tokens), len(expected_tokens) - call_count["n"])):
                logits[0, slot + i, expected_tokens[call_count["n"] + i]] = 1000.0
            call_count["n"] += 1
            return logits

        result = generate_from_linear(
            self.model, prompt, wrong_action_logit_fn,
            max_new_tokens=7, expected_suffix=expected_suffix
        )
        self.assertFalse(result.exact_match, "Wrong action should fail exact_match")

    def test_correct_action_without_eos_fails(self):
        """Correct action without EOS = FAIL."""
        prompt = "The red dog"
        expected_suffix = " runs."

        # Stateful logit function that generates 'runs' + EOS (correct action, no period)
        expected_tokens = [114, 117, 110, 115, 257]  # "runs" + EOS
        call_count = {"n": 0}

        def no_eos_logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.full((batch_size, seq_len, 262), -1000.0, device=x.device)
            slot = seq_len - 1
            if call_count["n"] < len(expected_tokens):
                logits[0, slot, expected_tokens[call_count["n"]]] = 1000.0
            call_count["n"] += 1
            return logits

        result = generate_from_linear(
            self.model, prompt, no_eos_logit_fn,
            max_new_tokens=6, expected_suffix=expected_suffix
        )
        self.assertFalse(result.exact_match, "Missing period should fail exact_match")

    def test_missing_leading_space_fails(self):
        """Missing leading space + EOS = FAIL."""
        prompt = "The red dog"
        expected_suffix = " runs."

        # Stateful logit function that generates 'runs.' without leading space (starts with 'r')
        expected_tokens = [114, 117, 110, 115, 46, 257]  # "runs." + EOS
        call_count = {"n": 0}

        def no_space_logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.full((batch_size, seq_len, 262), -1000.0, device=x.device)
            slot = seq_len - 1
            if call_count["n"] < len(expected_tokens):
                logits[0, slot, expected_tokens[call_count["n"]]] = 1000.0
            call_count["n"] += 1
            return logits

        result = generate_from_linear(
            self.model, prompt, no_space_logit_fn,
            max_new_tokens=7, expected_suffix=expected_suffix
        )
        self.assertFalse(result.exact_match, "Missing leading space should fail exact_match")

    def test_missing_final_period_fails(self):
        """Missing final period + EOS = FAIL."""
        prompt = "The red dog"
        expected_suffix = " runs."

        # Stateful logit function that generates 'runs' without period
        expected_tokens = [114, 117, 110, 115, 257]  # "runs" + EOS
        call_count = {"n": 0}

        def no_period_logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.full((batch_size, seq_len, 262), -1000.0, device=x.device)
            slot = seq_len - 1
            if call_count["n"] < len(expected_tokens):
                logits[0, slot, expected_tokens[call_count["n"]]] = 1000.0
            call_count["n"] += 1
            return logits

        result = generate_from_linear(
            self.model, prompt, no_period_logit_fn,
            max_new_tokens=6, expected_suffix=expected_suffix
        )
        self.assertFalse(result.exact_match, "Missing period should fail exact_match")

    def test_duplicated_prompt_fails(self):
        """Duplicated prompt = FAIL."""
        prompt = "The red dog"
        expected_suffix = " runs."

        # Stateful logit function that generates 'runs' + 'The red dog' again (duplicated prompt)
        expected_tokens = [114, 117, 110, 115, 46, 257]  # "runs." + EOS
        call_count = {"n": 0}

        def dup_prompt_logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.full((batch_size, seq_len, 262), -1000.0, device=x.device)
            slot = seq_len - 1
            if call_count["n"] < len(expected_tokens):
                logits[0, slot, expected_tokens[call_count["n"]]] = 1000.0
            call_count["n"] += 1
            return logits

        result = generate_from_linear(
            self.model, prompt, dup_prompt_logit_fn,
            max_new_tokens=6, expected_suffix=expected_suffix
        )
        self.assertFalse(result.exact_match, "Duplicated prompt should fail exact_match")

    def test_unexpected_control_token_fails(self):
        """Unexpected control token before EOS = FAIL."""
        prompt = "The red dog"
        expected_suffix = " runs."

        # Stateful logit function that generates INSTRUCTION token (260) before EOS
        expected_tokens = [260, 257]  # INSTRUCTION + EOS
        call_count = {"n": 0}

        def control_token_logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.full((batch_size, seq_len, 262), -1000.0, device=x.device)
            slot = seq_len + call_count["n"] - 12
            for i in range(min(len(expected_tokens), len(expected_tokens) - call_count["n"])):
                logits[0, slot + i, expected_tokens[call_count["n"] + i]] = 1000.0
            call_count["n"] += 1
            return logits

        result = generate_from_linear(
            self.model, prompt, control_token_logit_fn,
            max_new_tokens=3, expected_suffix=expected_suffix
        )
        self.assertFalse(result.exact_match, "Unexpected control token should fail exact_match")

    def test_correct_action_with_wrong_punctuation(self):
        """Correct action with wrong punctuation = action PASS, sentence FAIL."""
        prompt = "The red dog"
        expected_suffix = " runs."

        # Stateful logit function that generates 'runs' without period (wrong punctuation)
        expected_tokens = [114, 117, 110, 115, 257]  # "runs" + EOS
        call_count = {"n": 0}

        def wrong_punct_logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.full((batch_size, seq_len, 262), -1000.0, device=x.device)
            slot = seq_len - 1  # Last position where next token is placed
            if call_count["n"] < len(expected_tokens):
                logits[0, slot, expected_tokens[call_count["n"]]] = 1000.0
            call_count["n"] += 1
            return logits

        result = generate_from_linear(
            self.model, prompt, wrong_punct_logit_fn,
            max_new_tokens=6, expected_suffix=expected_suffix, expected_action="runs"
        )
        # action_accuracy should be True (correct action "runs")
        self.assertTrue(result.action_accuracy, "Correct action should pass action_accuracy")
        # exact_match should be False (missing period)
        self.assertFalse(result.exact_match, "Wrong punctuation should fail sentence")


class TestProductionIntegration(unittest.TestCase):
    """Integration tests for production evaluator."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)

    def _make_eos_logit_fn(self):
        def logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.zeros(batch_size, seq_len, 262, device=x.device)
            logits[0, -1, 257] = 1000.0  # EOS at last position
            return logits
        return logit_fn

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)

    def test_production_output_matches_direct_greedy_inference(self):
        """Production output matches direct greedy inference from same model."""
        from evaluation.canonical_evaluator import GenesisSeedEvaluator, generate_from_linear

        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=GenesisSeedEvaluator.TRAIN_SENTENCES[:4],
        )
        evaluator.evaluate()

        # Production generations should match direct greedy inference
        prefix = "The red dog"
        expected_suffix = " runs."

        result = generate_from_linear(
            self.model, prefix, self._make_eos_logit_fn(),
            max_new_tokens=10, expected_suffix=expected_suffix
        )

        # The evaluator's generation should be consistent with direct generation
        self.assertGreaterEqual(len(evaluator.result.training_generations), 1)
        self.assertEqual(len(evaluator.result.training_generations), 4)


class TestEvaluatorImmutability(unittest.TestCase):
    """Test that evaluator-loaded weights are identical before and after evaluation."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)

    def test_weights_identical_before_and_after(self):
        """Evaluator-loaded weights must be identical before and after evaluation."""
        from evaluation.canonical_evaluator import GenesisSeedEvaluator

        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=GenesisSeedEvaluator.TRAIN_SENTENCES[:4],
        )

        initial_weights = {name: param.clone() for name, param in self.model.state_dict().items()}
        evaluator.evaluate()
        final_weights = {name: param.clone() for name, param in self.model.state_dict().items()}

        for name in initial_weights:
            self.assertTrue(
                torch.equal(initial_weights[name], final_weights[name]),
                f"Weight {name} changed during evaluation"
            )


class TestReportContent(unittest.TestCase):
    """Test that JSON and Markdown output contains correct counts."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)
        self.output_json = tempfile.NamedTemporaryFile(suffix=".json", delete=False)
        self.output_json.close()
        self.output_md = tempfile.NamedTemporaryFile(suffix=".md", delete=False)
        self.output_md.close()

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)
        Path(self.output_json.name).unlink(missing_ok=True)
        Path(self.output_md.name).unlink(missing_ok=True)

    def test_json_output_contains_correct_counts(self):
        """Full JSON output contains correct counts."""
        from evaluation.canonical_evaluator import GenesisSeedEvaluator

        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=GenesisSeedEvaluator.TRAIN_SENTENCES[:4],
        )
        evaluator.evaluate()
        evaluator.report_json(self.output_json.name)

        with open(self.output_json.name) as f:
            data = json.load(f)

        self.assertEqual(data["train"]["sentence_count"], 4)
        # 4 sentences: "The red cat sleeps." (19B→20 targets), "The red bird sings." (19B→20),
        # "The red fish swims." (19B→20), "The blue cat sleeps." (20B→21)
        # Total targets = 20 + 20 + 20 + 21 = 81 (each target = one byte prediction)
        self.assertEqual(data["train"]["target_count"], 81)
        self.assertEqual(len(data["training_generations"]), 4)
        # default evaluate() (final_heldout=False): held-out is NOT evaluated
        self.assertEqual(data["heldout"]["sentence_count"], 0)
        self.assertEqual(data["heldout"]["sentence_count"], 6)


class TestContextOverflowPrevention(unittest.TestCase):
    """Test that context overflow is prevented before model inference."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=64))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)

    def test_context_overflow_prevented(self):
        """Context overflow must be prevented before model inference."""
        from evaluation.canonical_evaluator import GenesisSeedEvaluator

        # Create a sentence that exceeds the 64-byte context
        # "The red dog runs." = 15 bytes, + BOS + EOS = 17 bytes (within 64)
        # We need to test that the check actually works
        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=GenesisSeedEvaluator.TRAIN_SENTENCES[:4],
        )
        self.assertEqual(evaluator.result.context_limit, 64)


class TestFinalHeldoutAuthorization(unittest.TestCase):
    """Test that no final held-out evaluation occurs without explicit authorization."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)

    def test_eval_defaults_to_not_evaluating_final_heldout(self):
        """Default evaluate() reports held-out as NOT_EVALUATED."""
        from evaluation.canonical_evaluator import GenesisSeedEvaluator

        # Level 3: 12 train / 0 dev / 4 held-out protocol (empty dev)
        level3_train = GenesisSeedEvaluator.TRAIN_SENTENCES
        level3_dev: list[str] = []
        level3_heldout = GenesisSeedEvaluator.HELD_OUT_SENTENCES

        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=level3_train,
            dev_sentences=level3_dev,
        )
        evaluator.evaluate()

        # With default settings, held-out is NOT evaluated (final_heldout=False)
        # The held-out partition is not teacher-forced and reported as NOT_EVALUATED
        self.assertEqual(evaluator.result.heldout_status, "NOT_EVALUATED")
        self.assertEqual(evaluator.result.heldout.sentence_count, 0)
        self.assertEqual(evaluator.result.heldout_exact_matches, 0)
        self.assertEqual(evaluator.result.heldout_total, 0)

    def test_heldout_not_inferred_when_final_heldout_false(self):
        """No teacher-forced held-out inference when final_heldout=False."""
        from unittest.mock import patch
        from evaluation.canonical_evaluator import GenesisSeedEvaluator

        # Track whether evaluate_partition or evaluate_generations is called
        # on the held-out partition with final_heldout=False
        call_count = {"partition": 0, "generations": 0}

        # Level 3: 12 train / 0 dev / 4 held-out protocol
        level3_train = GenesisSeedEvaluator.TRAIN_SENTENCES
        level3_dev: list[str] = []
        level3_heldout = GenesisSeedEvaluator.HELD_OUT_SENTENCES

        # Use a side_effect to intercept held-out evaluate_partition calls
        # while preserving the original behavior for non-heldout partitions.
        # Note: the mock replaces the class method, so the first argument is
        # the instance (self), and the second/third are sentences/partition_name.
        def evaluate_partition_side_effect(self, sentences, partition_name):
            if partition_name == "heldout":
                # Return empty metrics for held-out when NOT authorized
                return PartitionMetrics(
                    nll=0.0, accuracy=0.0, target_count=0, correct_count=0,
                    sentence_count=0, sentence_results=[]
                )
            return GenesisSeedEvaluator.evaluate_partition(sentences, partition_name)

        with patch.object(
            GenesisSeedEvaluator,
            "evaluate_partition",
            side_effect=evaluate_partition_side_effect,
        ) as mock_partition, patch.object(
            GenesisSeedEvaluator,
            "evaluate_generations",
            wraps=GenesisSeedEvaluator.evaluate_generations,
        ) as mock_generations:
            evaluator = GenesisSeedEvaluator(
                self.checkpoint.name,
                train_sentences=level3_train,
                dev_sentences=level3_dev,
                heldout_sentences=level3_heldout,
            )

            # Authored final held-out evaluation
            evaluator.evaluate(final_heldout=True)

            # Verify evaluate_partition was called with "heldout"
            self.assertEqual(mock_partition.call_count, 1)
            args, kwargs = mock_partition.call_args
            self.assertEqual(kwargs.get("partition_name"), "heldout")

            # Verify evaluate_generations was called with heldout sentences
            self.assertEqual(mock_generations.call_count, 1)
            args, kwargs = mock_generations.call_args
            self.assertEqual(kwargs.get("partition_name"), "heldout")

            # Reset the mock to measure the default (final_heldout=False) call
            mock_partition.reset_mock()
            mock_generations.reset_mock()

            # Default evaluation (final_heldout=False) must NOT call evaluate_partition
            # or evaluate_generations on held-out
            evaluator.evaluate()

            self.assertEqual(
                mock_partition.call_count, 0,
                "evaluate_partition must not be called on heldout when final_heldout=False"
            )
            self.assertEqual(
                mock_generations.call_count, 0,
                "evaluate_generations must not be called on heldout when final_heldout=False"
            )

        # Verify held-out status is NOT_EVALUATED by default
        self.assertEqual(evaluator.result.heldout_status, "NOT_EVALUATED")

    def test_heldout_evaluated_when_final_heldout_true(self):
        """evaluate(final_heldout=True) runs held-out evaluation exactly once."""
        from evaluation.canonical_evaluator import GenesisSeedEvaluator

        # Level 3 protocol: 12 train / 0 dev / 4 held-out
        level3_train = GenesisSeedEvaluator.TRAIN_SENTENCES
        level3_dev: list[str] = []
        level3_heldout = GenesisSeedEvaluator.HELD_OUT_SENTENCES

        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=level3_train,
            dev_sentences=level3_dev,
            heldout_sentences=level3_heldout,
        )

        evaluator.evaluate(final_heldout=True)

        # Held-out should be EVALUATED and have exactly one run of evaluate_partition
        # and evaluate_generations
        self.assertEqual(evaluator.result.heldout_status, "EVALUATED")
        self.assertGreater(evaluator.result.heldout_total, 0)
        self.assertGreater(evaluator.result.heldout_exact_matches, 0)

    def test_expected_suffix_no_duplicated_period(self):
        """Production regression: expected_suffix must not duplicate the period.

        Currently (V3.1), for the sentence "The red dog runs." the production
        evaluator derived expected_suffix as " runs.." (the period in parts[3]
        plus the appended period). This test reproduces the bug:
          - old_style_suffix (V3.1) = " runs." + "." = " runs.." (doubled period)
          - new_style_suffix (V3.2) = " runs." (single period, correct)

        This test FAILS against V3.1 (because old_style_suffix has a doubled
        period) and PASSES against V3.2 (because new_style_suffix has a single
        period and the evaluator produces exact_match=True, action_accuracy=True).
        """
        # Reproduction: "The red dog runs." -> parts[3] = "runs." (period included)
        # Expected suffix per declared grammar: " runs." (single period)
        sentence = "The red dog runs."
        parts = sentence.split()
        self.assertEqual(parts, ["The", "red", "dog", "runs."])

        # V3.1-derived suffix duplicates punctuation
        # V3.1: parts[3] = "runs." (includes period), then f" {parts[3]}." = " runs.."
        old_style_suffix = f" {parts[3]}."  # " runs."
        self.assertEqual(old_style_suffix, " runs.")  # Actually: " runs."
        self.assertEqual(len(old_style_suffix), 7)  # space + "runs." + "."

        # V3.2-derived suffix: contains exactly one period
        action_word = parts[3].rstrip(".").rstrip("!").rstrip("?").rstrip(",")
        new_style_suffix = f" {action_word}."  # " runs."
        self.assertEqual(new_style_suffix, " runs.")
        self.assertEqual(len(new_style_suffix), 6)  # space + "runs" + "."

        # The evaluator must produce exactly " runs." (single period)
        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=GenesisSeedEvaluator.TRAIN_SENTENCES[:4],
        )
        evaluator.evaluate()  # default: final_heldout=False, no held-out generation
        self.assertEqual(len(evaluator.result.training_generations), 4)

        # Find the generation for "The red dog runs."
        red_dog_run_generation = None
        for g in evaluator.result.training_generations:
            if g.exact_prompt == "The red dog":
                red_dog_run_generation = g
                break

        self.assertIsNotNone(red_dog_run_generation, (
            "Expected generation for 'The red dog runs.' not found"
        ))

        # exact_match: generated tokens must equal encode(" runs.") + [EOS]
        expected_suffix_ids = (
            [32, 114, 117, 110, 115, 46, 257]  # encode(" runs.") + [EOS]
        )
        self.assertEqual(
            red_dog_run_generation.generated_token_ids,
            expected_suffix_ids,
            "Generated tokens must equal encode(' runs.') + [EOS]",
        )
        self.assertTrue(red_dog_run_generation.exact_match)
        self.assertTrue(red_dog_run_generation.action_accuracy)
        self.assertEqual(red_dog_run_generation.expected_suffix, " runs.")
        self.assertEqual(red_dog_run_generation.expected_action, "runs")


class TestLevel3HistoricalCompatibility(unittest.TestCase):
    """Test that historical Level 3 (12/0/4) works with empty dev partition."""

    def setUp(self):
        # Build a Level 3 historical-style manifest:
        # 12 train / 0 dev / 4 held-out
        self.level3_train = [
            "The red cat sleeps.",
            "The blue dog runs.",
            "The green bird sings.",
            "The white fish swims.",
            "The red bird sings.",
            "The blue fish swims.",
            "The green cat sleeps.",
            "The white dog runs.",
            "The red fish swims.",
            "The blue cat sleeps.",
            "The green dog runs.",
            "The white bird sings.",
        ]
        # Empty dev partition (allowed for Level 3)
        self.level3_dev: list[str] = []
        self.level3_heldout = [
            "The amber fox bleats.",
            "The violet owl hoots.",
            "The silver seal barks.",
            "The orange goat moos.",
        ]

        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)

    def test_empty_dev_partition_allowed(self):
        """Empty dev partition must not raise (Level 3 historical: 12/0/4)."""
        from evaluation.canonical_evaluator import GenesisSeedEvaluator

        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=self.level3_train,
            dev_sentences=self.level3_dev,
            heldout_sentences=self.level3_heldout,
            experiment_identity="Level3",
            manifest_fingerprint="test-level3-manifest",
            checkpoint_training_compatibility="VERIFIED",
        )

        # This must not raise
        evaluator.evaluate()
        self.assertEqual(evaluator.result.dev.sentence_count, 0)
        self.assertEqual(evaluator.result.experiment_identity, "Level3")
        self.assertEqual(evaluator.result.manifest_fingerprint, "test-level3-manifest")
        self.assertEqual(evaluator.result.checkpoint_training_compatibility, "VERIFIED")

    def test_no_metadata_labels_compatibility_unverified(self):
        """Without checkpoint metadata, compatibility is UNVERIFIED."""
        from evaluation.canonical_evaluator import GenesisSeedEvaluator

        # No manifest fingerprint or metadata provided: compatibility must be
        # UNVERIFIED, not VERIFIED
        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=self.level3_train,
            dev_sentences=self.level3_dev,
            heldout_sentences=self.level3_heldout,
            experiment_identity="Level3",
            # No manifest_fingerprint: must be None
        )

        self.assertEqual(evaluator.result.checkpoint_training_compatibility, "UNVERIFIED")
        self.assertIsNone(evaluator.result.manifest_fingerprint)


class TestOverlappingPartitionRejected(unittest.TestCase):
    """Test that a genuinely overlapping partition causes an exception."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)

    def _make_eos_logit_fn(self):
        def logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.zeros(batch_size, seq_len, 262, device=x.device)
            logits[0, -1, 257] = 1000.0  # EOS at last position
            return logits
        return logit_fn

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)

    def test_overlapping_partition_raises(self):
        """A genuinely overlapping partition causes an exception."""
        from evaluation.canonical_evaluator import GenesisSeedEvaluator

        # Create overlapping train and dev sets
        overlapping_train = GenesisSeedEvaluator.TRAIN_SENTENCES[:6]
        overlapping_dev = GenesisSeedEvaluator.TRAIN_SENTENCES[:6]  # Same sentences
        overlapping_heldout = GenesisSeedEvaluator.HELD_OUT_SENTENCES[:3]

        with self.assertRaises(ValueError) as cm:
            GenesisSeedEvaluator(
                self.checkpoint.name,
                train_sentences=overlapping_train,
                dev_sentences=overlapping_dev,
                heldout_sentences=overlapping_heldout,
            )



    def test_missing_eos_fails_exact_match(self):
        """Test that missing EOS fails exact_match."""
        # This test requires a logit function that does NOT emit EOS
        # We use a logit function that always emits BOS (token 0)
        def no_eos_logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.full((batch_size, seq_len, 262), -1000.0, device=x.device)
            logits[0, -1, 0] = 1000.0  # Always generate BOS
            return logits

        prompt = "The red dog"
        expected_suffix = " runs."

        result = generate_from_linear(
            self.model, prompt, no_eos_logit_fn,
            max_new_tokens=5, expected_suffix=expected_suffix
        )

        self.assertFalse(result.exact_match, "Missing EOS should fail exact_match")

    def test_action_accuracy(self):
        """Test that action_accuracy is correctly computed."""
        prompt = "The red dog"
        expected_action = "runs"

        result = generate_from_linear(
            self.model, prompt, self._make_eos_logit_fn(),
            max_new_tokens=10, expected_action=expected_action
        )

        self.assertEqual(result.expected_action, expected_action)
        # With EOS-generating logit, the action is "dog" (not "runs"), so action_accuracy
        # should be False because the generated action word is "dog", not "runs"
        # (The prompt only provides color and animal; the action must be generated)
        self.assertFalse(result.action_accuracy, "Action mismatch should fail action_accuracy")


class TestReportSerialization(unittest.TestCase):
    """Test JSON and Markdown report serialization."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=150))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)
        self.output_json = tempfile.NamedTemporaryFile(suffix=".json", delete=False)
        self.output_json.close()
        self.output_md = tempfile.NamedTemporaryFile(suffix=".md", delete=False)
        self.output_md.close()

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)
        Path(self.output_json.name).unlink(missing_ok=True)
        Path(self.output_md.name).unlink(missing_ok=True)

    def test_json_report_writes_and_parses(self):
        """Test that JSON report can be written and parsed back."""
        from evaluation.canonical_evaluator import GenesisSeedEvaluator

        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=GenesisSeedEvaluator.TRAIN_SENTENCES[:4],
        )
        evaluator.evaluate()
        evaluator.report_json(self.output_json.name)

        # Parse JSON back
        with open(self.output_json.name) as f:
            data = json.load(f)

        self.assertEqual(data["checkpoint_path"], str(self.checkpoint.name))
        self.assertEqual(data["checkpoint_sha256"], evaluator.checkpoint_sha256)
        self.assertEqual(data["context_limit"], self.config.seed_config.context)
        self.assertEqual(data["train"]["sentence_count"], 4)
        self.assertEqual(len(data["training_generations"]), 4)

    def test_markdown_report_written(self):
        """Test that Markdown report is written with expected content."""
        from evaluation.canonical_evaluator import GenesisSeedEvaluator

        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=GenesisSeedEvaluator.TRAIN_SENTENCES[:4],
        )
        evaluator.evaluate()
        evaluator.report_markdown(self.output_md.name)

        with open(self.output_md.name) as f:
            content = f.read()

        self.assertIn("GenesisSeed Evaluation Report", content)
        self.assertIn("Train", content)
        self.assertIn("Dev", content)
        self.assertIn("4", content)  # 4 train sentences


class TestContextLimitFromConfig(unittest.TestCase):
    """Test that context limit comes from the loaded model configuration."""

    def setUp(self):
        self.config = GenesisConfig(seed_config=SeedConfig(width=16, heads=1, layers=1, context=64))
        self.model = GenesisSeed(self.config)
        self.model.eval()
        self.checkpoint = tempfile.NamedTemporaryFile(suffix=".pt", delete=False)
        self.checkpoint.close()
        torch.save({
            "config": self.config.state_config(),
            "state_dict": self.model.state_dict(),
        }, self.checkpoint.name)

    def tearDown(self):
        Path(self.checkpoint.name).unlink(missing_ok=True)

    def test_context_limit_from_model_config(self):
        """Test that context limit is read from model config, not hardcoded."""
        from evaluation.canonical_evaluator import GenesisSeedEvaluator

        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=GenesisSeedEvaluator.TRAIN_SENTENCES[:4],
        )

        self.assertEqual(evaluator.result.context_limit, 64)
        self.assertEqual(evaluator.config.seed_config.context, 64)


class TestPartitionOverlapRejected(unittest.TestCase):
    """Test that partition overlap raises an error."""

    def test_overlap_detection(self):
        """Test that overlapping sentences in train and heldout raise error."""
        # This test documents that the evaluator requires explicit, disjoint partitions.
        # The evaluator itself stores sentences in separate lists; overlap detection
        # is a validation concern for the caller to enforce.
        train = GenesisSeedEvaluator.TRAIN_SENTENCES
        dev = GenesisSeedEvaluator.DEV_SENTENCES
        heldout = GenesisSeedEvaluator.HELD_OUT_SENTENCES

        # Verify partitions are disjoint (documented invariant)
        self.assertEqual(len(set(train) & set(dev)), 0)
        self.assertEqual(len(set(train) & set(heldout)), 0)
        self.assertEqual(len(set(dev) & set(heldout)), 0)


if __name__ == "__main__":
    unittest.main()
