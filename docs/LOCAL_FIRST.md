# Local-first plan — train and run Genesis without paying per token

The first Genesis model is trained and used **on this machine**. No API key, no
per-token bill, no data leaving the computer. Cloud GPUs are a later option for a
larger model, not a dependency for the first one.

## 1. What the machine can actually do

| Resource | Value | Consequence |
|---|---|---|
| CPU | Intel Core i7-870, 4 cores / 8 threads, 2.93 GHz | Fine for data work and a 1M–3M model; slow for anything larger |
| RAM | 16 GB | Fine for tokenization and small-model training |
| GPU | GTX 1050 Ti, 4 GB, compute capability 6.1 | All local presets fit; measured 1M–30M training works |
| GPU features | Pascal, no tensor cores, no BF16 | Use FP32; FP16 is not a speed win on this card |
| Storage | ~1.3 TB free | Enough for local corpora and checkpoints |
| PyTorch | 2.14.1+cu126 | Verified: `cuda_available=true`, device capability 6.1 |

The GPU is a real accelerator, but it is not a datacentre card. A 1M–6M model is
the sensible first quality target. A 15M–30M model is possible in about one day
for 300M tokens. A 100M+ model is still not a good use of this machine.

## 2. Local presets

All are text-first. The architecture still supports vision; a local vision tower
is added after the text model works.

| Preset | Parameters at preset vocabulary | Width / layers | Context | Default batch |
|---|---:|---|---:|---:|
| `local_1m` | ~1.2M | 128 / 4 | 256 | 32 |
| `local_3m` | ~3.1M | 192 / 4 | 384 | 32 |
| `local_6m` | ~6.3M | 256 / 6 | 512 | 16 |
| `local_15m` | ~15.5M | 384 / 8 | 512 | 8 |
| `local_30m` | ~26.7M | 512 / 8 | 512 | 4 |

The parameter counts above assume each preset's vocabulary (`local_1m` uses 4096;
the others use 8192). A small corpus may stop BPE earlier, which lowers the
embedding and final-layer cost; the measured runs below used vocabulary 751 and
therefore fewer parameters.

The token budget is roughly 20× parameters, capped at 300M tokens:

| Preset | Default tokens | Approx. ideal FLOPs |
|---|---:|---:|
| `local_1m` | 20M | 1.4e14 |
| `local_3m` | 60M | 1.1e15 |
| `local_6m` | 120M | 4.5e15 |
| `local_15m` | 300M | 2.8e16 |
| `local_30m` | 300M | 4.8e16 |

### Measured throughput (2026-10-23)

These are real short-run measurements on this machine, not estimates. They were
taken with PyTorch 2.14.1+cu126, FP32 (Pascal), `OMP_NUM_THREADS=1`, and a 157 KB
repeated seed corpus (vocabulary 751). Steady-state `tok_s` is the last logged
step after warm-up; peak VRAM is `torch.cuda.max_memory_allocated()`.

| Preset | Actual parameters | Batch × context | Steady tok/s | Peak VRAM | Default tokens | Projected GPU time |
|---|---:|---:|---:|---:|---:|---:|
| `local_1m` | 789,504 | 32 × 256 | 38,000–41,000 | 482 MB | 20M | 8–12 min |
| `local_3m` | 1,718,784 | 32 × 384 | 34,000–35,000 | 1,021 MB | 60M | 30–40 min |
| `local_6m` | 4,348,928 | 16 × 512 | 9,000–14,000 | 1,399 MB | 120M | 2.5–4 h |
| `local_15m` | 12,681,216 | 8 × 512 | 6,200–6,600 | 1,666 MB | 300M | 13–16 h |
| `local_30m` | 22,937,600 | 4 × 512 | 3,100–3,400 | 1,515 MB | 300M | 25–32 h |

Caveats, stated plainly:

- these are throughput measurements, not model-quality benchmarks;
- the corpus was repeated seed text, so loss falls quickly and the numbers say
  nothing about generalisation;
- the measured models had vocabulary 751, not the preset vocabulary, so a
  full-vocabulary run will be somewhat slower and larger;
- projected times include training compute only. Tokenizer training, sharding,
  evaluation and checkpoint writes add minutes to hours depending on corpus size;
- the first logged step is slower because CUDA kernels and the data loader are
  warming up. Use the steady-state column, not step 1.

Start with `local_1m` for one evening. The measured 1M run is short enough to
iterate on corpus quality, then move to `local_3m` or `local_6m`.

## 3. What quality to expect

A 1M–6M model trained from scratch on a modest local corpus will:

- learn spelling, punctuation, common phrases and some local style;
- complete short prompts and produce partly coherent sentences;
- imitate the format of its training data;
- fail at reasoning, facts, long context and reliable instructions.

A 15M–30M model with 100M+ tokens can produce noticeably better short prose and
simple dialogue, but it is still not comparable to a hosted assistant. That is
the honest ceiling of this hardware. The point of the first local model is to
own the full pipeline and prove it on real text, then scale the same code later.

## 4. Get a corpus

Use text you have the right to train on. Good local sources:

- public-domain books (Project Gutenberg, Standard Ebooks);
- Wikipedia / Wiktionary dumps under their licences;
- your own notes and documents, with consent;
- permissively licensed code and documentation.

Start with **10–100 MB of clean text**. More data helps, but duplicated or dirty
data hurts. Keep a manifest with source, licence and SHA-256. The training script
hashes every source file automatically.

Put files in a folder:

```text
C:\Users\jpowe\Desktop\Projects\Genesis-Model\data\local\books\
```

Supported: `.txt`, `.md`, `.jsonl`, `.json`. JSONL records need a `text` or
`content` field.

## 5. Train

Check the GPU once:

```powershell
python -c "import torch; print(torch.__version__, torch.cuda.is_available(), torch.cuda.get_device_name(0) if torch.cuda.is_available() else 'cpu')"
```

One command does tokenizer training, shard writing, training and checkpointing:

```powershell
python scripts\local_train.py `
  --corpus data\local\books `
  --preset local_1m `
  --tokens 20000000 `
  --device cuda `
  --out runs\local_1m
```

For a bounded session:

```powershell
python scripts\local_train.py `
  --corpus data\local\books `
  --preset local_3m `
  --tokens 60000000 `
  --max-minutes 240 `
  --device cuda `
  --out runs\local_3m
```

If the time limit is reached, the checkpoint is still written. Continue with the
same command plus `--resume`:

```powershell
python scripts\local_train.py `
  --corpus data\local\books `
  --preset local_3m `
  --tokens 60000000 `
  --max-minutes 240 `
  --device cuda `
  --out runs\local_3m `
  --resume runs\local_3m\genesis.pt
```

Resume restores the model, tokenizer, optimizer state, step count, token count
and history. It verifies that the shard and tokenizer match the checkpoint.

## 6. Use it

Interactive chat, fully local:

```powershell
python -m genesis_model.cli chat --checkpoint runs\local_1m\genesis.pt --device cuda
```

Single generation:

```powershell
python -m genesis_model.cli generate `
  --checkpoint runs\local_1m\genesis.pt `
  --prompt "The house at the end of the road" `
  --max-new-tokens 120
```

Local HTTP server, no cloud:

```powershell
python -m genesis_model.cli serve --checkpoint runs\local_1m\genesis.pt --port 8420
```

The agent loop is available too, but tool use should be evaluated only after the
model has seen enough tool-format SFT data:

```powershell
python -m genesis_model.cli agent `
  --checkpoint runs\local_1m\genesis.pt `
  --task "List the files in this workspace"
```

## 7. 4 GB memory rules

If CUDA runs out of memory:

1. Halve `--batch-size`.
2. Halve `--seq-len`.
3. Increase `--grad-accum` to keep the effective batch size.
4. Use a smaller preset.
5. Close browser windows that hold GPU memory.

The local presets fit in 4 GB at the default batches. Measured peaks were 482 MB
to 1.67 GB. The desktop already uses several hundred MB, so check
`nvidia-smi` and leave headroom; `local_30m` at batch 4 peaked at 1.52 GB.

## 8. What comes after the first local model

1. Measure held-out bits-per-byte and review samples honestly.
2. Add real image-text data and enable the small vision tower.
3. Add tool-call SFT and measure format validity.
4. Quantize for faster CPU inference only if memory or speed demands it.
5. If a larger model is ever needed, the same code and checkpoint format run on
   a rented GPU; no cloud dependency is built into the product.

The local model is the product. Cloud is an optional larger training run later.
