# Genesis v0.6 Level 2 Final Verification Report

**Run ID:** 20261025_level2_final_verification
**Date:** 2026-10-25
**Status:** PASS

---

## 1. Checkpoint Used

| Item | Value |
|---|---|
| Checkpoint | `diagnostics/multi_sentence/checkpoints/multi_sentence_model_20261024_multi_sentence_experiment_001.pt` |
| Environment | `.venv-torch-probe\Scripts\python.exe` (Probe venv, PyTorch 2.14.1+cpu) |
| Model config | GenesisSeed: width=128, layers=2, heads=4, context=128 |
| Parameters | 486,790 |
| Inference | CPU only, temperature=0, greedy decoding (argmax) |

---

## 2. Phase 1: Prompt-Conditioned Generation Benchmark

### Test Configuration
- `model.eval()` / `torch.inference_mode()`
- Temperature = 0
- `max_new_tokens = 64`

### Results

| Prefix | Reconstructed Sentence | Exact Match | EOS After Period? |
|---|---|---|---|
| `"The red"` | `The red cat is happy.` | ✅ PASS | ✅ |
| `"The blue"` | `The blue dog is sleepy.` | ✅ PASS | ✅ |
| `"The green"` | `The green bird can fly.` | ✅ PASS | ✅ |
| `"The little"` | `The little boy has a ball.` | ✅ PASS | ✅ |
| `"A yellow"` | `A yellow tree grows tall.` | ✅ PASS | ✅ |
| `"The dark"` | `The dark cat sleeps.` | ✅ PASS | ✅ |
| `"The white"` | `The white dog runs fast.` | ✅ PASS | ✅ |
| `"A small"` | `A small bird sings.` | ✅ PASS | ✅ |

**Generation samples (raw suffixes):**

| Prompt | Raw Generated Suffix | Tokens (incl. EOS) |
|---|---|---|
| `"The red"` | ` cat is happy.` | `[32, 99, 97, 116, 32, 105, 115, 32, 104, 97, 112, 112, 121, 46, 257]` |
| `"The blue"` | ` dog is sleepy.` | `[32, 100, 111, 103, 32, 105, 115, 32, 115, 108, 101, 101, 112, 121, 46, 257]` |
| `"The green"` | ` bird can fly.` | `[32, 98, 105, 114, 100, 32, 99, 97, 110, 32, 102, 108, 121, 46, 257]` |
| `"The little"` | ` boy has a ball.` | `[32, 98, 111, 121, 32, 104, 97, 115, 32, 97, 32, 98, 97, 108, 108, 46, 257]` |
| `"A yellow"` | ` tree grows tall.` | `[32, 116, 114, 101, 101, 32, 103, 114, 111, 119, 115, 32, 116, 97, 108, 108, 46, 257]` |
| `"The dark"` | ` cat sleeps.` | `[32, 99, 97, 116, 32, 115, 108, 101, 101, 112, 115, 46, 257]` |
| `"The white"` | ` dog runs fast.` | `[32, 100, 111, 103, 32, 114, 117, 110, 115, 32, 102, 97, 115, 116, 46, 257]` |
| `"A small"` | ` bird sings.` | `[32, 98, 105, 114, 100, 32, 115, 105, 110, 103, 115, 46, 257]` |

**All 8 prefixes yield correct continuations with EOS predicted after the final period.**
**Classification: PASS**

---

## 3. Phase 2: Training Loss Reconciliation

### Methodology
For each of the 8 training sentences:

```python
sequence = [BOS] + encode(sentence) + [EOS]
x = sequence[:-1]
y = sequence[1:]
loss = F.cross_entropy(logits.squeeze(0), y.squeeze(0), reduction='sum', ignore_index=-100)
avg_loss = loss / num_targets
```

### Full-Dataset Statistics

| Metric | Value |
|---|---|
| Total target positions | **189** |
| Full-dataset cross-entropy | **0.088791** |
| Full-dataset next-byte accuracy | **95.8%** |

### Per-Sentence Breakdown

| Sentence | Avg. Cross-Entropy | Targets | Accuracy |
|---|---|---|---|
| The red cat is happy. | 0.089849 | 22 | 95.5% |
| The blue dog is sleepy. | 0.090106 | 24 | 95.8% |
| The green bird can fly. | 0.094763 | 24 | 95.8% |
| The little boy has a ball. | 0.083150 | 27 | 96.3% |
| A yellow tree grows tall. | 0.083669 | 26 | 92.3% |
| The dark cat sleeps. | 0.091891 | 21 | 100.0% |
| The white dog runs fast. | 0.085949 | 25 | 96.0% |
| A small bird sings. | 0.093449 | 20 | 95.0% |

### Comparison with Previous Results

| Metric | Previous Report | This Verification | Match? |
|---|---|---|---|
| Training loss | 4.652 (initial) | — | — |
| Training loss | 0.092 (final) | 0.088791 | ✅ Consistent |
| Target positions | 189 (reported) | 189 (verified) | ✅ Matches |
| Next-byte accuracy | — | 95.8% | ✅ Consistent |

**Reconciliation note:** The previous training log reported a final cross-entropy of 0.092, which is consistent with the recomputed full-dataset cross-entropy of 0.088791 (within expected variance from the exact training step). The total target count of 189 is independently verified by summing the per-sentence target counts: 22 + 24 + 24 + 27 + 26 + 21 + 25 + 20 = 189.

---

## 4. Checkpoint SHA256

| Artifact | SHA256 (first 16 hex chars) |
|---|---|
| Multi-sentence checkpoint | `d0e81c71ace5cfb616d907d1cf71e9dda013b2d78a2b94bbeadc5c1b375b09fc` |
| Tiny-overfit checkpoint | `a4e7ab2051dcc13e5cbbaeed77ee3c602538b45f6548678e07ae11c4be3a43de` |
| BOS20 400-step (historical) | `ea4680b222f62a60...` |
| TinyStories 486k s42 100 (historical) | `d00b46fcf7b8d375...` |
| Sampler fixed 100-step (historical) | `41d63a2a153c2230...` |
| Sampler fixed 400-step (historical) | `9062f9745a6c35ea...` |

**Verification:** No hash mix-up. The multi-sentence checkpoint hash is distinct from all historical TinyStories checkpoints and the tiny-overfit checkpoint.

---

## 5. Phase 4 — Comparison with Previous Results

| Metric | Level 1 (Tiny-Overfit) | Level 2 (Multi-Sentence) |
|---|---|---|
| Architecture | 486,790 params | 486,790 params |
| Training data | 1 sentence | 8 sentences |
| Steps | 400 | 400 |
| Final cross-entropy | 0.000759 | 0.088791 |
| Teacher-forced accuracy | 100% (1 sentence) | 92-100% (8 sentences) |
| Prompt-conditioned generation | 100% (1 sentence) | 100% (8/8 sentences) |
| BOS-only generation | Exact reproduction | "The dark cat sleeps." |

**Key finding:** The multi-sentence model has learned to generate all 8 training sentences correctly when given a proper prefix. The training loss of 0.092 is consistent with the reported 0.092. The model has achieved exact-sequence memorization for all 8 training sentences.

---

## 6. Phase 3 — Final Classification

**Classification: PASS**

All eight distinguishable training prefixes yield their correct continuations with EOS:

1. `"The red"` → `The red cat is happy.` ✅
2. `"The blue"` → `The blue dog is sleepy.` ✅
3. `"The green"` → `The green bird can fly.` ✅
4. `"The little"` → `The little boy has a ball.` ✅
5. `"A yellow"` → `A yellow tree grows tall.` ✅
6. `"The dark"` → `The dark cat sleeps.` ✅
7. `"The white"` → `The white dog runs fast.` ✅
8. `"A small"` → `A small bird sings.` ✅

**This is a training-set memorization test, not an unseen-data generalization test.**

---

## 7. Preservation Status

| Item | Status |
|---|---|
| Multi-sentence checkpoint | Preserved (SHA256: d0e81c71...) |
| Tiny-overfit scratch checkpoint | Preserved (SHA256: a4e7ab20...) |
| Four historical TinyStories checkpoints | Preserved (SHA256: ea4680b2..., d00b46fc..., 41d63a2a..., 9062f974...) |
| Genesis-Model Git repository | Unmodified |
| Genesis-Human-Experiment Git repository | Unmodified |
| Native NumPy inference runtime | Unmodified |
| No new packages installed | Confirmed |

---

## 8. Deliverables

**Report:** `GENESIS_LEVEL2_FINAL_VERIFICATION.md`
**Location:** `C:\Users\jpowe\Desktop\Projects\Genesis\reports\`

---

## 9. Next Steps (Pending Approval)

If approved, the next logical experiment is a held-out generalization test:
- Train on 8 sentences, test on unseen combinations of the same vocabulary
- Example: Train on "The red cat is happy." and "The blue dog is sleepy.", test "The red dog is sleepy."
- This tests generalization, not memorization

**No further training has been initiated. No artifacts were modified.**
