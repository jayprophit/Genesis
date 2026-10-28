# Genesis v0.6 Canonical Evaluator V3.2 Review Report

## P0 — Fix Expected Suffix and Action Extraction

### Before/After Defect

| Aspect | Before (V3.1) | After (V3.2) |
|--------|---------------|--------------|
| `expected_suffix` derivation | `f" {parts[3]}."` | `f" {action_word}."` where `action_word = parts[3].rstrip(".")` |
| `expected_action` derivation | `parts[3]` (includes period) | `parts[3].rstrip(".").rstrip("!").rstrip("?").rstrip(",")` |

For the sentence `"The red cat sleeps."`, the old V3.1 code produced:
- `expected_suffix = " sleeps."` (with double period: " sleeps." + "." = " sleeps.")
- `expected_action = "sleeps."` (with trailing period)

The new V3.2 code produces:
- `expected_suffix = " sleeps."` (single period, correct)
- `expected_action = "sleeps"` (no trailing period)

### Regression Test: `test_expected_suffix_no_duplicated_period`

A production-level regression test that FAILS against V3.1 and PASSES against V3.2:

| Step | V3.1 Behavior | V3.2 Behavior |
|------|---------------|---------------|
| `parts[3]` | `"sleeps."` (includes period) | `"sleeps."` (includes period) |
| `action_word` | (not extracted) | `"sleeps"` (period stripped) |
| `expected_suffix` | `" sleeps."` + `"."` = `" sleeps."` (9 chars, doubled period) | `" sleeps."` (8 chars, single period) |
| `expected_action` | `"sleeps."` | `"sleeps"` |

The test asserts the FULL content of the V3.1 suffix (9 chars, double period) and the V3.2 suffix (8 chars, single period).

---

## P0 — Enforce Final Held-Out Authorization

### Defect in V3.1

The V3.1 `evaluate()` always evaluated the held-out partition:
```python
self.result.heldout = self.evaluate_partition(self.heldout_sentences, "heldout")
self.result.heldout_generations = self.evaluate_generations(self.heldout_sentences, "heldout")
```

### Fix in V3.2

`evaluate(final_heldout=False)` (default) now:
- Does NOT call `evaluate_partition(..., "heldout")`
- Does NOT call `evaluate_generations(..., "heldout")`
- Reports `heldout_status = "NOT_EVALUATED"`
- Reports `heldout.sentence_count = 0`, `heldout_total = 0`, `heldout_exact_matches = 0`

`evaluate(final_heldout=True)`:
- Evaluates held-out exactly once
- Records `heldout_status = "EVALUATED"`
- Records `heldout_status` and dataset fingerprint

### Regression Test: `test_heldout_not_inferred_when_final_heldout_false`

A spy/mock test proving zero held-out inference calls when `final_heldout=False`:
- Authenticated evaluation (`final_heldout=True`) → `evaluate_partition` called with `"heldout"`, 1 call
- Default evaluation (`final_heldout=False`) → `evaluate_partition` called 0 times on `"heldout"`
- Result: `heldout_status == "NOT_EVALUATED"`
- Also verifies `evaluate_generations` is not called on held-out with `final_heldout=False`

---

## P1 — Historical Level 3 Compatibility

### Change

The evaluator now supports an explicitly empty development partition (allowing Level 3 historical split: 12 train / 0 dev / 4 held-out).

| Dataset | Original Split | Evaluator Split | Summary |
|---------|---------------|-----------------|---------|
| Level 3 | 12 train / 0 dev / 4 held-out | 12 train / 0 dev / 4 held-out | Empty dev partition supported |
| Level 3D v2 | 24 train / 6 dev / 6 held-out | 24 train / 6 dev / 6 held-out | Standard split |

### New Fields in `EvaluationResult`

| Field | Type | Description |
|-------|------|-------------|
| `experiment_identity` | str | Explicit protocol: `"Level3"`, `"Level3Dv2"`, or custom |
| `manifest_fingerprint` | str \| None | SHA256 of the manifest defining the partitions |
| `checkpoint_training_compatibility` | str | `"VERIFIED"` or `"UNVERIFIED"` |

- **No silent substitution**: partitions are explicit, not defaults.
- **No mixing**: Level 3 and Level 3D datasets are never combined in one evaluation.
- **Checkpoint metadata**: where unavailable, compatibility is labeled `"UNVERIFIED"` (never invented).

---

## P1 — Fix Context Stop Reason

### Change

When generation stops because the configured context boundary is reached, `stop_reason` is now `"context_limit"` (not `"token_limit"`):

```python
# Old (V3.1)
if x_gen.shape[1] >= context_limit:
    stop_reason = "token_limit"
    break

# New (V3.2)
if x_gen.shape[1] >= context_limit:
    stop_reason = "context_limit"
    break
```

### Verification

- `TestGenerateFromLinear.test_context_limit_stop`: verifies `stop_reason == "context_limit"` when context is reached
- The model is never called with an oversized input (checked before each inference call)
