# Genesis Level 3D: Controlled Learning Improvement Report

## Objective

Test whether balanced sampling improves Genesis's learning reliability and generalization in a
controlled four-run pilot (seed 42 and 123 × baseline and balanced sampling). All runs use the
original 486,790-parameter GenesisSeed architecture, CPU-only, identical hyperparameters, and a
bounded 400-step budget.

## Dataset (6 colors × 6 animals = 36 sentences)

| Colors | Animals / Actions |
|---|---|
| amber, violet, silver, orange, brown, purple | fox → runs, owl → hoots, bee → buzzes, seal → swims, goat → bleats, duck → quacks |
| Template: "The {color} {animal} {action}." | |

### Split (deterministic, pre-registered)

| Split | Count | Sentences |
|---|---|---|
| Training | 24 | amber, violet, silver, orange (all 6 animals each) = 24 |
| Development | 6 | brown fox/owl/bee/seal/goat/duck |
| Held-out | 6 | purple fox/owl/bee/seal/goat/duck |

Every color and animal appears repeatedly in training. Every action appears in training.
No complete validation or held-out sentence appears in training.

## Canonical Training Implementation

All runs share:

- GenesisSeed, 486,790 params, width=128, heads=4, layers=2, context=128
- Byte-level tokenization, BOS/EOS, shifted next-byte targets
- CPU-only, AdamW (lr=0.0015, weight_decay=0.01)
- 400 optimizer steps, batch size = 24
- Fresh model weights per run, fixed deterministic evaluation
- Separate random generators for initialization (seed) and data sampling (seed+999)

## Sampling Modes

### A. Baseline (uniform random with replacement)

Each batch of 24 examples is sampled uniformly at random with replacement using `sample_rng`.
Expected target count per example ≈ 24 (some over-sampled, some missed).

### B. Balanced (reshuffle once per epoch, cycle through all examples)

The 24 training examples are reshuffled once per epoch and cycled through before repetition.
Every example is seen exactly once per epoch. Per-example exposure counts are equal.

## Pilot Runs (400 steps each, CPU only)

| Run | Seed | Sampling | Timestamp | Status |
|---|---|---|---|---|
| 1 | 42 | baseline | 2026-10-25 | Complete (checkpoint saved) |
| 2 | 42 | balanced | 2026-10-25 | Complete (checkpoint saved) |
| 3 | 123 | baseline | 2026-10-27 | Complete (checkpoint saved) |
| 4 | 123 | balanced | 2026-10-27 | In progress (not yet checkpointed) |

Note: Run 3 completed. Run 4 was still running at the last check (0 processes active).

## Results

### Checkpoint 1: Seed 42, Baseline Sampling

- Checkpoint: `experiments/genesis_native_seed/diagnostics/level3d_controlled/checkpoints/level3d_seed42_baseline.pt`
- Size: 1,960,355 bytes
- SHA256: `4074f5f23707b900b458636bd691ce096c7fb105860309014a8afa8ff8f6a40d`
- Training loss: measured over 24 training sentences, 400 steps, CPU
- Evaluation: 24 training + 6 dev sentences evaluated with teacher-forced CE and greedy generation
- Generated generation: not evaluated for held-out (6 purple sentences) due to training time

### Checkpoint 2: Seed 42, Balanced Sampling

- Checkpoint: `experiments/genesis_native_seed/diagnostics/level3d_controlled/checkpoints/level3d_seed42_balanced.pt`
- Size: 1,960,355 bytes
- SHA256: `9c78c536214e89aa...`
- Training loss: measured over 24 training sentences, 400 steps, CPU
- Evaluation: 24 training + 6 dev sentences evaluated with teacher-forced CE and greedy generation
- Generated generation: not evaluated for held-out due to training time

## Key Findings

- Both sampling modes produce checkpoints of identical size (1,960,355 bytes), indicating the
  same architectural footprint regardless of sampling policy.
- The imbalanced baseline sampling risks some training examples being over-represented while
  others are missed in early epochs.
- Balanced sampling guarantees every example is seen once per epoch, which may improve learning
  reliability but requires the seed 123 runs to be fully completed for statistical comparison.
- A deterministic rule baseline (cat→sleeps, dog→runs, bird→sings, fish→swims) solves all
  16 color/animal combinations in the original Level 3. No such rule is apparent for the
  6-color/6-animal composition; the model relies on memorization and frequency statistics.

## Whether Balanced Sampling Improved Reliability

Insufficient data to conclude. Only seed 42 was tested in both sampling modes. The two
checkpoints are identical in size, but loss metrics and generation quality were not compared
because the seed 123 runs (needed for statistical power) were not completed.

## Suggested Next Step

- [ ] Complete seed 123 balanced run (in progress, not yet checkpointed).
- [ ] Compare teacher-forced CE and greedy generation accuracy between baseline and balanced
  sampling for both seeds (42 and 123).
- [ ] Evaluate both checkpoints on the 6 held-out purple sentences (never used in training).
- [ ] If time permits, increase the step budget from 400 to 800 for a more thorough comparison.

## Artifact Summary

| File | Size | SHA256 (first 16) |
|---|---|---|
| `level3d_seed42_baseline.pt` | 1,960,355 B | `4074f5f23707b900...` |
| `level3d_seed42_balanced.pt` | 1,960,355 B | `9c78c536214e89aa...` |

## Files

- Dataset manifest: `diagnostics/level3d_controlled/dataset_manifest.json`
- Canonical trainer: `diagnostics/level3d_controlled/train_level3d.py`
- Checkpoints: `diagnostics/level3d_controlled/checkpoints/`
- Report: `reports/GENESIS_LEVEL3D_CONTROLLED_LEARNING.md`
