# Genesis-Model gap analysis against first-prompt requirements
# Created after progressive incorporation of master prompt / data files
# Every claim must have an evidence reference; nothing is claimed without one.

## Requirements from first prompt (conversation checkpoint / user directive)

### 1. Build Genesis as independent first-party brand
- Status: COMPLETE
- Evidence: `README.md` (independence policy), `docs/GENESIS_STATUS.yaml`, repo `C:\Users\jpowe\Desktop\Projects\Genesis-Model` (local git, `main` branch, commits `d0491dd` → `6527518` → `f814fe6`). No borrowed weights; tokenizer and model initialized randomly.

### 2. Fresh build — random initialization only; no borrowed weights/tokenizer/embeddings/encoders
- Status: COMPLETE
- Evidence: `genesis_model/tokenizer.py` (byte-level BPE from scratch), `genesis_model/model.py` (`nn.Module` with no `from_pretrained` calls), `tests/test_tokenizer.py` (multilingual exact round-trip). Registry `models.tsv` remains `MODEL-NONE`; not replaced.

### 3. Must be its own brand comparable to ChatGPT, Claude, Manus, Copilot, Kimi
- Status: PARTIAL
- Evidence: Brand name `Genesis` used throughout (`README.md`, `CHANGELOG.md`, `docs/COMPUTE_PLAN.md`, `docs/GENESIS_STATUS.yaml`). No marketing claim of parity; the document states explicitly that a 1.2M-parameter model is not comparable to hosted assistants (honest ceiling). Model card (`MODEL_CARD_TEMPLATE.md`) not yet filled.

### 4. Native tool calls and native vision/text learning; Agent-Bridge optional, not dependency
- Status: COMPLETE (core implemented); PARTIAL (C++ wiring external)
- Evidence: `genesis_model/tools.py` (native `<|tool_call|>` protocol, schema validation, approval gates), `genesis_model/model.py` (vision tower `VisionConfig`, `patch_embed`, `vision_layers`). `tests/test_agent.py` covers bounded loop. Agent-Bridge wiring (`IModelRuntime` seam) remains external (`docs/COMPONENT_CHECKLIST.md` `C23` SPEC, `C24` SPEC); not modified in other repos.

### 5. Preserve existing organism code, repo boundaries, registry IDs, contracts; nothing existing modified
- Status: COMPLETE
- Evidence: `Aetherius-OS`, `Agent-Bridge`, `Universal-Bridge/Poetek`, `IDE-Workspace`, `MAT` repos untouched (`git status` in those repos unchanged). Registry IDs (`models.tsv` `MODEL-NONE`) preserved. No source modifications in external repos.

### 6. PyTorch acceptable only as tensor/autograd library; eventual native C++20 runtime target
- Status: PARTIAL
- Evidence: Current code uses `torch` (tensors, autograd, `nn.Module`, `F.cross_entropy`). `docs/COMPUTE_PLAN.md` and `README.md` note C++20 runtime (`C19` SPEC in checklist) is future work; no C++ source added to `Genesis-Model` yet.

### 7. Maintain truthful implemented/tested/verified status; no fabricated benchmarks
- Status: COMPLETE
- Evidence: `docs/EVIDENCE.md` records exact commands (`python scripts/local_train.py ...`), exact results (`token_count=4124787`, `vocab=4096`, `parameters=1741952`, `steps=1220`, `text_ce=3.5639`), exact generation sample (`prompt="The quick brown fox"`, output shown). `docs/GENESIS_STATUS.yaml` separates `completed` / `partial` / `incomplete` with evidence references. No benchmark numbers without source.

### 8. Local-first: train and run first Genesis model entirely on user’s machine before any cloud model
- Status: COMPLETE (first real checkpoint achieved)
- Evidence: Checkpoint `runs/local_1m_gutenberg/genesis.pt` exists (14.7 MB, 1,741,952 params, 1220 steps, 9,994,240 tokens, trained on `GTX 1050 Ti` with `torch==2.14.1+cu126`, FP32). `docs/LOCAL_FIRST.md` documents measured throughput (38k–41k tok/s `local_1m`, 34k `local_3m`, 6.6k `local_15m`, 3.4k `local_30m`). No cloud API used.

### 9. Four separable layers: Genesis Model, Genesis Agent, Genesis Organism, Agent-Bridge
- Status: PARTIAL
- Evidence: `Genesis Model` fully implemented (`model.py`, `train.py`, `tokenizer.py`, `data.py`). `Genesis Agent` implemented (`agent.py`, bounded loop, audit events) but C++ wiring external. `Genesis Organism` (`C++` source) exists in `Aetherius-OS` (~53 TU, 23 test TU) but `IModelRuntime` seam not wired (`COMPONENT_CHECKLIST.md` `C09` `C23` `C24` SPEC). `Agent-Bridge` preserved untouched.

### 10. New independent repository at `Genesis-Model`; existing repos untouched
- Status: COMPLETE
- Evidence: `C:\Users\jpowe\Desktop\Projects\Genesis-Model` is a separate git repo (`main` branch, commits from `e095e67` to `f814fe6`). No modifications in `Aetherius-OS`, `Agent-Bridge`, etc.

### 11. Architecture v1: byte-level BPE, dense decoder with RMSNorm/RoPE/GQA/SwiGLU/tied embeddings, joint vision tower, native `<|tool_call|>` protocol, bounded agent loop, local HTTP runtime
- Status: COMPLETE (core architecture)
- Evidence: `genesis_model/config.py`, `model.py`, `tokenizer.py`, `data.py`, `tools.py`, `agent.py`, `server.py`, `cli.py`. All components implemented and tested (`tests/test_tokenizer.py`, `tests/test_model.py`, `tests/test_agent.py`, `tests/test_training.py`). HTTP runtime (`serve`) available but basic.

### 12. No Qwen/Llama/Claude/Gemini/DeepSeek/Kimi/Mistral weights, tokenizers, embeddings, pretrained encoders
- Status: COMPLETE
- Evidence: `genesis_model/tokenizer.py` trains BPE from `tokenizer.train()`; `GenesisModel.__init__` initializes weights randomly (`nn.Linear`, `nn.Embedding` without `from_pretrained`). `tests/test_tokenizer.py` verifies exact round-trip without borrowed vocabulary. `tests/test_model.py` verifies vision gradients reach `patch_embed` (not frozen encoder).

### 13. Local presets (`local_1m` … `local_30m`) sized for 4 GB VRAM; text-first (`vision=None` for local presets)
- Status: COMPLETE
- Evidence: `genesis_model/config.py` (`PRESETS`), `scripts/local_train.py` (`DEFAULT_BATCH` updated). All five presets measured: `local_1m` (1.2M params, 782 MB peak), `local_3m` (1.7M, 1.02 GB), `local_6m` (4.3M, 1.4 GB), `local_15m` (12.7M, 1.67 GB), `local_30m` (22.9M, 1.52 GB). All run successfully on `GTX 1050 Ti` (`docs/LOCAL_FIRST.md`).

### 14. No cloud model used; no per-token API costs; first model exists locally before any scale path
- Status: COMPLETE
- Evidence: Checkpoint exists locally (`genesis.pt`). `docs/COMPUTE_PLAN.md` notes `local_1m`–`local_30m` measured; `seed_30m`/`seed_120m` remain future options. No API keys used (`docs/EVIDENCE.md`).

### 15. Fresh initialization (random init only)
- Status: COMPLETE
- Evidence: `GenesisModel` weights initialized by `nn.Module` defaults; no `load_state_dict` in initialization (`checkpoint.py` only for safe resume). `tests/test_training.py` verifies resume uses saved optimizer/model state, not pretrained weights.

### 16. Preserve working modules; nothing existing modified
- Status: COMPLETE
- Evidence: `docs/EVIDENCE.md` confirms no changes in external repos. `git status` in `Genesis-Model` shows only local commits.

### Data / file tracking from first prompt (conversation checkpoint / user instructions)
- Status: PARTIAL
- Evidence: `data/local/gutenberg/` (public-domain corpus, 62 files, manifest) — implemented. `master-prompt-subset/` (10 Genesis-related files, manifest) — implemented. Full master prompt folder (`115` files, `10.86 MB`) — indexed but not registered in `SOURCE-REGISTRY.yaml` as sources (`subset` excluded). Conversation transcripts / pasted chat text — not registered or explicitly excluded (`TODO-GEN-006` `PARTIAL`). `docs/GENESIS_STATUS.yaml` records all stages.

## What remains (incomplete / future)
- Full master-prompt registry entry (115 files) or explicit exclusion with reason.
- Conversation transcript / pasted chat text formalization or exclusion.
- Held-out evaluation (`bits-per-byte`, perplexity).
- Tool-use format validity measurement.
- Multi-hour stability / thermal test.
- Model card (`MODEL_CARD_TEMPLATE.md`) filled.
- `local_15m` / `local_30m` full-token-budget runs (300M tokens).
- `seed_30m` / `seed_120m` scale runs (optional later).
- Vision/text dataset at scale (current: synthetic only).
- Agent-Bridge / C++ organism wiring (`C23` `C24` `SPEC`).
- Speech, distributed training, native C++ runtime.
- Registry `MODEL-NONE` replacement (`C10` `COMPONENT_CHECKLIST.md`).
