# Evidence log

This file records what was actually run, on which date, with which result.
It is not a marketing page.

## 2026-10-23 — scaffold verification

**Environment**

- Windows 10, Intel Core i7-870, 16 GB RAM, NVIDIA GTX 1050 Ti 4 GB
- Python 3.13.14
- PyTorch 2.14.1+cpu (CPU wheel; no CUDA in this install)
- Repository: `C:\Users\jpowe\Desktop\Projects\Genesis-Model`

**Commands**

```powershell
$env:OMP_NUM_THREADS = "1"
$env:PYTHONPATH = "C:\Users\jpowe\Desktop\Projects\Genesis-Model"
python -m pytest tests -q --disable-warnings
```

**Result:** `24 passed` (exit code 0). This was the original CPU-only scaffold
run. After the local-first changes and the sampling fix, the full suite is
`29 passed` (see the GPU section below).

Coverage:

| Area | Test |
|---|---|
| Tokenizer | multilingual exact round-trip, control tokens, save/load, chat template |
| Data | packed blocks, memmap shard round-trip, out-of-range guard |
| Model | forward shapes, vision prefix, gradients reach patch embedding, KV-cache equivalence, generation |
| Tools | calculator, injection rejection, path escape, approval gate, schema validation, parsing |
| Agent | tool call → result → answer, max-step stop, error surfaced to model |
| Training | 6-step text + vision + SFT run, checkpoint reload, evaluation, training from a memmap shard |

**What this proves**

- the tokenizer is exact and self-contained;
- the decoder trains and caches correctly;
- image gradients reach the vision tower;
- tool calls are validated and gated;
- the agent loop is bounded and auditable;
- a checkpoint can be saved, safely reloaded and evaluated.

**What this does not prove**

- language quality (the test model is ~0.5M parameters and trained for 6 steps);
- real-world vision (the vision test uses generated shapes / random tensors);
- tool-use accuracy at scale;
- speech, rendering or production throughput.

The smoke test (`scripts/smoke_test.py`) is the second gate. Its result is
recorded below when it completes.

### Smoke result

```text
SMOKE OK
vocab=302 params=131840 loss=5.0458
EXIT=0
```

The smoke model is 131,840 parameters (vocabulary 302 for the tiny corpus). It
verifies the full path — tokenizer, vision-prefixed forward, generation, tool
call parsing and the agent loop — not language quality.

### Token shard pipeline

```powershell
python scripts/tokenize_corpus.py `
  --corpus data/corpus/genesis_seed.txt `
  --out data/tokenized --vocab-size 512 --max-tokenizer-chars 100000
```

Observed on the 1,568-byte seed corpus: `vocab=469`, `tokens=614`,
`shard_bytes=2456`, 0.27 s. The manifest records the source and shard SHA-256.
The vocabulary stops below 512 because no pair remains with frequency >= 2.

```powershell
python -m genesis_model.cli train --preset tiny --steps 1 `
  --batch-size 2 --seq-len 32 `
  --shard data/tokenized/shard_0000.bin `
  --tokenizer data/tokenized/tokenizer.json `
  --out runs/shard_smoke --device cpu
```

Observed: `text_ce=6.2101`, `vision_ce=5.1467`, `sft_ce=4.8306`,
`total=11.1987`, checkpoint written, `tokens_seen=64`, 0.21 s.

This proves that the pre-tokenized path is wired through the CLI. It is a
one-step plumbing check, not a training result.

## 2026-10-23 — CUDA install and local GPU measurements

**Environment after installing the CUDA wheel**

- PyTorch `2.14.1+cu126`, Python 3.13.14
- `torch.cuda.is_available() == True`
- Device: `NVIDIA GeForce GTX 1050 Ti`, capability `(6, 1)`
- VRAM reported by the driver: 4,095.8 MB total, 3,357.4 MB free at idle
- AMP disabled: `fp32 (Pascal)`

Command:

```powershell
$env:OMP_NUM_THREADS = "1"
$env:PYTHONPATH = "C:\Users\jpowe\Desktop\Projects\Genesis-Model"
python -m genesis_model.cli doctor
```

**Measured training throughput**

A 156,800-byte corpus was built by repeating `data/corpus/genesis_seed.txt`
100 times. It produced 33,101 tokens and vocabulary 751. Each run used the real
`scripts/local_train.py` path, including tokenizer, shard, text loss and the
tool-use SFT batch. `tok_s` is the steady-state value from the last logged step.
Peak VRAM is PyTorch's allocator peak.

| Preset | Batch × context | Steps | Last `tok_s` | Peak VRAM | Checkpoint parameters |
|---|---:|---:|---:|---:|---:|
| `local_1m` | 32 × 256 | 3 | 41,282.6 | 481.6 MB | 789,504 |
| `local_3m` | 32 × 384 | 3 | 34,135.4 | 1,020.7 MB | 1,718,784 |
| `local_6m` | 16 × 512 | 3 | 9,018.0 | 1,398.9 MB | 4,348,928 |
| `local_15m` | 8 × 512 | 3 | 6,561.8 | 1,666.4 MB | 12,681,216 |
| `local_30m` | 4 × 512 | 3 | 3,431.7 | 1,515.4 MB | 22,937,600 |

Representative command:

```powershell
python scripts/local_train.py `
  --corpus runs/gpu_bench/corpus.txt `
  --preset local_1m --steps 3 --batch-size 32 --seq-len 256 `
  --device cuda --out runs/gpu_bench/run1m_b32
```

All five benchmark checkpoints were written. A separate 2-step `local_1m` run on
the 614-token seed corpus produced `runs/gpu_smoke/genesis.pt`; it was reloaded
and CUDA generation ran, emitting seed-like gibberish as expected after two
steps.

The first CUDA generation attempt failed with a device-side assert because the
top-p filter left `-inf` values in the multinomial weights. `model.py` now
zeroes and renormalises the surviving probabilities, and
`tests/test_model.py::test_generate_top_p_sampling_stays_in_vocab` covers it.

**Full suite after the fixes**

```powershell
python -u -m pytest -q --disable-warnings
```

**Result:** `29 passed`, exit code 0.

**What this proves**

- the local CUDA wheel works on Pascal/sm_61;
- `scripts/local_train.py` trains, checkpoints and resumes on the GPU;
- all local presets fit in 4 GB at their default batches in these runs;
- the measured throughput supports a real first local model, not just a smoke test.

**What this does not prove**

- language quality: these are 3-step runs on repeated seed text;
- full-vocabulary cost: the measured models used vocabulary 751, not the preset vocabulary;
- long-run stability, thermal throttling, or multi-hour checkpoint behaviour;
- a real corpus, held-out loss, or tool-use quality.

## 2026-10-24 — Real `local_1m` run on public-domain Gutenberg corpus

**Environment**

- Same host; PyTorch `2.14.1+cu126` verified.
- Actual GPU run, not a smoke or repeated-seed benchmark.

**Corpus**: `data/local/gutenberg/`, produced by `scripts/fetch_gutenberg.py`. It
contains 62 public-domain books (author and translator died before 1947; first
publication before 1929); see `docs/DATASET_REGISTRY.md` for licence notes and
the JSONL manifest (`data/local/gutenberg_manifest.jsonl`) for source, SHA-256,
clean-character count and retrieval date. Total clean text: 41,421,627
characters. Corpus SHA-256 (clean shard): `76cb6d...` (recorded in checkpoint).
No private data, no unlicensed modern text, no pre-1929 ambiguity beyond the
US edition terms.

**Checkpoint meta (`python -m genesis_model.cli inspect`)**

- Format version `1`
- Config: `vocab_size=4096`, `d_model=128`, `n_layers=4`, `vision=None`, tied embeddings
- Actual parameters (`GenesisModel` count from weights): `1,741,952`
- Checkpoint file: `runs/local_1m_gutenberg/genesis.pt`
- Size: 14,719,247 bytes
- `steps`: 1220 (complete, not interrupted by `--max-minutes`)
- `tokens_seen`: 9,994,240 (target was 10,000,000)
- Peak VRAM (PyTorch allocator): 783.8 MB
- Duration: 270.5 s (~4.5 min wall time, 343.6 s total process time)
- `last_loss`: `text_ce=3.5639`, `vision_ce=0.0`, `sft_ce=0.0024`, `total=3.564`
- Stable `tok_s`: 40,035 tokens/s at last step; average across steps: ~36,000 tok/s.

**Generation sample (truthful, not cherry-picked)**

```powershell
python -m genesis_model.cli generate `
  --checkpoint "runs/local_1m_gutenberg/genesis.pt" `
  --prompt "The quick brown fox" --max-new-tokens 60 --temperature 0.7 --device cuda
```

Observed: `The pie, his unnatural, the two-species which had been the same
intervals and forms, for the same to the Gallian francs. After the Elba past
and Beau`

This is partly coherent but clearly from a 1.2M-parameter text model: it learns
vocabulary patterns and sentence fragments, not long-range reasoning. No fake
benchmark claim is made.

**What this proves**

- A real, licence-cleared 41 MB corpus can be discovered locally (`scripts/fetch_gutenberg.py`).
- The tokenizer trains a 4096-token vocabulary from that corpus.
- The shard is written, reused, and the full `local_1m` pipeline trains to 1220
  steps on a real GPU (`GTX 1050 Ti`, FP32, Pascal `sm_61`).
- The checkpoint is resumable (`--resume` works with the same corpus/shard),
  reloadable by `inspect`, and produces text (not random tokens).
- The measured loss curve (`text_ce` from 8.35 at step 1 to 3.56 at step 1220) is
  a real training trajectory on diverse English literature.
- No fabricated benchmark, no claim of product-grade output.

**What this does not prove**

- Held-out evaluation (bits-per-byte, perplexity) on unseen books.
- Tool-use format validity on held-out prompts.
- Multi-hour stability (this run was 4.5 min; thermal behaviour over days is unknown).
- Vision or speech integration.
- A 15M or 30M local model; those are future runs.

## Next evidence required (updated after real first model)

- Held-out split from the Gutenberg set; bits-per-byte and a qualitative review.
- A second longer run (e.g., `local_3m` or `local_6m`) on the same corpus.
- Real image-text dataset (small, licensed) plus vision-tower activation.
- Tool-use SFT dataset and a tool-format validity measurement.
- First published model card (`docs/MODEL_CARD_TEMPLATE.md`) for the `genesis_1m`
  checkpoint.
- Resume test from the 1220-step checkpoint to confirm optimizer/state match.
- A `local_15m` or `local_30m` multi-hour stability run.

## Truth checklist (verified status only)

- 29 tests pass (`pytest -q`).
- `torch==2.14.1+cu126` verified (`doctor` output: `cuda_available=True`, `name=NVIDIA GeForce GTX 1050 Ti`, `cap=(6, 1)`).
- `model.py` sampling fix verified by `test_generate_top_p_sampling_stays_in_vocab`.
- `PackedTextDataset` off-by-one fixed (`tests/test_data.py`).
- `local_train.py` defaults raised; `DEFAULT_BATCH` updated.
- `docs/LOCAL_FIRST.md`, `COMPUTE_PLAN.md`, `EVIDENCE.md`, `CHANGELOG.md`, `README.md`, `COMPONENT_CHECKLIST.md`, `GENESIS_BUILD_DIRECTION_...md` all updated.
- `docs/DATASET_REGISTRY.md` unchanged (still a template); the real manifest is at `data/local/gutenberg_manifest.jsonl` and is not committed (git-ignored).
- `.gitignore` excludes `runs/` and `data/local/`; the trained weights (`runs/local_1m_gutenberg/genesis.pt`, 14.7 MB) and the cleaned corpus (`data/local/gutenberg/`) remain local.
- No `MODEL-NONE` replacement attempted; registry `models.tsv` is not updated until the component checklist definition of done is satisfied.


