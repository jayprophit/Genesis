# Genesis Native v0.6 — Development Inventory

**Generated:** 2026-10-25  
**Repository:** jayprophit/Genesis  
**Branch:** main  
**Local path:** `C:\Users\jpowe\Desktop\Projects\Genesis\Genesis-Model`

---

## 1. Repository Identification

| Property | Value |
|----------|-------|
| Owner | jayprophit |
| Repository | Genesis |
| URL | https://github.com/jayprophit/Genesis |
| Default branch | main |
| Visibility | Public |
| GitHub CLI permission | ADMIN |
| Local branch | main |

---

## 2. Directory Structure

```
Genesis-Model/
├── .git/                          # Git repository (507 files including .git internals)
├── .pytest_cache/                 # pytest cache
├── configs/                       # Configuration files
├── data/                          # Datasets and corpus
│   ├── corpus/
│   ├── local/
│   │   ├── conversation-corpus/
│   │   ├── gutenberg/
│   │   ├── gutenberg_heldout/
│   │   ├── gutenberg_raw/
│   │   └── master-prompt-subset/
│   ├── sft/                       # Supervised fine-tuning data
│   │   └── tool_use.jsonl
│   ├── tokenized/
│   └── heldout_local_1m_clean.json
├── docs/                          # Documentation
│   ├── EVIDENCE.md (modified)
│   ├── GAP_ANALYSIS.md (modified)
│   ├── GENESIS_STATUS.yaml (modified)
│   ├── MODEL_CARD_TEMPLATE.md (modified)
│   ├── GENESIS_BUILD_DIRECTION_2026-10-23.md (untracked)
│   └── heldout_local_1m_clean.json (untracked)
├── genesis_model/                 # Main Genesis Native v0.6 source
│   ├── __init__.py
│   ├── __main__.py
│   ├── aetherius_bridge.py        # Untracked (new)
│   ├── agent.py
│   ├── checkpoint.py
│   ├── cli.py
│   ├── config.py
│   ├── corpus.py
│   ├── data.py
│   ├── eval.py
│   ├── generate.py
│   ├── model.py                   # Modified
│   ├── server.py                  # Modified
│   ├── tokenizer.py               # Modified
│   ├── tools.py
│   └── train.py
├── registry/                      # Model registry
├── reports/                       # Reports directory
│   ├── SFT_STAGE_SCOPE_2026-10-25.md (untracked)
│   ├── WEEK1_CRITICAL_PATH_RESULTS_2026-10-25.md (untracked)
│   ├── eval_heldout_local_1m_gutenberg_2026-10-25.json (untracked)
│   └── GENESIS_LEVEL3C... (existing reports)
├── resources/                     # Resource files
│   ├── datasets/
│   │   └── tools/
│   │       └── Genesis_v0_6_Real_Dataset_Helper.zip (untracked)
├── runs/                          # Training runs
│   ├── gpu_bench/
│   ├── local_1m_clean/
│   ├── local_1m_gutenberg/
│   ├── local_1m_master_full/
│   ├── local_1m_master_retry/
│   ├── local_1m_master_retry2/
│   ├── local_3m_stage5/
│   ├── sft_a_tool_seq256/
│   ├── shard_smoke/
│   └── ...
├── scripts/                       # Training and evaluation scripts
│   ├── eval_heldout.py            # Untracked
│   ├── extend_context.py          # Untracked
│   ├── fetch_gutenberg.py
│   ├── generate_manifest_subset.py
│   ├── gen_sft_corpus.py          # Untracked
│   ├── local_train.py             # Modified
│   ├── prepare_master_subset.py
│   ├── sft_tool.py                # Untracked
│   ├── smoke_test.py
│   ├── tokenize_corpus.py
│   ├── train_preset.py
│   └── validate_sft.py            # Untracked
├── tests/                         # Test suite
│   ├── conftest.py
│   ├── __pycache__/
│   ├── test_agent.py
│   ├── test_data.py
│   ├── test_heldout_overlap.py    # Untracked
│   ├── test_local.py
│   ├── test_model.py
│   ├── test_tokenizer.py          # Modified
│   └── test_tools.py
├── .gitignore                     # Git ignore configuration
├── LICENSE
├── README.md
└── setup.py
```

---

## 3. Genesis Native v0.6 Components

### 3.1 Model Architecture
- `genesis_model/model.py` — GenesisSeed architecture (486,790 params, width=128, 2 layers, 4 heads, context=128, byte vocabulary 262)

### 3.2 Tokenizer and Byte Encoding
- `genesis_model/tokenizer.py` — Byte-level tokenizer with BOS=256, EOS=257, PAD=259

### 3.3 Transformer Architecture
- Attention layers, positional embeddings, output projection in `genesis_model/model.py`

### 3.4 Training
- `genesis_model/train.py` — Training loop, loss computation, AdamW optimizer
- `scripts/local_train.py` — Sentence memorization training script
- `scripts/train_preset.py` — Training presets

### 3.5 Optimizers and Scheduling
- AdamW with configurable learning rate and weight decay

### 3.6 Corpus Ingestion and Sampling
- `genesis_model/corpus.py` — Corpus loading and sampling
- `genesis_model/data.py` — Data utilities
- `scripts/fetch_gutenberg.py` — Gutenberg corpus downloader
- `scripts/tokenize_corpus.py` — Corpus tokenizer

### 3.7 Dataset Provenance and Manifests
- `data/sft/tool_use.jsonl` — Training data file (modified)
- `data/local/master-prompt-subset/` — Training subset
- Dataset manifests in various reports

### 3.8 Checkpoint Loading/Saving
- `genesis_model/checkpoint.py`

### 3.9 Generation and Decoding
- `genesis_model/generate.py` — Greedy decoding, temperature, EOS handling
- `genesis_model/tokenizer.py` — Byte-level tokenizer

### 3.10 EOS/BOS Handling
- BOS=256, EOS=257, PAD=259 in tokenizer

### 3.11 Native NumPy Runtime
- `genesis_model/native_*.py` (referenced in task descriptions)

### 3.12 Configuration
- `genesis_model/config.py` — GenesisConfig, SeedConfig, model hyperparameters

### 3.13 Evaluation
- `genesis_model/eval.py`
- `scripts/eval_heldout.py`

### 3.14 Agent Loop and Tools
- `genesis_model/agent.py`
- `genesis_model/tools.py`

### 3.15 Server
- `genesis_model/server.py`

---

## 4. Files Modified (9)

| File | Change Description |
|------|---------------------|
| `data/sft/tool_use.jsonl` | Supervised fine-tuning tool use corpus |
| `docs/EVIDENCE.md` | Evidence documentation for Genesis experiments |
| `docs/GAP_ANALYSIS.md` | Gap analysis between existing implementation and requirements |
| `docs/GENESIS_STATUS.yaml` | Genesis development status tracking |
| `docs/MODEL_CARD_TEMPLATE.md` | Model card template for GenesisSeed |
| `genesis_model/server.py` | HTTP server for Genesis inference |
| `genesis_model/tokenizer.py` | Byte-level tokenizer implementation |
| `scripts/local_train.py` | Sentence memorization training script |
| `tests/test_tokenizer.py` | Tokenizer test suite |

---

## 5. Files Added Untracked (14)

| File | Description |
|------|-------------|
| `data/sft/tool_use.jsonl.seed5.bak` | Backup of tool use corpus |
| `docs/GENESIS_BUILD_DIRECTION_2026-10-23.md` | Build direction notes |
| `docs/heldout_local_1m_clean.json` | Clean heldout dataset |
| `genesis_model/aetherius_bridge.py` | Agent bridge integration |
| `reports/SFT_STAGE_SCOPE_2026-10-25.md` | SFT stage scope report |
| `reports/WEEK1_CRITICAL_PATH_RESULTS_2026-10-25.md` | Week 1 results report |
| `reports/eval_heldout_local_1m_gutenberg_2026-10-25.json` | Evaluation results JSON |
| `resources/datasets/tools/Genesis_v0_6_Real_Dataset_Helper.zip` | Dataset helper archive |
| `scripts/eval_heldout.py` | Held-out evaluation script |
| `scripts/extend_context.py` | Context extension utility |
| `scripts/gen_sft_corpus.py` | Generate SFT corpus utility |
| `scripts/sft_tool.py` | Tool use generation script |
| `scripts/validate_sft.py` | SFT validation script |
| `tests/test_heldout_overlap.py` | Heldout overlap test |

---

## 6. Repository Identification

| Property | Value |
|----------|-------|
| Owner | jayprophit |
| Repository | Genesis |
| GitHub URL | https://github.com/jayprophit/Genesis |
| Default branch | main |
| Visibility | Public |
| GitHub CLI permission | ADMIN |

This repository already exists and is accessible. The local git repository at `C:\Users\jpowe\Desktop\Projects\Genesis\Genesis-Model` is associated with this remote.

---

## 7. Git Status Summary

| Category | Count |
|----------|-------|
| Modified files | 9 |
| Untracked files | 14 |
| Total files | 545 |
| Total directories | 157 |
| Git remote | origin (jayprophit/Genesis) |
| Branch | main |

---

## 8. Reports and Experiments Included

| Report File | Description |
|-------------|-------------|
| `GENESIS_LEVEL2_FINAL_VERIFICATION.md` | Level 2 final verification |
| `GENESIS_LEVEL3_COMPOSITIONAL_GENERALIZATION.md` | Level 3 compositional generalization |
| `GENESIS_LEVEL3_CORRECTED_EVALUATION.md` | Level 3 corrected evaluation |
| `GENESIS_LEVEL3B_REPRODUCIBILITY.md` | Level 3B seed reproducibility |
| `GENESIS_LEVEL3C_LEARNING_DIAGNOSIS.md` | Level 3C learning diagnosis |
| `GENESIS_MULTI_SENTENCE_LEARNING_REPORT.md` | Multi-sentence learning results |
| `GENESIS_MULTI_SENTENCE_GENERATION_DIAGNOSTIC.md` | Multi-sentence generation diagnostic |
| `GENESIS_TINY_OVERFIT_REPORT.md` | Tiny overfit experiment |
| `GENESIS_V06_MECHANISM_AUDIT_20261024.md` | v0.6 mechanism audit |
| `GENESIS_REORGANIZATION_REPORT.md` | Reorganization report |
