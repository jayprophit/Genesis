# Compute plan — from a 4 GB GPU to a real Genesis checkpoint

> **Local-first addendum (2026-10-23):** the first model is trained on the local
> GTX 1050 Ti using `local_1m`–`local_30m`, not on rented compute. Measured
> steady throughput is 38k–41k tok/s for `local_1m`, 34k–35k for `local_3m`,
> 9k–14k for `local_6m`, 6.2k–6.6k for `local_15m`, and 3.1k–3.4k for
> `local_30m` (FP32, short runs). See [`LOCAL_FIRST.md`](LOCAL_FIRST.md) for the
> commands, memory measurements and caveats. This document covers the later
> `seed_30m`/`seed_120m`/1B scale path.

**Local machine:** Intel Core i7-870, 16 GB RAM, NVIDIA GTX 1050 Ti 4 GB, Windows 10.  
**Reality:** this machine can train the local presets through `local_30m`, run
inference on them, and do evaluation. The measured 300M-token `local_30m` run is
about one day, so a first real checkpoint is possible locally. It cannot train
`seed_120m` at useful speed; rented compute is an option only after the local
model and data pipeline are proven.

---

## 1. What the local machine is for

| Task | Feasible | Notes |
|---|---|---|
| Tokenizer training on a few GB | Yes | CPU, hours |
| `tiny` training / CI | Yes | minutes |
| `local_1m`–`local_30m` training | Yes | Measured 8 min to ~1 day for default token budgets |
| `seed_30m` inference (int8/int4) | Yes, slowly | 4 GB VRAM is tight; CPU offload |
| `seed_30m` training | Possible, but not the first target | Vision/extra capacity raise cost |
| `seed_120m` training | No | VRAM and throughput |
| Evaluation of a 100M model | Yes | batch 1, short prompts |
| Data curation, manifest, dedup | Yes | CPU/RAM bound |
| Native C++ kernel development | Yes | compile and unit test |

Use the local GPU as the **first real training and inference target**. It is slow
by datacentre standards, but it is fast enough to train a genuine 1M–30M model.

---

## 2. Target hardware and rough prices

Prices move; treat these as planning ranges, not quotes.

| Class | VRAM | Typical rented price | Good for |
|---|---|---|---|
| RTX 4090 | 24 GB | $0.35–0.70 / hr | 30M–400M training, fast iteration |
| RTX 5090 | 32 GB | $0.60–1.20 / hr | same, more headroom |
| A100 40 GB | 40 GB | $1.20–2.00 / hr | 400M–1.5B, stable throughput |
| A100 80 GB | 80 GB | $1.50–2.50 / hr | 1.5B dense / small MoE |
| H100 80 GB | 80 GB | $2.00–4.00 / hr | 1.5B–7B, FP8/BF16 |
| 8×H100 node | 640 GB | $16–30 / hr | 3B–7B |

Providers to compare: RunPod, Vast.ai, Lambda, Paperspace, Together, Modal, AWS/GCP/Azure spot. Choose by **availability, interconnect, storage location and egress**, not headline price.

---

## 3. Stage budgets

Training FLOPs ≈ `6 × N × D` (dense). With activation recomputation and optimiser overhead, multiply wall-clock by 1.5–2×. Assume 30–45% MFU on rented hardware.

### S0 — local presets, measured on the 1050 Ti

- `local_1m`, 20M tokens: **8–12 min** at 38k–41k tok/s, 482 MB peak VRAM.
- `local_3m`, 60M tokens: **30–40 min** at 34k–35k tok/s, 1,021 MB peak VRAM.
- `local_6m`, 120M tokens: **2.5–4 h** at 9k–14k tok/s, 1,399 MB peak VRAM.
- `local_15m`, 300M tokens: **13–16 h** at 6.2k–6.6k tok/s, 1,666 MB peak VRAM.
- `local_30m`, 300M tokens: **25–32 h** at 3.1k–3.4k tok/s, 1,515 MB peak VRAM.

These are short-run throughput measurements on repeated seed text, not quality
results. They establish that a real first checkpoint can be trained locally.

### S1 — `seed_30m`, 2B tokens

- FLOPs: `6 × 3.0e7 × 2.0e9 = 3.6e17`
- On a 4090 at ~100 TFLOP/s effective: ~1 hour of pure math; **3–6 hours** with data loading and checkpoints.
- Cost: **$2–5**.
- This is the first cloud run that can produce a model card worth publishing; a
  local `local_*` model card comes first.

### S2 — `seed_120m`, 20B tokens

- FLOPs: `6 × 1.2e8 × 2.0e10 = 1.44e19`
- On a 4090 at ~120 TFLOP/s: ~33 hours; **2–3 days** with overhead.
- On an A100 40 GB at ~180 TFLOP/s: ~22 hours.
- Cost: **$30–80**.
- Output: a genuinely usable small local model, especially with tool SFT.

### S3 — 1.5B dense, 300B tokens

- FLOPs: `6 × 1.5e9 × 3.0e11 = 2.7e21`
- At 400 TFLOP/s effective: ~1,875 H100-hours; **2,500–3,500 hours** with overhead.
- Cost: **$5k–12k** depending on price and utilisation.
- Requires 8×H100 or equivalent for reasonable wall-clock.

### S4 — 1.5B total / 0.4B active MoE, 300B tokens

- Compute uses active parameters: `6 × 4e8 × 3e11 ≈ 7.2e20` → ~500 H100-hours of math; **800–1,200 hours** with overhead.
- Memory needs all experts: 8×H100 for training.
- Cost: **$2k–5k**.
- This is the first architecture where MoE clearly pays for itself.

**Do not start at S3/S4.** The data pipeline and evaluation must be proven at S1/S2 first.

---

## 4. Data volume and storage

| Stage | Tokens | uint32 shards | uint16 shards |
|---|---|---|---|
| S1 | 2B | 8 GB | 4 GB |
| S2 | 20B | 80 GB | 40 GB |
| S3 | 300B | 1.2 TB | 600 GB |

- Store tokenized shards, not raw text, on the training host.
- Keep raw data and manifests in object storage in the same region to avoid egress.
- Checkpoint size: ~2 bytes/parameter at bf16 (plus optimizer state, ~8–12 bytes/parameter for Adam). A 1.5B model needs 3 GB weights, ~12–18 GB optimizer state, plus activations. Plan 100–200 GB of checkpoint storage for a long run.
- Save every 30–60 minutes; keep the last 3 and the best-by-validation.

---

## 5. Run protocol

1. **Freeze the corpus.** Write the manifest, hashes and tokenizer. No dataset changes after the run starts.
2. **Tokenize once.** Produce shards and a `dataset.json` with token count, dtype, EOS policy and language tags.
3. **Smoke first.** Run 20 steps on the rented machine; check loss, throughput, VRAM and checkpoint reload.
4. **Launch with resume.** Every checkpoint records step, tokens, optimizer, RNG and corpus hash.
5. **Monitor.** Loss, gradient norm, tokens/sec, GPU memory, temperature; alert on NaN/Inf.
6. **Evaluate on held-out data.** Bits-per-byte, tool format, task suite; never the training split.
7. **Transfer and wipe.** Encrypt the checkpoint, download, verify SHA-256, delete the rented volume.

---

## 6. Security on rented compute

- Never place long-lived API keys or personal data on the host.
- Use a dedicated SSH key per run and revoke it afterwards.
- Encrypt checkpoints before transfer; verify hashes on both sides.
- Do not install unpinned packages; use a lockfile and a container image.
- Assume the host is untrusted. The corpus and weights are the assets.

---

## 7. Realistic timeline

| Week | Work | Gate |
|---|---|---|
| 1 | Corpus manifest, licence review, tokenizer 32k, shard writer | tokenizer round-trip and shard reload |
| 2 | `seed_30m` 2B-token run, evaluation, model card | bits-per-byte beats bigram; samples reviewed |
| 3–4 | Tool SFT, agent evaluation, native runtime hardening | tool-format validity > 98% |
| 5–8 | `seed_120m` 20B-token run, real image-text data | usable local assistant; VQA/OCR baseline |
| 9–12 | C++/CUDA inference, quantization, organism wiring | parity with PyTorch; 4 GB inference |
| 13+ | Scale decision: MoE/MLA or stay small and specialise | evidence-based go/no-go |

---

## 8. Stop conditions

Pause and reassess if:

- loss is not falling after 2B tokens;
- throughput is below 50% of the planning estimate;
- the corpus cannot be licensed for the intended release;
- evaluation shows the model is not better than a well-prompted smaller baseline on the target tasks;
- costs exceed the approved budget without a checkpoint worth keeping.

A small, honest, well-evaluated model is a success. A large, unmeasured one is a liability.
