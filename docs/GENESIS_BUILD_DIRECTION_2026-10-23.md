# Genesis — independent model direction

**Date:** 23 October 2026  
**New repository:** `C:\Users\jpowe\Desktop\Projects\Genesis-Model`  
**Full analysis:** `Genesis-Model/docs/ECOSYSTEM_ANALYSIS_2026-10-23.md`  
**Global landscape:** `Genesis-Model/docs/GLOBAL_MODEL_LANDSCAPE.md`

---

## What the estate actually contains

| Repository | Role | Reusable | Gap |
|---|---|---|---|
| `Aetherius-OS/kernel/Genesis` (C++ v0.31) | organism, identity, memory, cognition, development, authority, tool execution, runtime, security | Yes — keep as authoritative organism | `registry/models.tsv` = **MODEL-NONE**; no neural model |
| `Agent-Bridge` | external tools, providers, sessions, policy, multimodal adapters | Yes — keep as optional capability plane | does not train or own a model |
| `IDE-Workspace` | UI, Genesis shell, avatar seam | Yes — renderer/presentation | live binding deferred |
| `Aetherius-OS` | host/supervisor, apps, predecessor absorption | Yes — host services | not a model runtime |
| `MAT`, `Universal-Bridge`, `Poietek` | domain apps | Yes — consumers | not on the model critical path |
| `genesis_native_seed_v0_1` | ~145k-param from-random prototype | Yes — mechanism proof | toy scale; not a product model |

The estate has the organism and the connectors. It does not have the **model**.

---

## What was built (new, independent repository)

`Genesis-Model` is a fresh repository. It does not modify Genesis, Agent-Bridge, Aetherius-OS or the IDE.

Implemented and tested:

- byte-level BPE tokenizer with 16 control tokens and exact multilingual round-trip;
- decoder with RMSNorm, RoPE, grouped-query attention, SwiGLU, tied embeddings;
- KV cache with full-forward equivalence;
- from-scratch vision tower fused into the same sequence as text;
- native tool protocol (`<|tool_call|>…<|/tool_call|>`), JSON-schema validation, approval gates, workspace confinement;
- native agent loop (plan → tool → observe → answer) with bounded steps and audit events;
- training loop for text, image caption and tool-use SFT, with AMP, resume and manifest;
- safe checkpoint format (`weights_only=True`);
- local HTTP runtime (`/health`, `/v1/models`, `/v1/tools`, `/v1/chat/completions`, `/v1/agent`);
- evaluation for bits-per-byte, tool-format accuracy and decode throughput.

**Local evidence:** `24 passed` in the test suite; end-to-end smoke test `SMOKE OK` (vocab 302, 131,840 parameters).  
These prove correctness of the pipeline, **not** language or vision quality. No trained product checkpoint exists yet.

---

## The identity rule

Genesis is its own model:

- random initialisation, no pretrained weights, no borrowed tokenizer;
- own data manifest with licence, hash and consent basis;
- own training, checkpoints and evaluation;
- other models may be reached through Agent-Bridge **as tools**, clearly labelled, never as Genesis;
- no fine-tune of Qwen/Llama/Claude/Gemini/DeepSeek/Kimi/Mistral renamed as Genesis;
- no distillation from a closed API to create the base model.

---

## Global landscape — headline

- **US:** product discipline, tool UX, and the reproducibility template (OLMo/Molmo); small-model data quality (Phi).
- **China:** the richest source of mechanisms worth adopting — MLA, fine-grained MoE + shared experts, multi-token prediction, GRPO, FP8, hybrid attention, long context, multilingual breadth, aggressive open weights.
- **Europe:** efficiency (Mistral), documentation and regulation-ready governance (Aleph Alpha, EU AI Act).
- **Japan/Korea/India/Middle East/SEA:** language coverage, tokenizer fertility, regional benchmarks and data sovereignty — Japanese, Korean, Indic, Arabic, SEA, Traditional Chinese.
- **Closed products:** benchmark their behaviour; never pretend to have their weights.

**Adoption plan:** dense + GQA + RoPE + SwiGLU + early vision + tool tokens in v1; MLA, MoE, MTP, GRPO and hybrid attention evaluated at v2; 7B scale only at v3.

---

## Realistic scale

| Version | Params | Tokens | Hardware | Purpose |
|---|---|---|---|---|
| `tiny` | 0.5M | 1M | CPU | correctness/CI |
| `local_1m` | 1.2M | 20M | GTX 1050 Ti 4 GB | first local run; 8–12 min measured |
| `local_3m` | 3.1M | 60M | GTX 1050 Ti 4 GB | 30–40 min measured |
| `local_6m` | 6.3M | 120M | GTX 1050 Ti 4 GB | main local target; 2.5–4 h measured |
| `local_15m` | 15.5M | 300M | GTX 1050 Ti 4 GB | 13–16 h measured |
| `local_30m` | 26.7M | 300M | GTX 1050 Ti 4 GB | 25–32 h measured |
| `seed_30m` | 30M | 2B | 1× 8–24 GB | optional later run |
| `seed_120m` | 120M | 20B | 1× 24 GB / 4× A100 | larger machine |
| Genesis-2 | 1.5B / 0.4B active | 300B | 8–16× H100 | brand-quality assistant |
| Genesis-3 | 7B / 1.5B active | 1–2T | 64+× H100 | competitive open model |

The first model is trained and used **locally**, with no API bill. The GTX 1050 Ti
has now been measured at 38k–41k tok/s for `local_1m`, 34k–35k for `local_3m`,
9k–14k for `local_6m`, 6.2k–6.6k for `local_15m`, and 3.1k–3.4k for
`local_30m` (FP32, short runs). That makes a real 300M-token `local_30m` run a
roughly one-day local job. A 1B model on 20B tokens is still roughly 83
H100-hours (≈ $170–400 at $2–5/hour) before retries and evaluation. A 1.5B model
on 300B tokens is roughly 1,875 H100-hours (≈ $4k–9k). Those are planning
estimates for later scale, not quotes. The 1050 Ti cannot train a 1B model.

---

## Next five actions

1. Curate 10–100 MB of local, licence-cleared text with a manifest.
2. Train `local_1m`/`local_3m`/`local_6m` on the real local corpus; chat with it locally; publish the first model card with real held-out measurements.
3. Wire the native agent loop to the C++ `ActionProposal` / `Authority` / `ToolExecutor` seam.
4. Register Agent-Bridge as an optional tool provider, not the default.
5. Replace `registry/models.tsv` `MODEL-NONE` only after a checkpoint passes the component checklist definition of done.

The next milestone is not another architecture document. It is a trained checkpoint with measured results.
