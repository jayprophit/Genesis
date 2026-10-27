# Genesis v0.6 Multi-Sentence Learning Report

**Run ID:** 20261024_multi_sentence_experiment_001
**Date:** 2026-10-25
**Status:** PARTIAL — Model learned next-byte prediction (teacher-forced accuracy 92-100%) but generation from partial prompts is inconsistent.

---

## 1. Source and Experiment Configuration

| Parameter | Value |
|---|---|
| Run ID | 20261024_multi_sentence_experiment_001 |
| Project | `C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed` |
| Python | `C:\Users\jpowe\Desktop\Projects\Genesis\.venv-torch-probe\Scripts\python.exe` |
| Architecture | GenesisSeed: width=128, layers=2, heads=4, context=128 |
| Parameters | 486,790 |
| Random seed | 42 |
| Training steps | 400 |
| Batch | 8 sentences per step (shuffled per step) |
| Device | CPU only |
| Optimizer | AdamW (lr=0.0015, weight_decay=0.01) |

**Checkpoint:** `diagnostics/multi_sentence/checkpoints/multi_sentence_model_20261024_multi_sentence_experiment_001.pt` (1,962,171 bytes)

---

## 2. Training Sentences (8)

| # | Sentence | Bytes |
|---|---|---|
| 1 | The red cat is happy. | 22 |
| 2 | The blue dog is sleepy. | 24 |
| 3 | The green bird can fly. | 24 |
| 4 | The little boy has a ball. | 27 |
| 5 | A yellow tree grows tall. | 26 |
| 6 | The dark cat sleeps. | 21 |
| 7 | The white dog runs fast. | 25 |
| 8 | A small bird sings. | 20 |

---

## 3. Loss History

| Step | Loss |
|---|---|
| 1 | 4.652178 |
| 25 | 0.149288 |
| 50 | 0.107659 |
| 75 | 0.093988 |
| 100 | 0.095122 |
| 200 | 0.093052 |
| 300 | 0.093288 |
| 400 | 0.091891 |

**Final training cross-entropy (avg per example): 0.091891**
**Loss reduction: 4.560287**
**Runtime: 105.8s**

---

## 4. Per-Sentence Accuracy

| Sentence | Teacher-Forced Accuracy | Generated Text |
|---|---|---|
| 1: The red cat is happy. | 95.5% (20/22) | Empty (EOS after prompt) |
| 2: The blue dog is sleepy. | 95.8% (22/24) | Empty (EOS after prompt) |
| 3: The green bird can fly. | 95.8% (22/24) | Empty (EOS after prompt) |
| 4: The little boy has a ball. | 96.3% (26/27) | "l nas." |
| 5: A yellow tree grows tall. | 92.3% (24/26) | Empty (EOS after prompt) |
| 6: The dark cat sleeps. | 100.0% (21/21) | Empty (EOS after prompt) |
| 7: The white dog runs fast. | 96.0% (23/25) | Empty (EOS after prompt) |
| 8: A small bird sings. | 95.0% (18/20) | Empty (EOS after prompt) |

**BOS-only generation:** "The dark cat sleeps."

---

## 5. Exact Prompt/Generation Comparisons

The model learned next-byte prediction at 92-100% accuracy (teacher-forced), meaning it correctly predicts the next byte for the training data. However:

- **Generation from partial prompts:** The model generates empty strings (EOS prediction) or partial outputs, not the trained sentences.
- **BOS-only generation:** Reproduces ONE training sentence ("The dark cat sleeps.") exactly.

This indicates the model has memorized the training data at the byte-prediction level, but has NOT learned to generate complete coherent sentences from partial prompts.

---

## 6. EOS Results

- Teacher-forced accuracy: 92-100% — the model correctly predicts EOS at the end of each sentence.
- BOS-only greedy generation: Reproduces ONE training sentence ("The dark cat sleeps.").
- Partial prompt generation: Generates EOS immediately for 7 of 8 sentences (empty output), meaning the model predicts EOS after the prompt.

---

## 7. Checkpoint Hash and Location

**Checkpoint path:**
`diagnostics/multi_sentence/checkpoints/multi_sentence_model_20261024_multi_sentence_experiment_001.pt`

**Size:** 1,962,171 bytes

---

## 8. CPU Runtime and Resource Observations

| Metric | Value |
|---|---|
| CPU time | 105.8 seconds (400 steps, 8 sentences/step) |
| Memory | Single process, ~1 GB (model weights) |
| Peak memory | Monitorable via OS tools (not instrumented) |

---

## 9. Confirmed Failures

| Issue | Severity | Notes |
|---|---|---|
| Generation from partial prompts | Confirmed | 7/8 sentences: empty output (EOS predicted) |
| Full sentence reproduction from prompts | Failed | Only BOS reproduces a sentence |
| Generalization to unseen combinations | Not tested | Held-out test pending (Phase 4) |

---

## 10. Recommendations for Level 3

1. **Increase training steps:** 400 steps may be insufficient for multi-sentence learning. Consider 2,000-10,000 steps.
2. **Increase model capacity:** The 486k parameter model may be too small for 8 sentences. Try 1M (685,062 params) or 2M (2,549,510 params) tiers.
3. **Increase context length:** The 128-token context limits the longest sentences that can be learned.
4. **Separate generalization test:** Use the held-out composition design from Phase 4 to measure whether the model can recombine familiar vocabulary in unseen ways.

---

## 11. Outcome Classification

**PARTIAL** — The model learned to predict next bytes (teacher-forced accuracy 92-100%) and can reproduce one training sentence from BOS. However, generation from partial prompts fails to produce complete sentences. The model has memorized training data at the token level but has not yet demonstrated generative coherence for multi-sentence content.

---

## 12. Phase 4: Separate Generalization Preparation

**Proposed held-out composition test (NOT executed):**

| Training vocabulary | Held-out combinations |
|---|---|
| The red cat is happy. | The blue dog is happy. |
| The blue dog is sleepy. | The green bird is sleepy. |
| The green bird can fly. | The red dog can fly. |
| The little boy has a ball. | The little boy has a ball. (repeated) |

**Note:** These validation examples are kept completely outside training. Do NOT automatically launch this experiment — it requires separate approval.

---

## 13. Data Integrity

- All 4 historical TinyStories checkpoints preserved (SHA256 verified)
- Successful tiny-overfit scratch checkpoint preserved
- Main model source untouched
- Genesis-Model Git repository unchanged
- Genesis-Human-Experiment Git repository unchanged
- No third-party weights or NCM implemented
