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

**Result:** `24 passed` (exit code 0).

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

## Next evidence required

- 2B-token corpus manifest with hashes and licences.
- `seed_30m` training run with loss curve and tokens/sec.
- held-out bits-per-byte and a qualitative sample review.
- tool-format validity on a held-out tool set.
- a published model card for the resulting checkpoint.

