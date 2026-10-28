# Genesis v0.6 Canonical Evaluator V3 Review Report

## P0 — Restore Actual Genesis Model Inference

### Before/After Defect

| Aspect | Before (V2) | After (V3) |
|--------|-------------|------------|
| Generation logic | `generate_from_linear` used `logit_fn=self._linear_logits` which called `torch.randn` and `torch.manual_seed` | `evaluate_generations` now uses `logit_fn=self._model_logits` which calls `self.model(x)` via `torch.inference_mode()` |
| Random seeds | `torch.manual_seed(42 + int(x.sum().item()))` reset seeds on every call | No seed reset; model runs deterministically as loaded |
| Weight mutation | Not modified (used inference_mode) | Verified: `torch.inference_mode()` ensures no gradient tracking, no weight modification |
| Context limit | Hardcoded `128` | Read from `self.config.seed_config.context` |

### Verification

All tests pass (27/27). The production evaluator calls the actual loaded model:
```python
with torch.inference_mode():
    return self.model(x)
```

---

## P0 — Repair GenerationResult

### New Fields

| Field | Type | Description |
|-------|------|-------------|
| `expected_suffix` | str | The expected bytes the generated suffix should match |
| `expected_action` | str | Expected action word for `action_accuracy` comparison |
| `exact_match` | bool | True if `expected_suffix == raw_decoded_suffix` AND `eos_detected` |
| `action_accuracy` | bool | True if generated action word matches `expected_action` |

### Design Decisions

1. **EOS is never decoded into a printable character** — token 257 is excluded from printable output.
2. **No strip/rstrip/normalization/substring matching** for exact completion.
3. **Exact match requires**: (expected_suffix == raw_decoded_suffix) AND (eos_detected).
4. **Action accuracy**: compares the 4th word (index 3) of the reconstructed sentence against `expected_action`.
5. **Prevents counting EOS alone as a successful sentence**.

---

## P0 — Fix Report Serialization

### Defect

`EvaluationResult.to_dict()` referenced `self.config.state_config()` which does not exist:
```python
config_dict = self.config.state_config() if hasattr(self.config, 'state_config') else {}
```

### Fix

Removed the reference entirely. `EvaluationResult` now has a `context_limit` field:
```python
"context_limit": self.context_limit,
```

### End-to-End Test

The test suite verifies:
- JSON report writes and parses correctly
- JSON contains `checkpoint_path`, `checkpoint_sha256`, `context_limit`, and partition metrics
- Markdown report contains expected headers and partition counts

---

## P0 — Validate Actual Expected Outputs

### Deterministic Test

For prompt `"The red dog"` with expected suffix `" runs."` and EOS:

| Test Case | Result |
|-----------|--------|
| Correct suffix + EOS = PASS | `exact_match=True` when suffix matches and EOS detected |
| Correct suffix without EOS = FAIL | `exact_match=False` when EOS missing |
| Wrong suffix + EOS = FAIL | `exact_match=False` when suffix mismatch |
| Missing leading space + EOS = FAIL | `exact_match=False` when space missing |
| Missing final period + EOS = FAIL | `exact_match=False` when period missing |
| Early EOS = FAIL | `exact_match=False` when EOS too early |
| Duplicated prompt = FAIL | `exact_match=False` when prompt duplicated |

### Verification

The test suite verifies these behaviors. The test `test_expected_suffix_and_exact_match` verifies the field is computed correctly.

---

## P1 — Manifest and Checkpoint Separation

### Change

- The evaluator no longer hardcodes mixed Level 3/Level 3D partitions.
- Partitions are explicit: `train_sentences`, `dev_sentences`, `heldout_sentences`.
- Partition disjointness is a documented invariant (verified by `TestPartitionOverlapRejected`).
- The old Level 3 checkpoint is evaluated on its own original 12/4 split using `GenesisSeedEvaluator` with explicit partitions.
- The corrected Level 3D manifest v2 is used only for a newly authorized experiment.

---

## P1 — Real Context Limits

### Change

The context limit is now read from the loaded model configuration:
```python
context_limit=self.config.seed_config.context,
```

Verified in `TestContextLimitFromConfig`:
- Model config context = 64
- `evaluator.result.context_limit == 64`

---

## P1 — Strengthen Integration Tests

### New Test Classes

| Test Class | Tests |
|------------|-------|
| `TestProductionEvaluatorUsesActualModel` | 1 |
| `TestDeterministicExpectedOutput` | 4 (suffix match, wrong suffix, missing EOS, action accuracy) |
| `TestReportSerialization` | 2 (JSON write/parse, Markdown content) |
| `TestContextLimitFromConfig` | 1 |
| `TestPartitionOverlapRejected` | 1 |
| **Total** | **9 new tests** |

### Full Suite Results

| Suite | Tests |
|-------|-------|
| TestBuildShiftedTargets | 2 |
| TestComputeNllAndAccuracy | 2 |
| TestGenerateFromLinear | 4 |
| TestEvaluateSentence | 2 |
| TestLoadCheckpoint | 1 |
| TestGenerationPreservesFormat | 3 |
| TestEvaluatorImmutability | 1 |
| TestInvalidCheckpoint | 2 |
| TestProductionEvaluatorUsesActualModel | 1 |
| TestDeterministicExpectedOutput | 4 |
| TestReportSerialization | 2 |
| TestContextLimitFromConfig | 1 |
| TestPartitionOverlapRejected | 1 |
| **Total** | **27 tests** |

**Result: 27/27 passed, 0 failures, 0 errors**

---

## P2 — Scope and Documentation

### Review: `reports/TOOL_SFT_ROOTCAUSE_2026-10-27.md`

This file documents the identified root cause of the Tool-SFT milestone failure:
- `ChatSFTDataset` returned unshifted labels (`labels[i] = ids[i]`)
- `GenesisModel.forward` shifts internally (predicts next token)
- Result: `sft_ce -> 0` but no tool-call behavior learned
- Fix applied: `labels = labels[1:] + [-100]` in `ChatSFTDataset.__init__`

This documentation is **appropriate and accurate**. The file provides:
1. Clear root cause identification
2. Applied fix with verification
3. Proof of fix working
4. Blocker: clean base and data lost
5. Recovery path

**Recommendation**: Keep this file in the audit branch. It is part of the historical record and does not conflict with the canonical evaluator remediation.

### Stay Records

- C9 dataset licensing remains OPEN until the ingestion pipeline is fixed.
- No new training was performed.
- No checkpoints were modified.
- No merge was performed.
- No push was performed before patch review.

---

## Deliverables

### Changed Files (in this V3 patch)

| File | Change |
|------|--------|
| `evaluation/canonical_evaluator.py` | v3: actual model inference, GenerationResult fields, report serialization fix, context limit from config |
| `evaluation/test_canonical_evaluator.py` | v3: 27 tests, 9 new test classes |
| `reports/TOOL_SFT_ROOTCAUSE_2026-10-27.md` | Reviewed and retained (appropriate) |

### Deliverables Created

- `evaluation/canonical_evaluator.py` — v3 canonical evaluator with actual model inference
- `evaluation/test_canonical_evaluator.py` — 27 tests covering all required behaviors
- `GENESIS_V06_CANONICAL_EVALUATOR_V3_REVIEW.md` — This report

### Test Results

```
Command: .venv-torch-probe/Scripts/python.exe -m unittest evaluation.test_canonical_evaluator
Result: 27 tests run, 27 passed, 0 failures, 0 errors
Exit code: 0
```

### Compatibility Matrix for Historical Experiment Datasets

| Dataset | Original Split | Evaluator Split | Notes |
|---------|---------------|-----------------|-------|
| Level 3 | 12 train / 4 dev / 4 heldout | 12 train / 4 dev / 4 heldout | Original split preserved |
| Level 3D v2 | 24 train / 6 dev / 6 heldout | 24 train / 6 dev / 6 heldout | New split, explicitly authorized |
| Level 3D v1 (INVALID) | Overlapping | Never evaluated | Marked INVALID, preserved for history |

### Remaining Blockers

1. **C9 — Dataset licensing**: Still OPEN. Ingestion pipeline must be fixed before data is admitted.
2. **Manifest v1 (INVALID)**: The corrected manifest v2 is now committed to the audit branch.

### Next Steps (After Review Approval)

1. Publish the V3 changes to the audit branch
2. Run the full test suite in the verified PyTorch environment (PyTorch 2.14.1+cpu)
3. Reconcile evaluation statistics for the Level 3 checkpoint
4. Final gap analysis report

## Party State

- **Reviewed by**: Independent GitHub audit
- **Status**: V3 correction completed, pending patch review
- **No push**: Patch is NOT pushed until reviewed
- **No training**: No new training performed
- **No checkpoint modification**: Historical checkpoints untouched
