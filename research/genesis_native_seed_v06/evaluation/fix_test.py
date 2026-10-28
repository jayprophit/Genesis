#!/usr/bin/env python3
"""Fix test_canonical_evaluator.py: replace test method and remove unused helper."""

import sys

path = "c:/Users/jpowe/Desktop/Projects/Genesis/Genesis-Model/research/genesis_native_seed_v06/evaluation/test_canonical_evaluator.py"
with open(path, "r") as f:
    content = f.read()

# Remove _make_eos_logit_fn method
old_helper = """    def _make_eos_logit_fn(self):
        def logit_fn(x):
            batch_size, seq_len = x.shape
            logits = torch.zeros(batch_size, seq_len, 262, device=x.device)
            logits[0, -1, 257] = 1000.0  # EOS at last position
            return logits
        return logit_fn

"""
content = content.replace(old_helper, "")

# Replace the test method
old_method = """    def test_production_output_matches_direct_greedy_inference(self):
        \"\"\"Production output matches direct greedy inference from same model.

        Every generated token ID and EOS stop reason from the production
        evaluator must match an independent direct greedy-decoding reference
        using the same checkpoint and prompt. No test merely checks result
        count or object existence.
        """
        from evaluation.canonical_evaluator import GenesisSeedEvaluator, generate_from_linear, decode, encode, BOS, EOS

        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=GenesisSeedEvaluator.TRAIN_SENTENCES[:4],
        )
        evaluator.evaluate()

        # Independent direct greedy-decoding reference: same model, same prompt,
        # deterministic decoding (argmax at each step), same expected suffix.
        # We reproduce the reference outside the evaluator to catch any
        # divergence in the production path.
        for i, train_sentence in enumerate(GenesisSeedEvaluator.TRAIN_SENTENCES[:4]):
            parts = train_sentence.split()
            # Skip sentence without 4+ tokens (some are truncated in TRAIN_SENTENCES[:4])
            if len(parts) < 4:
                continue
            prefix = f\"The {parts[1]} {parts[2]}\"
            expected_suffix = f\" {parts[3]}..\"
            expected_tokens = encode(expected_suffix) + [EOS]

            # Direct greedy decoding from the same checkpoint
            direct_result = generate_from_linear(
                self.model, prefix, self._make_eos_logit_fn(),
                max_new_tokens=10, expected_suffix=expected_suffix
            )

            # Production evaluator generation for the same seed
            # Find the matching generation: skip sentences that don't have 4+ tokens
            gens = evaluator.result.training_generations
            self.assertGreaterEqual(len(gens), i + 1)
            gen = gens[i]

            # Compare every generated token ID
            self.assertEqual(
                gen.generated_token_ids, direct_result.generated_token_ids,
                \"Generated token IDs differ between production evaluator and direct greedy inference\"
            )
            # Compare EOS detection and stop reason
            self.assertEqual(
                gen.eos_detected, direct_result.eos_detected,
                \"EOS detection differs between production evaluator and direct greedy inference\"
            )
            self.assertEqual(
                gen.stop_reason, direct_result.stop_reason,
                \"Stop reason differs between production evaluator and direct greedy inference\"
            )
            self.assertEqual(
                gen.num_generated_tokens, direct_result.num_generated_tokens,
                \"Number of generated tokens differs between production evaluator and direct greedy inference\"
            )
            self.assertEqual(
                gen.expected_suffix, expected_suffix,
                \"Expected suffix differs from declared grammar\"
            )
            self.assertEqual(
                gen.expected_action, parts[3],
                \"Expected action does not match sentence structure\"
            )
"""

new_method = """    def test_production_output_matches_direct_greedy_inference(self):
        \"\"\"Production output matches direct greedy inference from same model.

        Every generated token ID and EOS stop reason from the production
        evaluator must match an independent direct greedy-decoding reference
        using the same checkpoint and prompt. No test merely checks result
        count or object existence.
        """
        from evaluation.canonical_evaluator import (
            GenesisSeedEvaluator, generate_from_linear, decode, encode, BOS, EOS
        )

        # Use the model actually loaded by the evaluator for the reference
        # (independent greedy decoding on the same checkpoint).
        evaluator = GenesisSeedEvaluator(
            self.checkpoint.name,
            train_sentences=GenesisSeedEvaluator.TRAIN_SENTENCES[:4],
        )
        evaluator.evaluate()

        # Direct greedy reference: run the same model through
        # generate_from_linear with greedy logits (argmax), same expected suffix.
        for i, train_sentence in enumerate(GenesisSeedEvaluator.TRAIN_SENTENCES[:4]):
            parts = train_sentence.split()
            # Skip sentence without 4+ tokens (some are truncated in TRAIN_SENTENCES[:4])
            if len(parts) < 4:
                continue
            prefix = f\"The {parts[1]} {parts[2]}\"
            expected_suffix = f\" {parts[3]}..\"

            # Direct greedy decoding from the same loaded model
            direct_result = generate_from_linear(
                evaluator.model, prefix, evaluator._model_logits,
                max_new_tokens=128, expected_suffix=expected_suffix
            )

            # Production evaluator generation for the same seed
            gens = evaluator.result.training_generations
            self.assertGreaterEqual(len(gens), i + 1)
            gen = gens[i]

            # Compare every generated token ID and EOS stop reason
            self.assertEqual(
                gen.generated_token_ids, direct_result.generated_token_ids,
                \"Generated token IDs differ between production evaluator and direct greedy inference\"
            )
            self.assertEqual(
                gen.eos_detected, direct_result.eos_detected,
                \"EOS detection differs between production evaluator and direct greedy inference\"
            )
            self.assertEqual(
                gen.stop_reason, direct_result.stop_reason,
                \"Stop reason differs between production evaluator and direct greedy inference\"
            )
            self.assertEqual(
                gen.num_generated_tokens, direct_result.num_generated_tokens,
                \"Number of generated tokens differs between production evaluator and direct greedy inference\"
            )
            self.assertEqual(
                gen.expected_suffix, expected_suffix,
                \"Expected suffix differs from declared grammar\"
            )
            self.assertEqual(
                gen.expected_action, parts[3],
                \"Expected action does not match sentence structure\"
            )
"""

if old_method in content:
    content = content.replace(old_method, new_method)
    with open(path, "w") as f:
        f.write(content)
    print("Replaced successfully")
else:
    print("OLD METHOD NOT FOUND")
    idx = content.find("test_production_output_matches_direct_greedy_inference")
    if idx != -1:
        print(content[idx:idx+3000])
