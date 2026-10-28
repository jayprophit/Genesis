# Genesis v0.6 Canonical Evaluator v2 Review Report

## Phase 7 — Audit Documentation

### C3: Incorrect Full-Dataset CE Averaging

**Location:** `diagnostics/level3_composition/phase3_corrected_eval.py`

**Issue:** The "Full-dataset CE" was computed by dividing summed loss by the number of sentences, not by the number of target positions.

**Fix:** Use `total_targets` (token count) for the division:

```python
full_dataset_ce = total_loss_sum / total_targets
```

### C6: Multiple Optimizer Updates per Outer Step

**Location:** `genesis_seed/train.py` (Level 3) and `diagnostics/level3d_controlled/train_level3d.py` (Level 3D)

**Issue:** Level 3 trains one optimizer update per sentence (12 updates per outer iteration). Level 3D trains one update per batch of 24. These represent different training budgets and are not directly comparable.

**Fix:** Track optimizer updates separately from outer loop steps.

### C8: Corrected Dataset Manifest

**Location:** `diagnostics/level3d_controlled/dataset_manifest.json` (original, invalid)

**Fix:** Created `Level3D-fix/dataset_manifest_v2.json` using cyclic pair allocation:
- For each color index i: held-out animal = i, dev animal = (i+1)%6, training = remaining 4 animals
- 24 training sentences, 6 dev sentences, 6 held-out sentences
- 36 unique sentences total, zero partition overlap
- All 6 colors and 6 animals represented in training

### C9: Dataset Licensing (OPEN until code fix)

**Location:** All dataset ingestion code

**Status:** OPEN — The corrected manifest does not fix the ingestion pipeline, which automatically assumes CC0 licensing.

## Phase 8 — Test Results

### Test Suite Summary

|| Test Group | Passed | Failed | Skipped | Total |
||---|---|---|---|---|
|| TestBuildShiftedTargets | 2 | 0 | 0 | 2 |
|| TestComputeNllAndAccuracy | 2 | 0 | 0 | 2 |
|| TestGenerateFromLinear | 4 | 0 | 0 | 4 |
|| TestEvaluateSentence | 2 | 0 | 0 | 2 |
|| TestLoadCheckpoint | 1 | 0 | 0 | 1 |
|| TestEvaluatorImmutability | 1 | 0 | 0 | 1 |
|| TestInvalidCheckpoint | 2 | 0 | 0 | 2 |
|| TestPartitionSeparation | 1 | 0 | 0 | 1 |
|| TestGenerationPreservesFormat | 3 | 0 | 0 | 3 |
|| **Total** | **18** | **0** | **0** | **18** |

**Total: 18 tests, 18 passed, 0 errors, 0 failures**

### Test Command and Environment

```
Command: C:/Users/jpowe/Desktop/Projects/Genesis/.venv-torch-probe/Scripts/python.exe -c "import sys; sys.path.insert(0, 'research/genesis_native_seed_v06'); sys.path.insert(0, 'research/genesis_native_seed_v06/evaluation'); import unittest; from evaluation.test_canonical_evaluator import *; suite = unittest.TestLoader().loadTestsFromModule(sys.modules[__name__]); runner = unittest.TextTestRunner(verbosity=1); result = runner.run(suite); print('Tests run:', result.testsRun); print('Failures:', len(result.failures)); print('Errors:', len(result.errors))"
Environment: Python 3.14.8, PyTorch 2.14.1+cpu, i7-870 CPU
Exit code: 0 (18 tests executed, 18 passed, 0 failed)
```

### Modified Files

| File | Change |
|---|---|
|| `research/genesis_native_seed_v06/evaluation/canonical_evaluator.py` | v2 rewrite: structured generation result, explicit partition handling, checkpoint SHA256, proper metadata extraction |
|| `research/genesis_native_seed_v06/evaluation/test_canonical_evaluator.py` | v2 tests: 18 tests, deterministic controlled logits, EOS-producing test model |
|| `research/genesis_native_seed_v06/Level3D-fix/dataset_manifest_v2.json` | New manifest v2 with cyclic pair allocation: 24/6/6 split, zero overlap, all colors in training |
|| `research/genesis_native_seed_v06/Level3D-fix/dataset_manifest_v1_INVALID.json` | Old invalid manifest (preserved for historical comparison) |

### Remaining Defects

1. **C9 (Dataset Licensing):** OPEN — The ingestion pipeline still automatically assumes CC0 licensing. Requires code and tests that actually verify provenance.

### Next Steps

1. Publish corrected manifest v2 as a follow-up artifact on the audit branch
2. Run the evaluator against the actual Level 3 checkpoint to reproduce results
3. Validate the manifest v2 against the standard evaluation scripts
4. Prepare for publication to the audit branch
