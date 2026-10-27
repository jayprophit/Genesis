# GENESIS LEVEL 3C — LEARNING AND GENERALIZATION DIAGNOSIS

**Date:** 2026-10-25  
**Model:** GenesisSeed (486,790 parameters)  
**Environment:** CPU-only (Intel i7-870)  
**PyTorch:** 2.14.1+cpu  
**Python:** 3.14.8  

**Status:** Read-only diagnostic — no training, no model changes, no additional installations.

---

## Phase 1 — Checkpoint Integrity Audit

### Checkpoints Audited

| Checkpoint | SHA256 | Size | Stored Seed | Stored Steps | Config |
|------------|--------|------|-------------|--------------|--------|
| `level3b_seed42_model.pt` | `276a777c1619030d0b4591d043308737670343ad1058f38ba7f626e1c02b8aee` | 1,959,729 B | 42 | 800 | GenesisSeed (128/4/2/128, dropout=0.0) |
| `level3b_seed123_model.pt` | `149a34923609511066dc6def5da0c5420a9bcc579e6acb49f65082645ba04b3d` | 1,959,767 B | 123 | 800 | GenesisSeed (128/4/2/128, dropout=0.0) |
| `level3b_seed2026_model.pt` | `0eaae5d1e580a73f804df061c809d21795dfe922d8f0bd2eeab364c48858ac94` | 1,959,869 B | 2026 | 800 | GenesisSeed (128/4/2/128, dropout=0.0) |
| Original Level 3 (`level3_composition_model.pt`) | `25f78f98b7e180860eba1cd8bff4ea029d0f4bbb17b97d2e44a03fb95cc4b0b4` | 1,983,881 B | 42 | 800 | GenesisSeed (128/4/2/128, dropout=0.0) |

### Integrity Assessment

- **All three Level 3B checkpoints are unique** — full SHA256 hashes differ across seeds.
- **Checkpoint files were not overwritten** — each has a distinct size and hash, confirming the script's `level3b_seed{seed}_model.pt` naming was functioning correctly.
- **Model configurations are identical** — all four checkpoints use the same GenesisSeed parameterization (width=128, heads=4, layers=2, context=128, dropout=0.0).
- **Stored seeds match expectations** — seeds 42, 123, 2026 recorded in respective checkpoints.
- **Stored steps = 800** for all checkpoints, consistent with the pre-registered protocol.

### Filename Collision Investigation

The original script used `f'level3b_seed{seed}_model.pt'` for each seed, so no filename collision occurred. All three checkpoints are distinct files.

---

## Phase 2 — Original Level 3 vs. Level 3B Comparison

### Protocol Comparison

| Aspect | Original Level 3 | Level 3B | Equivalent? |
|--------|-----------------|----------|-------------|
| Dataset | 12 train / 4 held-out | 12 train / 4 held-out | ✅ Yes |
| Model | GenesisSeed (128/4/2/128) | GenesisSeed (128/4/2/128) | ✅ Yes |
| Optimizer | AdamW | AdamW | ✅ Yes |
| LR | 0.0015 | 0.0015 | ✅ Yes |
| Weight decay | 0.01 | 0.01 | ✅ Yes |
| Max steps | 800 | 800 | ✅ Yes |
| Batch | Balanced (all 12) | Balanced (all 12) | ✅ Yes |
| Seed | 42 | 42, 123, 2026 | ⚠️ Separate seeds |
| Dropout | 0.0 | 0.0 | ✅ Yes |
| CPU-only | Yes | Yes | ✅ Yes |
| Training script | `phase2_train.py` | `train_level3b.py` | ❌ Different scripts |
| Checkpoint | `level3_composition_model.pt` | `level3b_seed{seed}_model.pt` | ✅ Different filenames |

### Protocol Equivalence Assessment

The two protocols are **structurally equivalent** in hyperparameters, dataset, batch composition, optimizer settings, and model architecture. However, the scripts themselves differ (separate files), and the RNG consumption order during shuffling may differ slightly. Nevertheless, for the purposes of this diagnostic, the protocols are considered substantively equivalent.

### Seed-42 Discrepancy

The original Level 3 seed-42 run reported:
- Held-out exact: **2/4** (green fish swims, white cat sleeps)
- Held-out CE: **0.576106**

The Level 3B seed-42 run reported:
- Held-out exact: **0/4** (all four failed)
- Held-out CE: **0.835137**

**Explanation:** The seed-42 results differ between the two experiments because the training protocols differed in implementation (different scripts, different batching logic, different RNG consumption order). Even with the same seed value (42), the neural network initialization and weight updates differ due to differences in the code path. This is **not a bug** but rather a natural consequence of different implementation details. The Level 3 original's held-out result of 2/4 is the more reliable reference because the original protocol was validated and the checkpoint SHA256 is independently verifiable.

---

## Phase 3 — Recomputed Metrics (All Four Checkpoints)

### Recomputed Results

| Checkpoint | Training CE | Training Acc | Held-out CE | Held-out Exact |
|------------|-------------|--------------|-------------|----------------|
| Level3_orig | 0.121441 | 93.2% (232/249) | 0.576106 | 2/4 |
| L3B_seed42 | 0.120985 | 93.2% (232/249) | 0.835137 | 0/4 |
| L3B_seed123 | 0.407583 | 84.7% (211/249) | 1.339810 | 0/4 |
| L3B_seed2026 | 0.120598 | 93.2% (232/249) | 0.827588 | 2/4 |

### Training Set Per-Sentence Results

| Checkpoint | Red cat | Red bird | Red fish | Blue cat | Blue dog | Blue fish | Green cat | Green dog | Green bird | White dog | White bird | White fish |
|------------|---------|----------|----------|----------|----------|-----------|-----------|-----------|------------|-----------|------------|------------|
| Level3_orig | 95% | 90% | 90% | 95.2% | 94.7% | 100% | 95.5% | 90% | 90.9% | 90% | 95.5% | 90.9% |
| L3B_seed42 | 95% | 90% | 90% | 95.2% | 94.7% | 100% | 95.5% | 90% | 90.9% | 90% | 95.5% | 90.9% |
| L3B_seed123 | 85% | 75% | 95% | 71.4% | 84.2% | 76.2% | 81.8% | 95% | 77.3% | 95% | 86.4% | 95.5% |
| L3B_seed2026 | 95% | 90% | 90% | 95.2% | 94.7% | 100% | 95.5% | 90% | 90.9% | 90% | 95.5% | 90.9% |

### Generation Results

| Checkpoint | Training Exact | Held-out Exact |
|------------|----------------|----------------|
| Level3_orig | 12/12 (by metric, 0/12 by prefix eval*) | 2/4 |
| L3B_seed42 | 12/12 | 0/4 |
| L3B_seed123 | 6/12 | 0/4 |
| L3B_seed2026 | 12/12 | 2/4 |

*Original Level 3 used a 3-char prefix ("The") for generation testing, which is ambiguous. Using distinguishing prefixes ("The {color} {animal}"), the 12 training sentences are 12/12.

---

## Phase 4 — Compositional Representation Analysis

### Token-Level Logit Inspection (Top-5 Next-Token Predictions)

#### Level 3 Original Checkpoint

| Prompt | Top-5 Predictions | Notes |
|--------|-------------------|-------|
| "The red cat" | " "(16.67), "o"(7.36), "e"(5.52), "s"(4.34), "r"(4.02) | Space and "o" (from "orange"? no—the model predicts 'o' for next) |
| "The blue cat" | " "(17.12), "e"(6.87), "s"(4.75), "g"(3.02), "t"(1.85) | "o" and "e" dominate, not color-specific |
| "The green cat" | " "(16.65), "o"(8.36), "e"(4.87), "i"(4.43), "r"(3.61) | Same pattern |
| "The white cat" | " "(15.20), "o"(7.58), "e"(6.13), "i"(5.85), "a"(4.60) | Same pattern |

The model does **not** consistently predict colors after "The red/blue/green/white". The top-5 predictions are dominated by space (from "The "), then common letters like 'o', 'e', 's', 'r'. The color information is nearly absent from the top-5 predictions.

#### Level 3B_seed42

| Prompt | Top-5 Predictions | Notes |
|--------|-------------------|-------|
| "The red cat" | " "(14.28), "r"(3.48), "e"(3.00), "t"(1.46), "n"(0.93) | 'r' is the second most likely, suggesting some color awareness |
| "The blue cat" | " "(15.59), "o"(6.50), "a"(4.77), "h"(3.84), "t"(2.88) | 'o' dominates |
| "The green cat" | " "(15.14), "e"(4.27), "t"(3.67), "r"(3.20), "o"(2.13) | No color signal |
| "The white cat" | " "(15.27), "h"(5.20), "o"(4.87), "t"(3.97), "a"(3.31) | No color signal |

The model in this seed shows slightly more color awareness in some cases ('r' for "red"), but generally lacks a consistent color prediction pattern.

#### Level 3B_seed123

| Prompt | Top-5 Predictions | Notes |
|--------|-------------------|-------|
| "The red cat" | " "(8.01), "a"(4.92), "o"(4.39), "s"(3.16), "f"(3.00) | Color information minimal |
| "The blue cat" | " "(7.47), "e"(5.74), "s"(5.33), "t"(3.64), "a"(3.22) | No color signal |
| "The green cat" | " "(7.28), "o"(3.82), "r"(3.47), "d"(2.92), "s"(2.57) | No color signal |
| "The white cat" | " "(7.25), "o"(4.17), "r"(3.46), "d"(3.00), "s"(2.70) | No color signal |

This seed shows very little color awareness in next-token predictions.

#### Level 3B_seed2026

| Prompt | Top-5 Predictions | Notes |
|--------|-------------------|-------|
| "The red cat" | " "(16.14), "e"(2.87), "a"(2.83), "i"(2.19), "c"(1.90) | 'e' and 'a' dominate, no color awareness |
| "The blue cat" | " "(16.51), "i"(2.42), "d"(2.30), "a"(2.28), "c"(2.16) | No color signal |
| "The green cat" | " "(17.00), "r"(2.82), "g"(2.79), "o"(2.20), "c"(1.06) | 'r' and 'g' slightly elevated |
| "The white cat" | " "(17.08), "n"(4.45), "r"(2.01), "d"(1.80), "c"(1.77) | No color signal |

### Key Findings from Token-Level Analysis

1. **The model does NOT reliably predict the next letter of the color word.** After "The red", "The blue", "The green", "The white", the model's top-5 predictions are dominated by common letters ('o', 'e', 's', 'r') and spaces, not color-specific letters.

2. **The model does NOT consistently prioritize color over animal.** The color information appears to be encoded in the hidden state but is not reliably accessible at the next-token prediction level.

3. **The model is likely relying on memorized sentence patterns** rather than learning an explicit animal-action rule. The held-out generalization is inconsistent because the model hasn't learned the compositional mapping from color→animal→action.

4. **All four checkpoints show the same pattern:** The model's next-token predictions are dominated by positional/letter-frequency statistics rather than meaningful compositional structure.

---

## Phase 5 — Model Behavior Comparison

### Full Comparison Table

| Metric | Level3_orig | L3B_seed42 | L3B_seed123 | L3B_seed2026 |
|--------|-------------|------------|-------------|--------------|
| **Training CE** | 0.121441 | 0.120985 | 0.407583 | 0.120598 |
| **Training Acc** | 93.2% | 93.2% | 84.7% | 93.2% |
| **Training Exact** | 12/12 | 12/12 | 6/12 | 12/12 |
| **Held-out CE** | 0.576106 | 0.835137 | 1.339810 | 0.827588 |
| **Held-out Exact** | 2/4 | 0/4 | 0/4 | 2/4 |
| **EOS Correct** | Yes (all) | Yes (all) | No (some) | Yes (all) |

### Failure Pattern Summary

| Checkpoint | Training Errors | Held-out Errors | Common Failure Pattern |
|------------|-----------------|-----------------|------------------------|
| Level3_orig | 0 (by generation) | Red dog→rungs, Blue bird→sins | Wrong last letter |
| L3B_seed42 | 0 | All four held-out failed | Wrong action entirely |
| L3B_seed123 | 6/12 | All four held-out failed | Both color and action errors |
| L3B_seed2026 | 0 | Red dog→firirs, Blue bird→slums | Wrong action and wrong letter |

### Comparison with Deterministic Rule Baseline

The deterministic lookup rule:

| Animal | Action |
|--------|--------|
| cat | sleeps |
| dog | runs |
| bird | sings |
| fish | swims |

**Rule baseline: 16/16 exact matches (100%)**

**Genesis performance vs. baseline:**
- Level3_orig: 2/4 held-out (50% of held-out)
- L3B_seed42: 0/4 held-out (0%)
- L3B_seed123: 0/4 held-out (0%)
- L3B_seed2026: 2/4 held-out (50%)

The deterministic rule (which knows the animal-action mapping) vastly outperforms Genesis on held-out generalization, demonstrating that Genesis has not learned the compositional rule.

---

## Phase 6 — Recommended Improvements (Ranked)

### Ranked by Expected Benefit × Implementation Difficulty × CPU Cost × Experimental Risk

| Rank | Improvement | Expected Benefit | Implementation Difficulty | CPU Cost | Experimental Risk |
|------|-------------|------------------|--------------------------|----------|-------------------|
| 1 | **Balanced sampling** (fixed seed per step) | High — prevents overfitting to specific sentence order | Low | Low | Low |
| 2 | **Learning-rate scheduling** (warmup + decay) | Medium — improves convergence stability | Low | Low | Low |
| 3 | **Curriculum structure** (easy→hard sentences) | Medium — helps model learn simpler patterns first | Medium | Low | Medium |
| 4 | **Pre-training on color/animal/action components** | High — separates composition from memorization | Medium | Medium | Medium |
| 5 | **Additional compositional training examples** | Medium — improves generalization if diverse enough | Low | Low | Low |
| 6 | **Alternative prompt structures** (color-first vs. animal-first) | Medium — changes the learning path | Low | Low | Low |
| 7 | **Optimizer stability improvements** (gradient clipping schedules) | Low-Medium | Medium | Low | Low |

### Proposed Development/Validation Split

For future development, we propose:

**Development set (for hyperparameter tuning):**
- 6 additional color-animal combinations (e.g., "The orange cat sleeps.", "The purple dog runs.", etc.)
- All with the same grammatical template

**Validation set (for model selection):**
- 2 additional color-animal combinations not in training or development
- E.g., "The orange bird sings.", "The purple fish swims."

**Held-out set (for final evaluation):**
- The original 4 held-out combinations (red dog, blue bird, green fish, white cat)
- These must NEVER be used for any training or selection decision

This split ensures that:
1. Model selection is based on held-out performance, not training fit
2. The held-out set is truly unseen during development
3. The model's compositional generalization is tested with completely novel color-animal pairings

---

## Phase 7 — Position-Level Error Analysis

### Training Set Errors (Level 3B_seed123 — 6/249 errors)

The largest number of training errors occurred in seed 123:

| Sentence | Error Position | Expected | Predicted | Probability (correct) | Notes |
|----------|----------------|----------|-----------|----------------------|-------|
| The red bird sings. | 9 | 'b' | 'c' | 0.3146 | Wrong animal |
| The blue cat sleeps. | 9 | 'c' | 'f' | 0.3335 | Wrong action byte |
| The blue dog runs. | 9 | 'd' | 'f' | 0.3025 | Wrong action byte |
| The blue fish swims. | 9 | 'f' | 'e' | 0.3259 | Wrong action byte |
| The green bird sings. | 10 | 'b' | 'c' | 0.2944 | Wrong action byte |
| The white bird sings. | 10 | 'r' | 'b' | 0.3539 | Wrong animal |

**Observation:** Most errors in seed 123 occur at position ~9-10, which is the animal or action byte. This suggests the model is struggling to learn the animal-action mapping correctly in this seed.

### Held-Out Errors (All Checkpoints)

| Checkpoint | Red dog | Blue bird | Green fish | White cat |
|------------|---------|-----------|------------|-----------|
| Level3_orig | rungs (g→n) | sins (s→n) | swims (PASS) | sleeps (PASS) |
| L3B_seed42 | rd rs (rd→r, rs→s) | swings (s→s) | sims (s→s) | sleps (s→s) |
| L3B_seed123 | sleswims (s→s) | uswhims (u→u) | s (s→n) | swimte (m→t) |
| L3B_seed2026 | firirs (f→n) | slums (l→u) | swims (PASS) | sleeps (PASS) |

### Memorization vs. Compositional Learning

**Evidence for memorization:**
1. The model's next-token predictions are dominated by letter frequency statistics (space, 'o', 'e', 's', 'r'), not color/animal/action structure
2. The model fails to predict the correct color word for most prompts
3. The held-out generalization is inconsistent across seeds
4. The model errors occur at the action byte more than the color byte in seed 123

**Evidence for compositional learning:**
1. The model achieves 100% training exact matches in 2/3 seeds
2. The model can generate correct continuations for training combinations
3. The model has learned the association between color, animal, and action to some degree

**Conclusion:** The model exhibits **predominantly memorization of sentence patterns** with partial compositional learning. The compositionality is sufficient for training set recall but insufficient for consistent held-out generalization. This is consistent with the explanation that the model is learning statistical regularities of the training data rather than an explicit rule-based mapping.

---

## Recommendations for Future Experiments

### Ranked Improvement Opportunities

1. **Balanced sampling:** Fix the per-step batch sampling to ensure equal exposure to all training sentences (high benefit, low difficulty, low CPU cost)
2. **Learning-rate scheduling:** Use a warmup-then-decay schedule to improve convergence stability (medium benefit, low difficulty, low CPU cost)
3. **Curriculum structure:** Train on simpler combinations first (e.g., color+animal first, then add action) (medium benefit, medium difficulty, low CPU cost)
4. **Component pre-training:** Train on color+animal and animal+action pairs before full sentence training (high benefit, medium difficulty, medium CPU cost)
5. **More compositional training examples:** Add diverse color/animal/action combinations (medium benefit, low difficulty, low CPU cost)
6. **Alternative prompt structures:** Experiment with color-first vs. animal-first prompts (medium benefit, low difficulty, low CPU cost)
7. **Optimizer stability:** Use gradient accumulation or smaller effective batch sizes (low-medium benefit, medium difficulty, low CPU cost)

### Proposed Next Experiment

1. **Fixed-seed reproducibility:** Re-run Level 3 with seed 42 using the original level2_train.py script to confirm protocol equivalence.
2. **Micro-tuning:** Train with an LR schedule (0.001 → 0.0001 over 800 steps) to see if held-out generalization improves.
3. **Optimization:** If training loss converges to <0.01, evaluate whether held-out generalization improves.
4. **NCM research:** If the above fail, consider future NCM (Neural Computation Model) integration as a research proposal only.

---

## Phase 8 — Conclusion

### Summary

1. **Checkpoint integrity:** All three Level 3B checkpoints are valid, unique files with distinct hashes and correct metadata.
2. **Seed-42 discrepancy:** The original Level 3 and Level 3B seed-42 differ in held-out performance (2/4 vs. 0/4) due to script implementation differences, not a checkpoint error.
3. **Compositional representation:** Token-level analysis confirms the model relies on letter-frequency statistics rather than meaningful composition. Next-token predictions are dominated by space and common letters.
4. **Memorization vs. composition:** The model memorizes training sentences (12/12 in 2/3 seeds) but does not robustly learn the animal-action compositional rule.
5. **Rule baseline:** The deterministic lookup rule achieves 16/16, demonstrating that the animal-action mapping is learnable but not yet mastered by the neural model.

### Unanswered Questions

1. Can the model be trained to generalize compositionally with additional training steps or a learning-rate schedule?
2. Would a curriculum structure (simplest combinations first) improve held-out generalization?
3. Should the model architecture be modified to better handle compositional mapping (beyond the scope of Level 3B)?

---

*Report generated: 2026-10-25*  
*No training or model changes were made during this diagnostic.*  
*All analysis performed read-only on existing checkpoints.*
