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

## Next evidence required

- a licence-cleared local corpus (10–100 MB) with a source manifest and hashes.
- a full `local_1m` run on that corpus, then `local_3m`/`local_6m`, with loss curve.
- held-out bits-per-byte and a qualitative sample review.
- a long-run stability check: at least several hours, thermal and checkpoint behaviour.
- tool-format validity on a held-out tool set.
- a published model card for the resulting checkpoint.
- later, optionally: a 2B-token corpus and a `seed_30m` run on larger hardware.

