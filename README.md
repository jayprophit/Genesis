# Genesis Model

**Genesis-1** — an independently trained multimodal model with native tool calling and its own agent runtime.

> Genesis is not a fine-tune, wrapper, rename or distillation of another released model. It starts from random weights, uses a tokenizer trained by this project, and is trained and evaluated on data with recorded provenance.

This repository is the missing **model layer** of the Genesis estate. It is deliberately separate from:

- `Genesis` (C++ organism: identity, memory, development, authority, tool execution);
- `Agent-Bridge` (external tools, devices, browsers, providers — optional);
- `Aetherius-OS` (host/supervisor);
- `IDE-Workspace` (UI).

The full estate analysis, global model-landscape review and component-by-component specification are in [`docs/ECOSYSTEM_ANALYSIS_2026-10-23.md`](docs/ECOSYSTEM_ANALYSIS_2026-10-23.md). The regional deep-dive is in [`docs/GLOBAL_MODEL_LANDSCAPE.md`](docs/GLOBAL_MODEL_LANDSCAPE.md), and the hardware/cost plan is in [`docs/COMPUTE_PLAN.md`](docs/COMPUTE_PLAN.md).

---

## What is implemented

| Component | Status | Evidence |
|---|---|---|
| Byte-level word-frequency BPE tokenizer, 16 control tokens, multilingual round-trip | **Implemented** | `tests/test_tokenizer.py` |
| Decoder: RMSNorm, RoPE, GQA, SwiGLU, tied embeddings | **Implemented** | `tests/test_model.py` |
| KV cache with full-forward equivalence | **Implemented** | `tests/test_model.py` |
| From-scratch vision tower fused into the text sequence | **Implemented** | gradient + shape tests |
| Native tool protocol (`<|tool_call|>`), schema validation, approvals | **Implemented** | `tests/test_tools.py` |
| Native agent loop (plan → tool → observe → answer) | **Implemented** | `tests/test_agent.py` |
| Data pipeline: packed text, memmap token shards, image captions, chat SFT | **Implemented** | `tests/test_data.py` |
| Training loop: text + image caption + tool SFT, AMP, resume, manifests | **Implemented** | `tests/test_training.py` |
| Checkpoint format with safe `weights_only` loading | **Implemented** | `tests/test_training.py` |
| Local HTTP runtime (`/health`, `/v1/chat/completions`, `/v1/agent`, `/v1/tools`) | **Implemented (minimal)** | `genesis_model/server.py` |
| Evaluation: bits-per-byte, tool-format accuracy, throughput | **Implemented (baseline)** | `genesis_model/eval.py` |
| Trained production checkpoint | **Not yet** | requires corpus + GPU run |
| Real-world vision / speech / native C++ kernels | **Not yet** | roadmap phases 2–4 |

No claim in this repository should be read as a benchmark result unless it names a checkpoint, dataset, command and date.

---

## Quick start

```powershell
cd C:\Users\jpowe\Desktop\Projects\Genesis-Model
$env:PYTHONPATH = (Get-Location)
python -m pytest tests -q

# Correctness-scale training on CPU (a few seconds to a minute)
python -m genesis_model.cli train --preset tiny --steps 40 --out runs/tiny

# Generate (the tiny model is not expected to be coherent)
python -m genesis_model.cli generate --checkpoint runs/tiny/genesis.pt --prompt "Hello"

# Native agent with built-in tools
python -m genesis_model.cli agent --checkpoint runs/tiny/genesis.pt --task "What is 17 * 24?"

# Inspect / evaluate / serve
python -m genesis_model.cli inspect --checkpoint runs/tiny/genesis.pt
python -m genesis_model.cli eval --checkpoint runs/tiny/genesis.pt
python -m genesis_model.cli serve --checkpoint runs/tiny/genesis.pt --port 8420
```

The tiny preset exists to verify gradients, caching, checkpointing and the agent control flow. It is **not** a useful assistant. `seed_30m` and `seed_120m` are the first real training targets.

---

## Architecture

```text
UTF-8 ── byte-level BPE ──┐
                          ├── shared decoder workspace ── LM head
image ── patch conv ── ViT ┘          │
                                      ├── tool-call tokens ── parser ── policy ── tools
                                      └── KV cache ── streaming runtime
```

- **Tokenizer:** 256 byte tokens + 16 control tokens + learned merges. No borrowed vocabulary.
- **Backbone:** pre-norm decoder, RMSNorm, rotary positions, grouped-query attention, SwiGLU, tied embeddings.
- **Vision:** convolutional patch embedding + ViT blocks + linear projector into the same sequence. Trained jointly, never a frozen pretrained encoder.
- **Tools:** `<|tool_call|>{"name":...,"arguments":{...}}<|/tool_call|>`, validated against JSON Schema, gated by risk and approval.
- **Agent:** bounded loop; tool results are observations, not instructions; refusals are recorded.
- **Runtime:** local HTTP API, OpenAI-compatible chat shape plus a native `/v1/agent` endpoint.

### Presets

| Preset | Parameters (approx.) | Context | Purpose |
|---|---|---|---|
| `tiny` | 0.5M | 256 | CI and correctness |
| `seed_30m` | 30M | 2k | first real training run |
| `seed_120m` | 120M | 4k | first usable local model |
| `genesis_1b` | 1B | 8k | scale target (rented compute) |

---

## Independence policy

**Required**

1. Random initialisation only. No `from_pretrained`, no third-party weight files.
2. Tokenizer trained by this project.
3. Training data with licence, URL, hash and consent basis.
4. Our own checkpoints, logs and evaluation.
5. Every capability claim tied to a named checkpoint and reproducible command.

**Allowed**

- PyTorch/JAX as numerical libraries;
- published algorithms and papers;
- public datasets under their terms;
- studying open and closed models as references.

**Not allowed**

- renaming or fine-tuning Qwen/Llama/Claude/Gemini/DeepSeek/Kimi/Mistral and calling it Genesis;
- distillation from a closed API to create the base model;
- copying tokenizers, embedding tables or weights;
- training on private camera/mic data without separate explicit consent;
- claiming benchmark results that were not run.

---

## Tool safety

- Path tools are confined to an explicit workspace.
- `write_file` and `http_get` require approval; network is off by default.
- There is no shell tool in the default registry.
- Arguments are type-checked before a handler runs.
- Every call returns a structured result with an error code and duration.
- The model's tool call is a **proposal**; the runtime decides whether it may run.

The C++ Genesis authority/tool executor remains the production authority. This repository's registry is the local reference implementation and the integration seam.

---

## Roadmap

1. **Now:** correctness tests, tiny training, native agent loop, local runtime.
2. **Next:** 2B-token corpus manifest; train `seed_30m`; first model card with real numbers.
3. **Then:** 20B tokens for `seed_120m`; tool-use SFT; 8k context.
4. **Then:** real image-text/OCR data; C++/CUDA inference kernels; quantization.
5. **Later:** MoE/MLA experiments, GRPO for tool use, 1B–3B scale, speech from scratch.

---

## Licence

To be decided by the project owner. Until a licence is published, the default is
all rights reserved. Third-party datasets and libraries retain their own
licences; see the dataset registry before release.
