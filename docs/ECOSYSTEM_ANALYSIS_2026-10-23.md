# Genesis — Independent Model and Agent: Estate Analysis and Build Direction

**Date:** 23 October 2026  
**Status:** Repository analysis completed; new model repository scaffolded and tested locally.  
**Scope:** `C:\Users\jpowe\Desktop\Projects` (Aetherius-OS, Agent-Bridge, MAT, Universal-Bridge, nested Genesis, IDE-Workspace), `C:\Users\jpowe\Downloads\genesis_native_seed_v0_1.zip`, and the supplied research corpus.

---

## 1. Executive finding

The estate already contains a serious **organism, agent-control and external-capability platform**. It does **not** yet contain a Genesis model.

That is not an opinion. The canonical Genesis repository registers exactly one model:

```text
registry/models.tsv
MODEL-NONE   No bundled model   -   -   -   -   no inference route   0   UNAVAILABLE
```

Everything labelled "AI" today falls into one of four categories:

1. **C++ organism and agent control** — identity, life record, memory, cognition, development, authority, tool execution, persistence, runtime, security. Real code, real tests, ~29 translation units and 17 benchmark programs.
2. **Agent-Bridge** — a large provider/tool/session/policy platform (600+ files, 2,221 passing tests reported). It routes to *other people's models* and *other people's tools*.
3. **A Python seed** (`genesis_native_seed`, ~145k parameters) — a genuine from-random prototype: byte tokenizer, 2-layer transformer, 16×16 synthetic shapes, save/reload, HTTP API. It proves the gradient path; it is not a language model, a product, or a brand.
4. **UI and host surfaces** — IDE-Workspace, Aetherius-OS, avatar events, renderer seams. The renderer binding is explicitly deferred and truthful about being a placeholder.

So the correct next move is **not** to rebuild the organism, the bridge, or the IDE. It is to build the missing middle: **Genesis-1, a first-party multimodal model with native tool calling and its own inference runtime**, then attach the existing organism and bridge as consumers.

This document records the audit, the global model landscape, the component specification, and the build order. The companion repository is at:

```text
C:\Users\jpowe\Desktop\Projects\Genesis-Model
```

It is a new repository. It does not modify Genesis, Agent-Bridge, Aetherius-OS or IDE-Workspace.

---

## 2. What Genesis is — four layers that must not be conflated

| Layer | Question it answers | Owner | Current state |
|---|---|---|---|
| **Genesis Model** | What does Genesis know and predict? | `Genesis-Model` (new) | Seed scaffold implemented and tested; no trained product checkpoint yet |
| **Genesis Agent** | What should Genesis do next, with which tool, under which policy? | Native loop in `Genesis-Model`; authority/execution in Genesis C++ | Native loop implemented; C++ authority and tool executor already exist |
| **Genesis Organism** | Who is Genesis across time — identity, memory, development, body? | `Genesis` C++ | Substantial; identity, life record, memory, cognition, maturity, genetics, organism loop |
| **Genesis Bridge** | How does Genesis reach devices, browsers, IDEs, clouds and third-party services? | `Agent-Bridge` (external), with a thin native adapter | Mature provider/tool plane; not the model |

The brand is **Genesis**. The model is **Genesis-1** (and later versions). The runtime is **Genesis Runtime**. The organism remains **Genesis**. Agent-Bridge is an *optional external capability plane*, not the definition of the agent.

This separation matters because it lets Genesis:

- run fully local with its own tools when no bridge is installed;
- use Agent-Bridge for browser, device, IDE, cloud and MCP/A2A work;
- change body/renderer without changing identity or weights;
- upgrade the model without rewriting the organism.

---

## 3. Verified estate inventory

### 3.1 Genesis (`Aetherius-OS/kernel/Genesis`, C++20, v0.31.0)

**What exists and is reusable**

- `identity/` — canonical Genesis identity, entity registry, lineage, life record, persistence.
- `memory/` — graph, origin, persistence.
- `cognition/` — workspace, world model, affect, capability, continuity.
- `learning/consolidation` — consolidation pipeline (behavioural, not neural training).
- `organism/` — anatomy, systems, loop, metabolism/homeostasis abstractions.
- `genetics/` — genome, expression, reproduction, persistence.
- `development/maturity` — developmental axes and independence threshold.
- `agents/` — action proposal, authority, admission, planner, tasks, hive, tool execution, platform.
- `perception/pipeline` — sensory intake abstraction.
- `runtime/` — host runtime.
- `security/` — key custody, crypto provider, platform inventory, open observation.
- `provenance/`, `ledger/`, `requirements/` — evidence and requirement registries.
- `benchmarks/` — 17 benchmark binaries including capability, continuity, learning, memory, perception, world model.
- `tests/` — 24 C++ test translation units.
- `registry/models.tsv` — **MODEL-NONE**. This is the gap.

**What does not exist**

- no neural network, tokenizer, optimizer, dataset loader, checkpoint or inference kernel;
- no image encoder or multimodal training;
- no speech model (only interfaces/contracts elsewhere);
- no trained Genesis model of any kind.

**Reuse decision:** keep the C++ organism as the identity/authority/organism plane. Add a narrow `IModelRuntime` seam that speaks the Genesis inference contract. Do not put training code into the C++ core.

### 3.2 Agent-Bridge (Python, large)

**What exists and is reusable**

- `models/inference_contract.py` — provider-independent request/response, tool call, usage, error, routing, privacy and budget schemas. This is a strong contract and should be adopted, not reinvented.
- `models/providers/` — Anthropic, Copilot, DeepSeek, Gemini, Kimi, llama.cpp, LM Studio, Ollama, OpenAI, OpenAI-compatible.
- `models/model_router.py`, `model_registry.py`, `model_lifecycle.py`, `provider_registry.py` — discovery, routing, health, leases, lifecycle.
- `tools/` — registry, router, selector, audit, toolkit; `tool_selector.py`; `actions.py`.
- `security/` — credential broker, KeePass bridge, vault adapters, supply-chain security.
- `execution_contract.py`, `executor.py`, `execution_environment.py`, `execution_observer.py` — execution and evidence.
- `policy.py`, `aether_policy_bridge.py`, `owner*.py` — authority and owner control.
- `memory.py`, `task_dag.py`, `taskcenter.py`, `orchestrator.py` — orchestration.
- `multimodal/` — STT, OCR, image providers, speech profile.
- `voice/`, `avatar/`, `nodes/`, `comms/`, `compute/` — modality and distributed execution.
- `benchmarks/` — benchmark schema, runner, calibration, history.
- `docs/contracts/GENESIS_AGENT_BRIDGE_v1.md` — integration contract.
- `docs/architecture/MULTIMODAL_ARCHITECTURE.md` — capability routing, never model-name routing.
- `docs/architecture/SMALL_MODEL_RUNTIME.md` — bounded shortlists, compact schemas, one action per turn.

**What it is not**

- it does not train a model;
- it does not own model weights;
- it does not provide a first-party tokenizer, transformer or vision tower;
- its "local models" are Ollama/llama.cpp artifacts from other families.

**Reuse decision:** use Agent-Bridge as the **external capability plane** and as the source of the wire contract. Genesis Runtime implements the same contract natively. The native tool registry is the default; Agent-Bridge tools are registered as a provider.

### 3.3 IDE-Workspace

- React/TypeScript Genesis shell, avatar stage, runtime binding docs.
- `RUNTIME_BINDING.md` explicitly documents that live avatar, lip sync, gaze, speech and neural rendering are deferred.
- Reuse the renderer seam and presentation events; do not make the IDE a model dependency.

### 3.4 Aetherius-OS

- Host/supervisor, predecessor absorption, native services, acceptance state, apps.
- Contains the nested Genesis repo and absorbed projects.
- Reuse process supervision, permissions, storage and observability. Do not duplicate the model runtime inside the OS.

### 3.5 MAT, Universal-Bridge, Poietek

Domain applications (materials atlas, DAW/device bridge, music production). They are consumers of Genesis capabilities, not part of the model. Keep them out of the model critical path.

### 3.6 `genesis_native_seed_v0_1`

**Verified by source inspection**

- `model.py`: byte-level tokenizer with control tokens, 2-layer/4-head transformer at width 64, convolutional patch embedding, image prefix, causal mask, tied random init.
- `train.py`: next-byte CE + synthetic image-caption CE, AdamW, gradient clipping, checkpoint.
- `evaluate.py`, `server.py`, `camera.py`, `bridge.py`, `speech.py`, `cli.py`, `data.py`.
- Tests: tokenizer round-trip, gradient reach, save/reload, HTTP health.
- Evidence manifest and `evaluation.json`.

**What it proves:** the team can train a differentiable model from random, route images into the same workspace, serve it, and test it.  
**What it does not prove:** language ability, real vision, speech, tool use, scale, or product quality. At ~145k parameters it is a mechanism demo.

---

## 4. The missing middle, stated precisely

```text
                    ┌──────────────────────────────┐
                    │  Genesis Organism (C++)      │
                    │  identity · memory · affect  │
                    │  development · authority     │
                    └──────────────┬───────────────┘
                                   │ intention / proposal
                    ┌──────────────▼───────────────┐
                    │  Genesis Agent (native)      │
                    │  plan · tool call · observe  │
                    │  verify · remember           │
                    └───────┬──────────────┬───────┘
                            │              │
              ┌─────────────▼───┐    ┌─────▼─────────────────┐
              │ Genesis Model   │    │ Tool plane            │
              │ weights · tok   │    │ native tools          │
              │ vision · heads  │    │ Agent-Bridge adapter  │
              └─────────────────┘    └───────────────────────┘
```

The estate has the top and right boxes. The bottom-left box is `MODEL-NONE`.

The new repository supplies:

- an independently trained tokenizer;
- a modern decoder (RMSNorm, RoPE, GQA, SwiGLU, KV cache);
- a from-scratch vision tower fused into the same sequence;
- native tool-call tokens and a tool registry;
- a native agent loop;
- a local HTTP runtime;
- training, checkpointing and evaluation.

---

## 5. What "our own model" must mean

This is the policy boundary, and it should be enforced in code review, not just prose.

### Required

1. **Random initialisation.** No `from_pretrained`, no loading Qwen/Llama/Claude/Gemini/DeepSeek/Kimi/Mistral weights, no importing a pretrained encoder or vocoder.
2. **Our tokenizer.** Trained by us on our corpus. No copied vocabulary file.
3. **Our training run.** Our data manifest, our optimizer, our checkpoints, our logs.
4. **Our inference path.** We can explain every tensor, kernel and cache.
5. **Our evaluation.** Held-out sets we constructed, with contamination checks.
6. **Provenance.** Every dataset, licence, URL and hash recorded.

### Allowed

- PyTorch/JAX as numerical libraries;
- published algorithms and papers;
- public datasets under their licences;
- studying open architectures and closed products as references;
- using Agent-Bridge to call other models **as tools**, clearly labelled, never as Genesis.

### Not allowed

- renaming or fine-tuning another model and calling it Genesis;
- distillation from a closed API to create the base model;
- copying a tokenizer, embedding table or weight file;
- claiming benchmark results that were not run on the named checkpoint;
- using private camera/mic data for training without separate consent.

The scaffold enforces the technical part: `GenesisModel` constructs random weights and `load_checkpoint` loads only our own format, with `weights_only=True`.

---

## 6. Global model landscape — what to learn, not copy

The point of this section is architectural intelligence. Every system below is a **reference**, not a weight source. Version numbers move monthly; families and mechanisms are what matter.

### 6.1 By region

| Region | Families worth studying | Mechanism worth learning | What Genesis should adopt |
|---|---|---|---|
| **United States** | OpenAI GPT (closed), Anthropic Claude (closed), Google Gemini (closed), Meta Llama (open weights), xAI Grok, Microsoft Phi, NVIDIA Nemotron, Ai2 OLMo/Molmo, Apple on-device | instruction tuning at scale, RLHF/Constitutional AI, multimodal early fusion, fully-open training recipes (OLMo), small-model distillation (Phi), on-device quantization | OLMo-style **full transparency**; Llama-style open serving ecosystem; Phi-style small-model discipline; Gemini/GPT-style tool schema quality |
| **China** | DeepSeek, Alibaba Qwen, Moonshot Kimi, Zhipu GLM, MiniMax, ByteDance Seed/Doubao, Tencent Hunyuan, Baidu ERNIE, 01.AI Yi, InternLM, OpenBMB MiniCPM, StepFun, Xiaomi MiMo, Huawei Pangu, Ant Ling, Meituan LongCat | **MLA** KV compression, fine-grained **MoE + shared experts**, **multi-token prediction**, **FP8** mixed-precision training, **GRPO** RL, hybrid/linear attention, long context, aggressive open-weight releases, strong multilingual and code data | MLA/GQA KV strategy; MoE as v2; MTP as an auxiliary objective; GRPO for tool-use RL; FP8 when hardware allows; multilingual data from the start |
| **Europe** | Mistral (FR), Aleph Alpha (DE), DeepMind (UK), Stability (UK), Hugging Face (FR/US), Liquid AI, ETH/EPFL labs | efficient sparse MoE, edge models, European-language data, open release strategy, safety/regulatory documentation | Mistral-style efficiency; EU AI Act documentation discipline; multilingual European corpora |
| **Japan** | Sakana AI, Preferred Networks PLaMo, Rakuten AI, SB Intuitions, NTT tsuzumi | Japanese tokenization/data, evolutionary model merging, compact enterprise models | Japanese corpus and evaluation; not model merging as a base strategy |
| **Korea** | Naver HyperCLOVA X, LG EXAONE, Upstage Solar, Kakao Kanana | Korean-centric data, bilingual evaluation, enterprise deployment | Korean data and Korean benchmark track |
| **India** | Sarvam, Krutrim, AI4Bharat (IndicBERT/IndicTrans), IIT labs | Indic scripts, code-mixing, low-resource language coverage | Indic tokenizer coverage and evaluation; code-mixed data |
| **Middle East** | TII Falcon (UAE), G42 Jais, SDAIA ALLaM (Saudi) | Arabic data quality, long-context, open weights | Arabic corpus and right-to-left handling |
| **South-East Asia** | SEA-LION (Singapore), Sea AI Lab, VinAI PhoGPT (Vietnam), Sahabat-AI (Indonesia) | multilingual SEA coverage, regional evaluation | SEA language track |
| **Other** | AI21 Jamba (Israel), YandexGPT/GigaChat (Russia), TAIDE (Taiwan), Masakhane/InkubaLM (Africa) | Mamba-Transformer hybrid, sovereign-language models, low-resource community corpora | hybrid attention experiment; community data licensing |

**Closed-source references** (architecture inferred from papers, docs and behaviour; never copied): GPT-5-class, Claude 4-class, Gemini 3-class, Grok, ERNIE, Doubao, Hunyuan, Pangu, Command A, Jamba, HyperCLOVA X, EXAONE, PLaMo. Their value is in **product behaviour**: tool schemas, refusal quality, latency, context management, multimodality, agent UX. Their weights and internals are not available and must not be pretended to be.

### 6.2 By mechanism

| Mechanism | Reference | Why it matters | Genesis v1 | Genesis v2 |
|---|---|---|---|---|
| Dense decoder + RoPE + GQA + SwiGLU + RMSNorm | Llama, Qwen, Mistral, OLMo | reliable, well-understood, fast to train | **yes** | yes |
| Byte-level BPE, multilingual | GPT-4/Qwen/Llama-style, but trained by us | no UNK, all scripts, tool tokens | **yes** | larger vocab |
| MLA (low-rank KV compression) | DeepSeek | much smaller KV cache at long context | no | **evaluate** |
| Fine-grained MoE + shared experts | DeepSeek, Qwen, MiniMax | more capacity at fixed active compute | no | **evaluate at 3B+** |
| Multi-token prediction | DeepSeek, Meta | denser training signal, speculative decoding | auxiliary head later | yes |
| GRPO / verifiable RL | DeepSeek, Kimi | tool use and reasoning improve from outcomes | SFT first | **yes** |
| FP8 mixed precision | DeepSeek, Qwen | cheaper large training | no (consumer GPU) | on H100/H200 |
| Hybrid linear/full attention | MiniMax, Jamba, Qwen | long context at lower cost | sliding-window experiment | yes |
| Early vision fusion | Qwen-VL, Molmo, Gemini | image tokens in the same workspace | **yes** (patch encoder) | stronger encoder + video |
| Native tool tokens | Kimi, Qwen, Claude, GPT | tool calls are model behaviour, not prompt glue | **yes** | constrained decoding + RL |
| Small-model distillation | Phi, MiniCPM, Qwen small | useful models on 4–16 GB devices | later | yes |
| Fully open data/model/eval | OLMo, Pythia, BLOOM | reproducibility and trust | **policy** | full release |

### 6.3 Honest scale reality

Frontier pretraining is a **$100M–$1B+** activity with tens of thousands of GPUs and petabyte-scale curated data. Genesis will not match GPT-5 or Claude on general knowledge by spending a few thousand dollars.

What is achievable, and worth doing:

- a genuine **0.1B–1B** model trained on **20B–300B** high-quality tokens;
- strong **tool use, instruction following, multilingual coverage and local deployment**;
- a **domain advantage** (engineering, manufacturing, Aetherius projects, personal memory) that frontier models do not have;
- full **provenance and local privacy**, which closed APIs cannot offer;
- a credible path to **3B–8B** with MoE and better data if compute is raised.

That is a real model brand. It is not a frontier lab, and the documentation must say so.

---

## 7. Genesis-1 reference architecture

```text
UTF-8 / images / tool events
        │
        ├── byte-level BPE tokenizer (ours, ~32k–64k)
        │        └── control tokens: system, user, assistant, end,
        │            image, tool_call, tool_result, think
        │
        ├── vision: patch conv → ViT blocks → linear projector
        │        └── image tokens prepended to the same sequence
        │
        ▼
┌──────────────────────────────────────────────────────────────┐
│ Genesis decoder                                              │
│  RMSNorm → GQA + RoPE → SwiGLU → residual                   │
│  × N layers, pre-norm, tied embeddings, KV cache            │
└──────────────────────────────────────────────────────────────┘
        │
        ├── LM head (next token)
        ├── tool-call parser (special tokens + JSON validation)
        └── future: MTP head, modality heads, value/uncertainty head
        │
        ▼
Genesis Agent loop → policy/authority → native tools / Agent-Bridge
        │
        ▼
Memory + provenance + organism state (C++ Genesis)
```

**Parameter targets**

| Version | Params (total / active) | Context | Tokens | Hardware | Purpose |
|---|---|---|---|---|---|
| G0 `tiny` | 0.5M | 256 | 1M | CPU | correctness, CI |
| G0.5 `seed_30m` | 30M | 2k | 2B | 1× 8 GB GPU | pipeline proof, tool format |
| G1 `seed_120m` | 120M | 4k | 20B | 1× 24 GB | first usable local model |
| G1.5 | 400M | 8k | 60B | 4× A100 40GB | multilingual + tools |
| G2 | 1.5B / 0.4B active MoE | 16k | 300B | 8–16× H100 | brand-quality assistant |
| G3 | 7B / 1.5B active | 32k+ | 1–2T | 64+× H100 | competitive open model |

**Compute estimate.** Training FLOPs ≈ 6 × N × D. At 40% MFU on H100 (~400 TFLOP/s effective), 1B × 20B tokens ≈ 1.2×10²⁰ FLOPs ≈ 83 H100-hours ≈ **$170–$400** at $2–5/hour. 1.5B × 300B tokens ≈ 2.7×10²¹ ≈ 1,875 H100-hours ≈ **$4k–$9k**. These are planning estimates, not quotes; data, retries and evaluation multiply them by 2–4×.

---

## 8. Component-by-component engineering specification

Each component has an owner boundary, an interface, an implementation, an acceptance test and a phase. "Reuse" names existing estate code; "new" means this repository.

### C01 — Corpus and provenance registry

- **Purpose:** every training token traceable to a source, licence, date, hash and consent basis.
- **Interface:** `DatasetRecord(id, uri, licence, sha256, tokens, languages, modality, contamination_notes, allowed_use)`.
- **Implementation:** JSONL manifest; `corpus_digest()` in `checkpoint.py`; downloader scripts record hashes; no dataset enters training without a record.
- **Acceptance:** `genesis-model inspect` prints corpus hash; a missing licence fails CI.
- **Reuse:** Genesis `provenance/`, `registry/datasets.tsv`; MAT evidence discipline.
- **Phase:** 0 (now).

### C02 — Tokenizer

- **Purpose:** convert all supported scripts and control events to IDs without UNK.
- **Interface:** `train(texts, vocab_size)`, `encode`, `decode`, `apply_chat_template`, `save/load`.
- **Implementation:** byte-level BPE; 256 bytes + 16 control tokens + merges; word-frequency merge training; deterministic merge order; exact round-trip.
- **Acceptance:** multilingual round-trip exact; control-token round-trip exact; save/load identical; vocabulary never reordered.
- **Reuse:** none (must be ours).
- **Phase:** 0 — **implemented and tested**.

### C03 — Data pipeline

- **Purpose:** packed token blocks, image-caption pairs, tool-use SFT, deterministic sharding.
- **Interface:** `PackedTextDataset`, `MultimodalCaptionDataset`, `ChatSFTDataset`, collators.
- **Implementation:** memory-mapped token arrays for large runs; prompt masking for SFT; image normalisation; dedup.
- **Acceptance:** no sample crosses a document boundary without an EOS; labels align with inputs; prompt tokens masked.
- **Reuse:** Agent-Bridge `knowledge_fabric/` for ingestion metadata.
- **Phase:** 0 — **implemented** for small corpora; sharding is next.

### C04 — Model backbone

- **Purpose:** the trainable sequence model.
- **Interface:** `GenesisModel.forward(tokens, images, targets, cache, position_offset)`.
- **Implementation:** RMSNorm, RoPE, GQA, SwiGLU, pre-norm, tied embeddings, dropout, KV cache.
- **Acceptance:** shape tests; loss finite; KV-cache logits match full forward; gradients reach every parameter; deterministic seed.
- **Reuse:** published algorithms only.
- **Phase:** 0 — **implemented and tested**.

### C05 — Vision tower

- **Purpose:** grounded image learning in the same workspace as text.
- **Interface:** `VisionEncoder(images) → [B, N, d]`, projected by `vision_proj`.
- **Implementation:** conv patch embed, learned position, ViT blocks, LayerNorm; trained jointly.
- **Acceptance:** gradients reach patch weights; caption loss falls on held-out synthetic seeds; real-image evaluation later.
- **Reuse:** none (must be ours).
- **Phase:** 0 — **implemented**; real-data training is Phase 2.

### C06 — Native tool protocol

- **Purpose:** tool calls are model output, not prompt glue.
- **Interface:** `<|tool_call|>{"name":...,"arguments":{...}}<|/tool_call|>`; `parse_tool_calls`; `ToolRegistry`.
- **Implementation:** reserved tokens; JSON schema validation; risk levels; approval gate; structured result.
- **Acceptance:** malformed calls rejected; schema violations rejected; path escape rejected; approval required for writes; every call audited.
- **Reuse:** Agent-Bridge `AetheriusToolCall` shape and `GENESIS_AGENT_BRIDGE_v1` contract.
- **Phase:** 0 — **implemented and tested**.

### C07 — Native tool registry

- **Purpose:** default capabilities with no bridge installed.
- **Tools v0:** calculator, list_dir, read_file, write_file (approval), search_files, now, http_get (disabled).
- **Acceptance:** no shell by default; no network by default; workspace confinement; size limits; timeout.
- **Reuse:** Genesis C++ `ToolExecutor` authority model; Agent-Bridge `tools/registry.py`.
- **Phase:** 0 — **implemented**.

### C08 — Agent loop

- **Purpose:** goal → model → proposal → policy → execution → observation → answer.
- **Interface:** `GenesisAgent.run(task) → AgentResult(answer, steps, calls, results, events)`.
- **Implementation:** bounded steps; tool results fed back; refusals are valid outcomes; transcript and audit events.
- **Acceptance:** scripted backend completes a tool task; max-step stop; tool error surfaced; no tool result invented.
- **Reuse:** Genesis `agents/planner`, `action_proposal`, `authority`, `tool_execution`.
- **Phase:** 0 — **implemented and tested**.

### C09 — Training engine

- **Purpose:** reproducible training on one GPU or many.
- **Interface:** `TrainConfig`, `train(...)`, `resume_from`.
- **Implementation:** AdamW, cosine schedule, warmup, grad accumulation, clipping, AMP/bf16, checkpointing, manifest.
- **Acceptance:** loss decreases on a fixed tiny run; resume produces identical next-step loss; manifest records tokens and hash.
- **Reuse:** PyTorch only.
- **Phase:** 0 — **implemented**; distributed launch is next.

### C10 — Checkpoint and model registry

- **Purpose:** safe, versioned artifacts.
- **Interface:** `save_checkpoint`, `load_checkpoint(weights_only=True)`, `inspect_checkpoint`.
- **Implementation:** config + tokenizer + weights + metadata; no pickle execution; SHA-256; version rollback.
- **Acceptance:** corrupt/mismatched checkpoint rejected; no arbitrary code execution; metadata complete.
- **Reuse:** Genesis `registry/models.tsv` becomes the canonical model registry; replace `MODEL-NONE` only with a verified artifact.
- **Phase:** 0 — **implemented**.

### C11 — Inference runtime

- **Purpose:** fast local generation with KV cache and streaming.
- **Interface:** `generate_ids`, `generate_text`, `serve`.
- **Implementation:** preallocated KV cache (next), fused kernels (later), C++/CUDA runtime (Phase 4), OpenAI-compatible local API.
- **Acceptance:** tokens/sec measured on target hardware; first-token latency; memory ceiling; deterministic greedy output.
- **Reuse:** Agent-Bridge inference contract; `llama.cpp`/`vLLM` as performance references, not code copies.
- **Phase:** 0 Python; Phase 3 native C++.

### C12 — Multimodal data and grounding

- **Purpose:** real image understanding, not synthetic shapes.
- **Interface:** `ImageSample(image, captions, provenance, consent)`.
- **Implementation:** licensed image-text corpora; OCR/document pairs; UI screenshots; camera frames only with separate training consent.
- **Acceptance:** held-out caption retrieval, VQA, OCR; no private frames; contamination report.
- **Reuse:** Agent-Bridge `multimodal/` for OCR/STT adapters.
- **Phase:** 2.

### C13 — Long context

- **Purpose:** 8k → 32k+ without retraining from scratch.
- **Interface:** `max_seq_len`, RoPE scaling config.
- **Implementation:** RoPE theta/scale, sliding-window attention, later MLA/hybrid.
- **Acceptance:** needle-in-a-haystack, long-document QA, KV memory measurement.
- **Reuse:** research only.
- **Phase:** 2.

### C14 — Alignment and safety

- **Purpose:** useful, honest, non-harmful behaviour with explicit uncertainty.
- **Interface:** SFT/DPO/GRPO datasets; refusal policy; safety classifier.
- **Implementation:** curated SFT, preference pairs, red-team set, tool-risk policy, approval gates.
- **Acceptance:** refusal rate on harmful set; over-refusal rate on benign set; jailbreak suite; tool misuse suite.
- **Reuse:** Agent-Bridge `policy.py`, `security/`.
- **Phase:** 1–2.

### C15 — Evaluation harness

- **Purpose:** no invented scores.
- **Interface:** `run_evaluation(checkpoint, corpus)`; per-task runners.
- **Implementation:** perplexity/bits-per-byte, tool-format accuracy, task success, latency, memory; dataset cards.
- **Acceptance:** every reported number has checkpoint hash, dataset hash, command and date.
- **Reuse:** Agent-Bridge `benchmarks/`, Genesis `benchmarks/`.
- **Phase:** 0 — **implemented baseline**.

### C16 — Memory and retrieval

- **Purpose:** continuity without confusing memory with weights.
- **Interface:** working/episodic/semantic/procedural stores; retrieve(query, scope, time).
- **Implementation:** SQLite/Postgres + vector index; provenance; decay; contradiction handling.
- **Acceptance:** retrieval precision; stale-fact handling; privacy scoping; no confabulated citation.
- **Reuse:** Genesis `memory/graph`, Agent-Bridge `memory.py`, Hindsight-style patterns.
- **Phase:** 2.

### C17 — Learning and consolidation

- **Purpose:** distinguish context, memory, behaviour and weights.
- **Interface:** `LearningEvent(kind, before, after, evidence, rollback)`.
- **Implementation:** SFT/LoRA jobs, replay buffer, retention tests, promotion gate.
- **Acceptance:** no catastrophic forgetting beyond threshold; rollback works; evaluator independence.
- **Reuse:** Genesis `learning/consolidation`.
- **Phase:** 3.

### C18 — Speech (TTS/STT)

- **Purpose:** own voice, not a borrowed checkpoint.
- **Interface:** `UtteranceStream(text, voice, emotion, timing)`.
- **Implementation:** text normaliser → phonemes → duration/prosody → acoustic model → vocoder; rights-cleared corpus; MOS/WER tests.
- **Acceptance:** intelligibility, latency, viseme alignment; licences.
- **Reuse:** Agent-Bridge `voice/`, `multimodal/speech_profile.py`; no pretrained vocoder.
- **Phase:** 3.

### C19 — Native inference engine (C++/CUDA)

- **Purpose:** independent runtime, no permanent PyTorch dependency.
- **Interface:** `NativeModelFormat`, `NativeLoader`, `NativeKVCache`, `Scheduler`, `InferenceStream`.
- **Implementation:** C++20 core, quantized kernels (INT8/INT4), CPU SIMD, CUDA/Vulkan/Metal backends, memory budget.
- **Acceptance:** matches PyTorch logits within tolerance; tokens/sec and memory on GTX 1050 Ti and target GPU.
- **Reuse:** Genesis C++ build system; study llama.cpp/vLLM.
- **Phase:** 4.

### C20 — Local API and SDK

- **Purpose:** stable integration for IDE, desktop, mobile, bridge.
- **Interface:** `/health`, `/v1/models`, `/v1/chat/completions`, `/v1/agent`, `/v1/tools`.
- **Implementation:** stdlib server now; production server later; Python/TS SDK.
- **Acceptance:** schema conformance; CORS; auth; rate limits; graceful shutdown.
- **Reuse:** Agent-Bridge `docs/api_schema.json`.
- **Phase:** 0 — **implemented** minimal server.

### C21 — Observability and audit

- **Purpose:** explain what happened.
- **Interface:** events for model load, prompt, token, tool proposal, approval, result, error, latency.
- **Implementation:** JSONL traces; trace IDs; redaction; retention.
- **Acceptance:** one trace per request; no secrets; replayable.
- **Reuse:** Agent-Bridge `compute/distributed_tracing.py`, `eventlog.py`.
- **Phase:** 1.

### C22 — Security

- **Purpose:** protect keys, files, devices and users.
- **Interface:** credential broker, workspace scopes, sandbox, network policy.
- **Implementation:** OS keychain, least privilege, signed updates, secret scanning, dependency pinning.
- **Acceptance:** no plaintext keys; escape tests; supply-chain scan; incident runbook.
- **Reuse:** Agent-Bridge `security/`, `supply_chain_security.py`; Genesis `security/`.
- **Phase:** 1.

### C23 — Organism integration

- **Purpose:** Genesis model serves Genesis organism.
- **Interface:** `IModelRuntime.generate(request) → response`; `AgentEvent` → organism loop.
- **Implementation:** C++ adapter calls the local runtime; identity and life record remain authoritative.
- **Acceptance:** one identity across restarts; model swap preserves memory; no duplicate organism per client.
- **Reuse:** Genesis `runtime/`, `identity/`, `organism/loop`.
- **Phase:** 1.

### C24 — Agent-Bridge adapter

- **Purpose:** reach external tools when needed.
- **Interface:** register bridge tools as `ToolSpec` providers; policy remains native.
- **Implementation:** map `AetheriusToolCall` ↔ native `ToolCall`; preserve approvals and evidence.
- **Acceptance:** bridge unavailable → native tools still work; bridge call audited; no policy bypass.
- **Reuse:** `docs/contracts/GENESIS_AGENT_BRIDGE_v1.md`.
- **Phase:** 1.

### C25 — Brand and distribution

- **Purpose:** Genesis as a product, not a folder.
- **Surfaces:** CLI, local runtime, desktop app, IDE panel, web chat, mobile later.
- **Assets:** name, mark, model card, system card, versioning (`Genesis-1`, `1.1`, `2`), release notes, download page.
- **Acceptance:** one identity; signed artifacts; model card per release; honest capability table.
- **Phase:** 1–2.

### C26 — Model card and system card

- **Purpose:** truthful disclosure.
- **Contents:** architecture, data, training compute, evaluations, limitations, intended use, prohibited use, licence, contact.
- **Acceptance:** published with every checkpoint; no unverified claim.
- **Phase:** every release.

### C27 — Evaluation independence

- **Purpose:** prevent self-grading.
- **Implementation:** withheld sets, external reviewer, contamination checks, reproducible commands.
- **Acceptance:** evaluator cannot be edited by the training job; hashes match.
- **Phase:** 2.

### C28 — Continuous improvement

- **Purpose:** improve without unsafe self-modification.
- **Implementation:** candidate → evaluation → approval → promotion → rollback.
- **Acceptance:** no self-authorised promotion; signed snapshots; regression suite.
- **Phase:** 3.

---

## 9. Training curriculum

| Stage | Data | Objective | Gate |
|---|---|---|---|
| S0 correctness | 1M tokens, synthetic shapes | LM + caption + tool format | loss falls, cache matches, tests green |
| S1 seed | 2B tokens, curated web/books/code/multilingual | next-token | bits-per-byte beats bigram baseline; sample quality review |
| S2 instruct | 100k–1M instruction pairs | SFT | instruction-following win rate vs base |
| S3 tools | 50k–500k tool traces | tool SFT | format validity > 98%; task success > baseline |
| S4 preference | 50k–200k pairs | DPO/GRPO | helpfulness/honesty/safety win rates |
| S5 multimodal | licensed image-text + docs + UI | caption/VQA/OCR | held-out retrieval and OCR |
| S6 long context | long docs/code | continued pretrain | needle test; memory ceiling |
| S7 specialisation | Aetherius/MAT/engineering corpora | continued pretrain | domain eval |

**Data sources to evaluate (licence first):** FineWeb-Edu, Dolma, CulturaX, Wikipedia/Wiktionary, OpenWebMath, proof-pile, The Stack v2 (filtered, licence-aware), OpenAssistant/UltraChat-style instruction sets with licence review, OpenImages/DataComp/COYO (image licences vary), OBELICS, DocVQA-style document sets, Common Voice (audio, later). Every source gets a `DatasetRecord`.

**Do not:** scrape private data, ignore robots/terms, train on user camera frames without separate consent, or mix a dataset with an unclear licence into a release.

---

## 10. Evaluation and evidence policy

Every claim must carry:

```text
checkpoint hash · dataset hash · command · hardware · date · evaluator version
```

Report at minimum:

- bits-per-byte on held-out text (not just loss);
- tool-format validity and tool-selection accuracy;
- task success on a fixed agent suite;
- multilingual accuracy per script;
- image caption retrieval / VQA / OCR;
- latency, first-token time, tokens/sec, peak memory;
- safety: harmful refusal, benign over-refusal, jailbreak, tool misuse.

The scaffold's `run_evaluation` already labels synthetic-shape results as non-evidence of real vision. Keep that honesty.

---

## 11. Build order (protecting the priority)

1. **Model correctness** — tokenizer, backbone, vision path, tool tokens, tests. *(done in scaffold)*
2. **Seed training** — 30M–120M on 2B–20B tokens; publish a model card with real numbers.
3. **Native agent** — tool registry, policy, approvals, traces. *(loop done; wire to C++ authority)*
4. **Organism integration** — one identity, memory, runtime seam.
5. **Real multimodal** — licensed images, OCR, documents.
6. **Instruct + tool SFT + preference optimisation.**
7. **Native inference engine** — C++/CUDA, quantization, streaming.
8. **Speech from scratch.**
9. **Embodiment** — renderer-neutral presentation; neural appearance later.
10. **Scale** — MoE, MLA, longer context, more data.

Parallel work that does **not** block the model: IDE binding, avatar presentation, Aetherius host services, MAT.

---

## 12. What would make this fail

- Training a 100M model and marketing it as frontier.
- Calling an Ollama/Qwen model "Genesis".
- Measuring loss on the training set and calling it capability.
- Building more UI before the model can hold a conversation or call a tool reliably.
- Letting the agent self-approve tool writes.
- Skipping data licensing and discovering it at release.
- Assuming the GTX 1050 Ti can train a 1B model; it cannot. Rent compute or stay small.
- Confusing the organism's biological simulation with validated physiology.
- Adding a second identity system instead of using the existing Genesis identity.

---

## 13. Immediate next actions

1. Run the scaffold tests and the tiny training smoke test (verified locally).
2. Replace `registry/models.tsv` `MODEL-NONE` only after a checkpoint passes evaluation.
3. Assemble the first 2B-token corpus manifest with licences and hashes.
4. Rent one 24 GB GPU; train `seed_30m` for 2B tokens; publish the first honest model card.
5. Wire `GenesisAgent` to the C++ `ActionProposal`/`Authority`/`ToolExecutor` seam.
6. Add the Genesis Runtime adapter to Agent-Bridge as one provider among others, clearly labelled.
7. Build the tool-use SFT set from real, consented traces of the estate's own workflows.

---

## 14. Bottom line

Genesis already has the hard, unusual parts: an organism with identity, memory, authority and evidence discipline, plus a broad external tool plane. What it lacks is the model itself.

The new `Genesis-Model` repository supplies that missing layer from random initialisation: tokenizer, decoder, vision tower, tool protocol, agent loop, runtime, training and evaluation. It is deliberately independent of every existing model and every existing repository. The next milestone is not more architecture — it is a trained checkpoint with measured results.
