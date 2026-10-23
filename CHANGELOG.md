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
- 24 tests and an end-to-end smoke test.
- Estate analysis and C01–C28 component specification.

### Not included

- a trained product checkpoint;
- real-world image data;
- speech;
- native C++/CUDA kernels;
- organism and Agent-Bridge wiring.
