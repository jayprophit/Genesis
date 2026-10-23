# Genesis Model

**Genesis-1** — an independently trained multimodal model with native tool calling and its own agent runtime.

> Genesis is not a fine-tune, wrapper, rename or distillation of another released model. It starts from random weights, uses a tokenizer trained by this project, and is trained and evaluated on data with recorded provenance.

This repository is the missing **model layer** of the Genesis estate. It is deliberately separate from:

- `Genesis` (C++ organism: identity, memory, development, authority, tool execution);
- `Agent-Bridge` (external tools, devices, browsers, providers — optional);
- `Aetherius-OS` (host/supervisor);
- `IDE-Workspace` (UI).

The full estate analysis, global model-landscape review and component-by-component specification are in [`docs/ECOSYSTEM_ANALYSIS_2026-10-23.md`](docs/ECOSYSTEM_ANALYSIS_2026-10-23.md). The regional deep-dive is in [`docs/GLOBAL_MODEL_LANDSCAPE.md`](docs/GLOBAL_MODEL_LANDSCAPE.md). The local-first training/use plan is in [`docs/LOCAL_FIRST.md`](docs/LOCAL_FIRST.md); the later hardware/cost plan is in [`docs/COMPUTE_PLAN.md`](docs/COMPUTE_PLAN.md).

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
| Local presets (`local_1m` … `local_30m`) sized for 4 GB VRAM | **Implemented** | `tests/test_local.py` |
| True resume: model, tokenizer, optimizer, step/token counters, history | **Implemented** | `tests/test_local.py` |
| One-command local pipeline (corpus → tokenizer → shard → train) | **Implemented** | `scripts/local_train.py` |
| Interactive local chat with no API or network | **Implemented** | `python -m genesis_model.cli chat` |
| Checkpoint format with safe `weights_only` loading | **Implemented** | `tests/test_training.py` |
| Local HTTP runtime (`/health`, `/v1/chat/completions`, `/v1/agent`, `/v1/tools`) | **Implemented (minimal)** | `genesis_model/server.py` |
| Evaluation: bits-per-byte, tool-format accuracy, throughput | **Implemented (baseline)** | `genesis_model/eval.py` |
| Trained production checkpoint | **Not yet** | needs a local corpus and a completed run |
| Real-world vision / speech / native C++ kernels | **Not yet** | roadmap phases 2–4 |

No claim in this repository should be read as a benchmark result unless it names a checkpoint, dataset, command and date.

---

## Quick start

```powershell
cd C:\Users\jpowe\Desktop\Projects\Genesis-Model
$env:PYTHONPATH = (Get-Location)
python -m pytest tests -q

# Check that the local GPU is visible
python -c "import torch; print(torch.__version__, torch.cuda.is_available())"

# One-command local run: put .txt/.md/.jsonl files in a folder first.
# Start small; resume is supported if the time limit is reached.
python scripts\local_train.py --corpus data\local\books --preset local_1m `
  --tokens 20000000 --device cuda --out runs\local_1m

# Chat with the checkpoint locally; no API key, no network.
python -m genesis_model.cli chat --checkpoint runs\local_1m\genesis.pt --device cuda

# Inspect / evaluate / serve
python -m genesis_model.cli inspect --checkpoint runs\local_1m\genesis.pt
python -m genesis_model.cli eval --checkpoint runs\local_1m\genesis.pt
python -m genesis_model.cli serve --checkpoint runs\local_1m\genesis.pt --port 8420
```

The `tiny` preset exists to verify gradients, caching, checkpointing and the agent control flow. It is **not** a useful assistant. The `local_*` presets are the first real targets; `seed_30m` and `seed_120m` are optional later runs on larger hardware. See [`docs/LOCAL_FIRST.md`](docs/LOCAL_FIRST.md) for measured-time guidance and memory rules.

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
| `local_1m` | 1.2M | 256 | first real run; 20M tokens in 8–12 min measured |
| `local_3m` | 3.1M | 384 | 60M tokens in 30–40 min measured |
| `local_6m` | 6.3M | 512 | main 4 GB quality target; 120M in 2.5–4 h |
| `local_15m` | 15.5M | 512 | 300M tokens in 13–16 h measured |
| `local_30m` | 26.7M | 512 | 300M tokens in 25–32 h measured |
| `seed_30m` | 30M | 2k | optional larger-context run |
| `seed_120m` | 120M | 4k | rented GPU / larger machine |
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

1. **Now:** local-first pipeline; train `local_1m`–`local_6m` on a real local corpus; local chat; first honest model card.
2. **Next:** real image-text data and a small vision tower; tool-use SFT; measured held-out evaluation.
3. **Then:** `local_15m`/`local_30m` long runs; C++/CUDA inference kernels.
4. **Optional later:** `seed_30m`/`seed_120m` on rented or larger hardware; 1B+ scale; MoE/MLA experiments.
5. **Long term:** speech from scratch, GRPO for tool use, distributed training.

---

## Licence

To be decided by the project owner. Until a licence is published, the default is
all rights reserved. Third-party datasets and libraries retain their own
licences; see the dataset registry before release.
