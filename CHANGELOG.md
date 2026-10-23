# Changelog

## 0.1.0 — 2026-10-23

First scaffold of the independent Genesis model layer.

### Added

- Byte-level BPE tokenizer with 16 control tokens and exact multilingual round-trip.
- Word-frequency BPE training over unique chunks, with byte-level merge format.
- Memmap token shard writer and reader for corpora too large for Python lists.
- Training directly from a pre-tokenized shard (`--shard` + `--tokenizer`).
- Decoder: RMSNorm, RoPE, GQA, SwiGLU, tied embeddings.
- KV cache with full-forward equivalence.
- From-scratch vision tower (patch conv + ViT + projector).
- Native tool protocol, schema validation, approval gates and workspace confinement.
- Native agent loop with bounded steps and audit events.
- Training loop: text, image caption and tool SFT objectives; AMP; checkpoint resume; manifest.
- Training from memmap token shards (`scripts/tokenize_corpus.py`).
- Safe checkpoint format (`weights_only=True`).
- Local HTTP runtime with `/health`, `/v1/models`, `/v1/tools`, `/v1/chat/completions`, `/v1/agent`.
- Evaluation: bits-per-byte, tool-format accuracy, decode throughput.
- Local-first presets: `local_1m`, `local_3m`, `local_6m`, `local_15m`, `local_30m`.
- One-command local pipeline: corpus discovery, tokenizer, shard, training, checkpoint.
- True resume: model, tokenizer, optimizer, step/token counters and history.
- Interactive local chat (`genesis-model chat`) with no API or network.
- Time-limited runs that still write a resumable checkpoint.
- Throughput logging (`tok_s`) and peak VRAM in the training manifest.
- Pascal-aware AMP fallback: FP32 on the GTX 1050 Ti, BF16 on Ampere+.
- Verified CUDA training on the GTX 1050 Ti (`torch 2.14.1+cu126`, sm_61): all
  five local presets trained and checkpointed, with measured steady throughput
  from 41k tok/s (`local_1m`) to 3.4k tok/s (`local_30m`).
- Fixed top-p sampling on CUDA: zero and renormalise filtered probabilities
  instead of passing `-inf` weights to `torch.multinomial`.
- 29 tests and an end-to-end smoke test.
- Estate analysis and C01–C28 component specification.
- Local-first guide (`docs/LOCAL_FIRST.md`) with measured times and VRAM.

### Not included

- a trained product checkpoint;
- real-world image data;
- speech;
- native C++/CUDA kernels;
- organism and Agent-Bridge wiring.
