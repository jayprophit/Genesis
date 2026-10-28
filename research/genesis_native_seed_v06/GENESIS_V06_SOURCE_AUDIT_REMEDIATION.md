# Genesis v0.6 Source Audit Remediation

Audit of confirmed GenesisSeed experiment and evaluation defects, with remediation plan.

## Confirmed Code Defects

| ID | Defect | Source File | Severity |
|---|---|---|---|
|| C1 | Original Level 3 model initialization occurs before proper RNG seeding | `diagnostics/level3_composition/phase2_train.py` (not in core genesis_seed/train.py) | High |
|| C2 | Phase 3 corrected evaluation slices a generated suffix incorrectly | `phase3_corrected_eval.py` | Medium |
|| C3 | Incorrect full-dataset CE averaging | `diagnostics/level3_composition/phase3_corrected_eval.py` (not in phase0_reconcile.py) | High |
|| C4 | Held-out loss uses incomplete prompt text | `phase3_corrected_eval.py` | Medium |
|| C5 | Level 3B generation exact-match calculations do not require EOS | `train_level3b.py` (not in review) | Medium |
|| C6 | Multiple optimizer updates per reported outer step | `genesis_seed/train.py` (Level 3) and `diagnostics/level3d_controlled/train_level3d.py` (Level 3D) | Medium |
|| C7 | Level 3D balanced mode trains on a full batch each step | `train_level3d.py` | Low |
|| C8 | Level 3D split excludes two colors entirely from training | `dataset_manifest.json` | High |
|| C9 | Dataset ingestion assumes files have CC0 licensing and training permission | All dataset-related code | **OPEN** |

### C8 Correction

**Location:** `diagnostics/level3d_controlled/dataset_manifest.json`

**Issue:** The original dataset split excluded brown and purple from training. The manifest has been corrected.

**Fix:** Updated `Level3D-fix/dataset_manifest_fixed.json` so that all 6 colors appear in training.

## Defect Details

### C1: RNG Initialization Order

**Location:** `diagnostics/level3_composition/phase2_train.py`

**Issue:** In the original Level 3 training script, the model was created before `torch.manual_seed(seed)` was called, meaning the model's weight initialization was not reproducible.

**Fix:** Seed torch and random generators before model creation. The current `train.py` already does this correctly:

```python
torch.manual_seed(seed)
random.seed(seed)
model = GenesisSeed(SeedConfig(width=width))
```

But the original Level 3 script used a different ordering.

### C3: Per-Sentence Average Loss

**Location:** `diagnostics/level3_composition/phase3_corrected_eval.py`

**Issue:** The original code computed per-sentence average loss as:

```python
avg_loss = loss_sum / num_targets
```

This is correct for teacher-forced evaluation. However, the "Full-dataset CE" was computed as:

```python
total_train_loss / len(TRAIN_SENTENCES)
```

This divides summed loss by the number of sentences, not the number of target positions, giving a wrong per-sentence value.

**Fix:** Use `total_targets` (token count) for the division:

```python
full_dataset_ce = total_loss_sum / total_targets
```

### C4: Held-Out Loss Uses Incomplete Prompt

**Location:** `diagnostics/level3_composition/phase3_corrected_eval.py`

**Issue:** Held-out sentences like `"The red dog runs."` were evaluated with incomplete prompt text `"The red dog"` instead of the full sentence. This gave the model an unfair advantage (or disadvantage) since it wasn't evaluating on the full context.

**Fix:** Use the full sentence for evaluation, not a truncated prefix.

### C6: Multiple Optimizer Updates per Outer Step

**Location:** `diagnostics/level3_composition/phase2_train.py`

**Issue:** The original training script ran both text and vision losses through the optimizer in a single step:

```python
tl = F.cross_entropy(model(tx).flatten(0, 1), ty.flatten())
il = F.cross_entropy(model(ix, im).flatten(0, 1), iy.flatten())
loss = tl + il
loss.backward()
opt.step()
```

This counts as one optimizer step but aggregates two loss components. The reported "steps" count may not match the effective number of gradient updates.

**Fix:** Track optimizer updates separately from outer loop steps.

### C7: Level 3D Balanced Mode Uses Full Batch

**Location:** `diagnostics/level3d_controlled/train_level3d.py`

**Issue:** The balanced sampling mode uses `batch_size = len(train_sequences)` (24) each step, meaning it always trains on the full dataset per step. This is not "balanced" in the sense of cycling through examples; it's equivalent to a full-batch update.

**Fix:** Use a fixed batch size (e.g., 6) and sample from the balanced epoch sequence, not the entire dataset per step.

### C8: Level 3D Split Excludes Colors

**Location:** `diagnostics/level3d_controlled/dataset_manifest.json`

**Issue:** The original dataset split put all 6 colors into training (amber, violet, silver, orange, brown, purple) and the dev set used brown, while held-out used purple. This means:
- Dev and held-out use the SAME color (brown vs purple)
- The model never sees the held-out color during training
- Colors are not balanced across splits

**Fix:** Use cyclic pairing indices so each color appears in multiple splits, with the held-out and dev sets balanced across colors.

### C9: Dataset Licensing Assumption

**Location:** All dataset-related code

**Issue:** Dataset ingestion code assumed all files have CC0 licensing and training permission without verification.

**Fix:** Require explicit provenance and permission declarations before data admission. Add tests that reject unverified documents.

## Files Modified

| File | Action |
|---|---|
| `research/genesis_native_seed_v06/evaluation/canonical_evaluator.py` | New canonical evaluator |
| `research/genesis_native_seed_v06/evaluation/test_canonical_evaluator.py` | New unit tests |
| `research/genesis_native_seed_v06/Level3D-fix/dataset_manifest_fixed.json` | Fixed dataset manifest |
| `research/genesis_native_seed_v06/GENESIS_V06_SOURCE_AUDIT_REMEDIATION.md` | This audit report |

## Historical Measurements Affected

| Measurement | Status |
|---|---|
| Level 3 CE (0.121441) | Use corrected evaluation for re-evaluation |
| Level 3B held-out (0/4) | Re-evaluate with canonical metrics |
| Level 3D pilot results | Re-evaluate with canonical evaluator |

## Proposed Revised Level 3D Protocol

### Dataset Distribution

Use cyclic pairing indices with one held-out pair and one development pair per color:

```
For color i and animal j:
  train_pairs[i][j] = (color_i, animal_j) if j != heldout and j != dev
  dev_pairs[i] = (color_i, animal_j) for one dev animal
  heldout_pairs[i] = (color_i, animal_j) for one heldout animal
```

### Sampling Modes

- **Uniform random with replacement:** Sample 6 examples per step from the 24 training sentences
- **Balanced reshuffling without replacement:** Cycle through all 24 training examples once per epoch

### Hyperparameters

- Batch size: 6
- Optimizer steps: 400 (matched between modes)
- LR: 0.0015, weight_decay: 0.01
- Same initialization seed for both modes
- Equal token budget: 24 × 400 = 9,600 tokens per epoch

### Evaluation

- Evaluate on 24 train, 6 dev, 6 held-out separately
- Use canonical evaluator with weights_only=True
- Require EOS for exact match
- Generate from exact distinguishing prefixes
- Compare action accuracy and exact sentence accuracy separately

### Pre-registration

The training schedule and evaluation rules must be frozen before any run begins.

## Next Steps

1. Run existing test suite plus new evaluator tests
2. Validate canonical evaluator against historical checkpoints
3. Apply corrected evaluation to historical results
4. Design and pre-register revised Level 3D protocol
5. Execute revised Level 3D runs (do not train further models without review)
