# Genesis model card — local_1m (Gutenberg corpus, clean run)

**Model:** Genesis-1 local_1m
**Checkpoint:** `runs/local_1m_clean/genesis.pt`
**SHA-256 (checkpoint file):** `076b23b22a62774853e0e8255a2217d1838bc9863cc0b8af894c3b1a7af5f9f2` (14,726,095 bytes)
**Corpus shard:** `runs/local_1m_clean/corpus.bin`, 50,532,564 bytes, 12,633,141 tokens, SHA-256 `e5f8aaa6a9f25bc8bf384d54f570655e85c367651101f855b48d1eedbcdf237f`; the checkpoint records `corpus_sha256 = 9ea3c5dcb1461863f2f11ae5fce304ddc8f6683846fa9667de594c2761adedf0` (the training engine's corpus digest, not a plain file hash)
**Date:** 2026-10-25
**Owner:** Genesis first-party

## 1. What this model is

A from-random dense decoder with **1,217,664 unique parameters**: 693,376
non-embedding parameters plus a 524,288-element token embedding tied to the
output projection (`lm_head.weight is token_embedding.weight`; summing
`state_dict` tensors double-counts the tied matrix and gives 1,741,952, which
earlier revisions reported by mistake). It uses a byte-level BPE tokenizer
(vocabulary 4096), RMSNorm, RoPE, GQA (4 heads / 1 KV head per layer), a SwiGLU
MLP (`d_ff=344`), tied embeddings, and a maximum sequence length of 256. The
vision tower (`vision=None` for this preset) exists in the architecture but is
not activated. Trained entirely locally on an NVIDIA GTX 1050 Ti (compute
capability 6.1, FP32, `torch==2.14.1+cu126`); no cloud API or rented GPU was
used.

## 2. Training

| Item | Value |
|---|---|
| Tokens seen | 19,996,672 (target 20,000,000; 2,441 steps) |
| Compute | 866.2 s wall time, 28,474.9 tok/s at the last step, 783.8 MB peak VRAM |
| Hardware | Intel Core i7-870, 16 GB RAM, NVIDIA GTX 1050 Ti 4 GB (`sm_61`, FP32) |
| Data | 57 public-domain Gutenberg books: 40,190,580 bytes, 38,728,147 characters after newline normalisation, 12,633,141 tokens. Provenance in `runs/local_1m_clean/shard_manifest.json` (per-file SHA-256, corpus digest, tokenizer digest). |
| Held out | `pg1342` (Pride and Prejudice) and `pg1399` (Anna Karenina), copied to `data/local/gutenberg_heldout/`, excluded from the 59-file source folder by name for every training run. |
| Objectives | next-token (`text_ce`), image caption (`vision_ce=0`), tool-use SFT (`sft_ce=0.0024`) |
| Vocab | 4096 (byte-level BPE: 256 byte tokens + 16 control tokens + merges learned from the training corpus) |
| Batch / context / grad accum | `batch_size=32`, `seq_len=256`, `grad_accum=1` |
| Checkpoint meta | `format_version=1`, `model_config=local_1m`, `vision=None`, `max_seq_len=256`, `n_layers=4`, `d_model=128`, `tie_embeddings=true`, `norm_eps=1e-6` |
| Reproducibility | corpus digest and tokenizer digest are checked before any `--reuse`/`--resume`; a shard without a manifest is refused unless `--trust-shard` is passed |

## 3. Evaluation

The held-out evaluation was run on the complete held-out text, not a sample:
`python scripts/eval_heldout.py --checkpoint runs/local_1m_clean/genesis.pt
--device cuda --max-blocks 0 --output docs/heldout_local_1m_clean.json`.
It refuses to run if a held-out file's content digest appears in the training
plan. It reported `checked: true`, `overlaps: []`, 57 training files and both
held-out books excluded by name. Result file: `docs/heldout_local_1m_clean.json`.

| Metric | Value | Dataset / notes |
|---|---|---|
| Held-out loss | `3.467427` | Both books combined, 885,447 scored tokens in 3,459 blocks |
| Held-out perplexity | `32.0542` | Same text and tokenizer; comparable only to runs using this tokenizer |
| Held-out bits per byte | `1.638118` | Same text; byte count measured from the decoded blocks |
| Held-out loss, `pg1342` | `3.401453`, ppl `30.0077`, bpb `1.550229` | 231,079 scored tokens |
| Held-out loss, `pg1399` | `3.490284`, ppl `32.7953`, bpb `1.670540` | 654,367 scored tokens |
| Training loss (final step) | `text_ce=3.4716`, `total=3.4717` | Training shard (`training_manifest.json`) |
| Peak VRAM | `783.8` MB | `torch.cuda.max_memory_allocated()` |
| Training throughput | `28,474.9` tok/s | Last logged step (2,441) |
| Decode throughput | `5.04` tok/s | 32 tokens in 6.35 s, single-stream generation on the GTX 1050 Ti |
| Tool format validity | Not measured | Synthetic/held-out measurement still pending (`TODO-GEN-006`) |

The held-out loss is slightly better than the final training loss. That is
plausible for a model this small that has not memorised the corpus, but it does
not mean the model is good: it learns vocabulary and local style, not reasoning.
No benchmark or quality claim beyond these measurements is made.

## 4. Intended use

Local first-party assistant research, estate automation experiments and
data-pipeline verification. Not a competitive product assistant. At 1.22M
parameters and with a classic-literature corpus it produces locally plausible
fragments and weak long-range structure. The checkpoint can be resumed
(`--resume`) and evaluated without cloud access.

## 5. Limitations

- Very small parameter count (1,217,664 unique). It cannot match hosted
  assistants and should not be presented as if it could.
- Corpus is public-domain classic literature; no modern domain knowledge, no
  licensed image-text pairs, no real-world documents from the master prompt or
  conversation corpus were fed to this run.
- The vision tower exists in the architecture but `vision=None` for this preset;
  no real image-text dataset was used.
- Tool-use format validity on held-out prompts is not measured. The SFT loss is
  near zero on a tiny synthetic set and is not evidence of tool reliability.
- No multi-hour stability test; thermal behaviour over days is unknown.
- Single-stream decoding is slow (about 5 tokens/s), so this checkpoint is not
  suitable for interactive use without a better serving path.
- Agent-Bridge / C++ organism (`Genesis Organism`) wiring remains external; this
  training pipeline does not integrate it.
- Speech, native C++20 runtime, distributed training, MoE and the larger
  `local_15m`/`local_30m` runs are not implemented or not executed.
- Master prompt full set (115 files, 10.86 MB) is indexed but not fed to this
  model; the 10-file subset was prepared separately and is not in this shard.
- Conversation transcripts are not registered as verified sources.

## 6. Provenance / registry

- Corpus source: Project Gutenberg (`https://www.gutenberg.org/ebooks/{id}`);
  public domain (author/translator died before 1947, editions before 1929).
- Corpus manifest: `data/local/gutenberg_manifest.jsonl` (JSONL, SHA-256 per
  file, retrieval date, clean character count).
- Shard provenance: `runs/local_1m_clean/shard_manifest.json`.
- Held-out report: `docs/heldout_local_1m_clean.json`.
- Master prompt subset manifest: `data/local/master-prompt-subset/manifest.jsonl`.
- Project todos: `Aetherius-OS/docs/PROJECT-TODOS.yaml` (`TODO-GEN-006`).
- Evidence: `docs/EVIDENCE.md` (exact commands, loss curve, checkpoint meta,
  held-out report).
- Gap analysis: `docs/GAP_ANALYSIS.md`.

## 7. Reproduction commands (verified)

```powershell
# Check CUDA / doctor
python -m genesis_model.cli doctor

# Train the clean checkpoint (new shard; writes shard_manifest.json)
python scripts/local_train.py --corpus data/local/gutenberg `
  --exclude pg1342.txt --exclude pg1399.txt `
  --preset local_1m --tokens 20000000 --device cuda --out runs/local_1m_clean

# Inspect checkpoint
python -m genesis_model.cli inspect --checkpoint runs/local_1m_clean/genesis.pt

# Generate
python -m genesis_model.cli generate --checkpoint runs/local_1m_clean/genesis.pt `
  --prompt "The quick brown fox" --max-new-tokens 60 --temperature 0.7 --device cuda

# Held-out evaluation (full text, digest-overlap checked)
python scripts/eval_heldout.py --checkpoint runs/local_1m_clean/genesis.pt `
  --device cuda --max-blocks 0 --output docs/heldout_local_1m_clean.json
```

All numbers above come from the checkpoint's own metadata, the training
manifest, the held-out report, or `doctor` output. Nothing is estimated from a
different run.
