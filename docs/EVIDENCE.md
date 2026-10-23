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

**Result:** `20 passed` (exit code 0).

Coverage:

| Area | Test |
|---|---|
| Tokenizer | multilingual exact round-trip, control tokens, save/load, chat template |
| Model | forward shapes, vision prefix, gradients reach patch embedding, KV-cache equivalence, generation |
| Tools | calculator, injection rejection, path escape, approval gate, schema validation, parsing |
| Agent | tool call → result → answer, max-step stop, error surfaced to model |
| Training | 6-step text + vision + SFT run, checkpoint reload, evaluation |

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
vocab=307 params=132160 loss=4.9061
EXIT=0
```

The smoke model is 132,160 parameters (vocabulary 307 for the tiny corpus). It
verifies the full path — tokenizer, vision-prefixed forward, generation, tool
call parsing and the agent loop — not language quality.

## Next evidence required

- 2B-token corpus manifest with hashes and licences.
- `seed_30m` training run with loss curve and tokens/sec.
- held-out bits-per-byte and a qualitative sample review.
- tool-format validity on a held-out tool set.
- a published model card for the resulting checkpoint.

