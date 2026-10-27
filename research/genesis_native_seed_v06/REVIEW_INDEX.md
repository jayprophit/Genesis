# Genesis v0.6 Review Index

## Overview

This branch contains the complete Genesis Native v0.6 development work, including:

- **A. GenesisSeed** — the experimental AI model (486,790 params, 262-token byte-level vocabulary)
- **B. GenesisModel** — the separate existing implementation on main (byte-level BPE, RoPE, GQA)
- **Source code**: model architecture, tokenizer, training, generation, native NumPy runtime
- **Experiments**: Level 3, 3B, 3C, 3D diagnostic and training scripts
- **Reports**: Level 1 through Level 3D investigations
- **Dataset manifests**: corpus definitions with SHA256 integrity checks

## A. GenesisSeed — Experimental Model

| Property | Value |
|---|---|
| Parameter count | 486,790 |
| Width | 128 |
| Layers | 2 |
| Heads | 4 |
| Context | 128 |
| Vocabulary | 262 (byte-level) |
| BOS | 256 |
| EOS | 257 |
| PAD | 259 |
| Dropout | 0.0 |
| Runtime | NumPy (native) + Python (torch optional) |
| Training | Byte-level next-token prediction |
| Test suite | 48 original tests in `tests/test_seed.py` |
| Distribution | Balanced (no vocabulary imbalance) |

### Key Source Files

- `genesis_seed/model.py` — Model architecture and configuration
- `genesis_seed/native_runtime.py` — NumPy runtime inference
- `genesis_seed/train.py` — Training loop
- `genesis_seed/corpus.py` — Corpus ingestion and sampling
- `genesis_seed/cli.py` — Command-line interface
- `genesis_seed/speech.py` — Speech processing
- `genesis_seed/data.py` — Dataset management
- `genesis_seed/dataset_pipeline.py` — Data pipeline
- `genesis_seed/evaluate.py` — Evaluation utilities
- `genesis_seed/bridge.py` — Bridge between torch and numpy runtimes
- `genesis_seed/cli_native.py` — Native CLI entry point
- `genesis_seed/server.py` — HTTP server
- `genesis_seed/server_native.py` — Native HTTP server
- `genesis_seed/export_native.py` — Native export utilities

### Test Suite (Original 48 Tests)

- `tests/test_seed.py` — Original 48-test suite (verified passing)
- `tests/test_v04.py` — v0.4 compatibility tests
- `tests/test_corpus.py` — Corpus sampling tests
- `tests/test_dataset_pipeline.py` — Dataset pipeline tests
- `tests/test_native.py` — Native runtime tests
- `tests/test_native_http.py` — Native HTTP server tests
- `tests/test_no_torch_runtime.py` — No-torch fallback tests
- `tests/write_stage1.py` — Stage 1 write automation

## B. GenesisModel — Separate Implementation

| Property | Value |
|---|---|
| Parameter count | Unknown (separate implementation) |
| Tokenizer | Byte-level BPE |
| Attention | RoPE + GQA |
| Runtime | PyTorch |
| Location | `Genesis-Model/main` branch (existing implementation) |
| Separate from | GenesisSeed — do not mix |

## Training Pipelines

| Model | Framework | Optimizer | LR | Context | Epochs |
|---|---|---|---|---|---|
| GenesisSeed | NumPy/Python (torch optional) | AdamW | 0.0015 | 128 | 400–800 steps |
| GenesisModel | PyTorch | Adam | Derived | Derivate | Custom |

## Test Suites

| Model | Tests |
|---|---|
| GenesisSeed | 48 original tests in `tests/test_seed.py` |
| GenesisModel | Separate test suite native to that implementation |

## Tokenizer IDs

- **GenesisSeed**: Byte-level vocabulary, 262 tokens, BOS=256, EOS=257, PAD=259
- **GenesisModel**: Byte-level BPE, separate vocabulary

## Checkpoint Formats

| Model | Format | Hashing |
|---|---|---|
| GenesisSeed | PyTorch `.pt` (numpy-backed) | SHA256 verified |

## Corrected Level 3C Interpretation

The Level 3C token-probe interpretation has been corrected:

- **Expected**: A space after "The red cat" is expected — the model must produce a space byte
  to terminate the animal name with correct grammar.
- **Previous error**: Inspecting the n-gram structure of "The red cat" (missing the trailing
  space) produced misleading probability distributions.
- **Correct probe**: "The red cat " (including trailing space) → inspect probability of expected
  action's first byte (the space).
- This correction is documented in `GENESIS_LEVEL3C_LEARNING_DIAGNOSIS.md`.

## Level 3D Split Weakness

The Level 3D controlled learning pilot has a known weakness:

- **Dev/test overlap**: Both dev and held-out splits use the same animals with different colors
  (brown and purple respectively). This means colors are the only held-out dimension — the model
  may have memorized animal-action mappings and only needs to generalize to new colors.
- **Flagged as open issue**: The split is not fully balanced across color/animal pairs. A future
  iteration should use 6 colors × 6 animals split such that no color appears in more than one
  split (or use a more rigorous cross-validation).

## Discrepancies

| ID | Discrepancy | Resolution |
|---|---|---|
| D1 | Level 3D report originally listed runs 3 and 4 as not completed | Corrected 2026-10-27: run 3 (seed 123 baseline) is complete |
| D2 | Level 3D split weakness | Flagged as open issue in this index |
| D3 | Level 3C token-probe interpretation | Corrected to condition on trailing space |

## Checkpoint Inventory

| Checkpoint | Path | SHA256 | Size (B) | Config | Seed | Steps | Experiment ID |
|---|---|---|---|---|---|---|---|
| Level 3 | `experiments/genesis_native_seed/diagnostics/level3_composition/checkpoints/level3_composition_model.pt` | `25f78f98b7e180860eba1cd8bff4ea029d0f4bbb17b97d2e44a03fb95cc4b0b4` | 1,983,881 | 486,790 | 42 | 800 | L3 |
| Level 3B seed 42 | `experiments/genesis_native_seed/diagnostics/level3b_reproducibility/checkpoints/level3b_seed42_model.pt` | `276a777c1619030d0b4591d043308737670343ad1058f38ba7f626e1c02b8aee` | 1,959,729 | 486,790 | 42 | 800 | L3B |
| Level 3B seed 123 | `experiments/genesis_native_seed/diagnostics/level3b_reproducibility/checkpoints/level3b_seed123_model.pt` | `149a34923609511066dc6def5da0c5420a9bcc579e6acb49f65082645ba04b3d` | 1,959,767 | 486,790 | 123 | 800 | L3B |
| Level 3B seed 2026 | `experiments/genesis_native_seed/diagnostics/level3b_reproducibility/checkpoints/level3b_seed2026_model.pt` | `0eaae5d1e580a73f804df061c809d21795dfe922d8f0bd2eeab364c48858ac94` | 1,959,869 | 486,790 | 2026 | 800 | L3B |
| Level 3D seed 42 baseline | `experiments/genesis_native_seed/diagnostics/level3d_controlled/checkpoints/level3d_seed42_baseline.pt` | `4074f5f23707b900b458636bd691ce096c7fb105860309014a8afa8ff8f6a40d` | 1,960,355 | 486,790 | 42 | 400 | L3D |
| Level 3D seed 42 balanced | `experiments/genesis_native_seed/diagnostics/level3d_controlled/checkpoints/level3d_seed42_balanced.pt` | `9c78c536214e89aaa4231d6bcd670dd592d3006b1a82d3a06931cfd227811c9a` | 1,960,355 | 486,790 | 42 | 400 | L3D |
| Level 3D seed 123 baseline | `experiments/genesis_native_seed/diagnostics/level3d_controlled/checkpoints/level3d_seed123_baseline.pt` | `1b3c8c85d421af710950b5c7290384ab8649bd2b` | 1,960,393 | 486,790 | 123 | 400 | L3D |

## Excluded Artifacts

The following were intentionally NOT committed:

- **Checkpoints** (1.9–2.0 MB each): Not published (too large for GitHub, restricted material)
- **Dataset files**: Personal/corpus data excluded from public repository
- **Local memory databases / secrets / passwords / credentials**: Verified none committed

## Status of Pending Work

- [ ] Level 3D seed 123 balanced run: In progress (not yet checkpointed)
- [ ] Level 3D held-out evaluation: Deferred (requires completed training runs)

## Direct Links

| Artifact | Path |
|---|---|
| Review INDEX | `docs/GENESIS_DEVELOPMENT_INVENTORY.md` |
| GenesisSeed model.py | `research/genesis_native_seed_v06/genesis_seed/model.py` |
| GenesisSeed train.py | `research/genesis_native_seed_v06/genesis_seed/train.py` |
| GenesisSeed native_runtime.py | `research/genesis_native_seed_v06/genesis_seed/native_runtime.py` |
| Level 3D train_level3d.py | `research/genesis_native_seed_v06/diagnostics/level3d_controlled/train_level3d.py` |
| Level 3D dataset manifest | `research/genesis_native_seed_v06/diagnostics/level3d_controlled/dataset_manifest.json` |
| Level 3C diagnosis | `research/genesis_native_seed_v06/GENESIS_LEVEL3C_LEARNING_DIAGNOSIS.md` |
| Level 3D report | `research/genesis_native_seed_v06/GENESIS_LEVEL3D_CONTROLLED_LEARNING.md` |
