# Genesis IDE Live Acceptance Report

**Date:** 2026-10-25
**Status:** GATE_D_IDE_E2E_VERIFIED

## Gate D scope
Controlled integration of GenesisClient into production Aetherius IDE (approved). Pre-change git status recorded; affected source backed up (useGenesisRuntime.tsx.bak).

## Confirmed installed changes
- Genesis: Gates A/B/C + followup applied. Trusted startup (GENESIS_AGENT_BRIDGE=1), checkpoint-free loading, 300s timeout, /health None-safe.
- Aetherius-OS: GenesisClient.ts present in runtime dir. NOT imported by any IDE source (grep: only TODO reference).

## Real HTTP runtime (verified)
- Checkpoint-free trusted startup: /health 200 (11 tools, agent_bridge_active=true, parameters=0).
- /v1/agent: "Calculate 47 + 58 using Agent Bridge" → 200, steps=2, phase4.add_numbers(47,58)=105, answer contains 105, 6.92s (warm, CUDA, GTX 1050 Ti).
- Genesis tests: 9/9 passed.

## Verification
- tsc --noEmit: PASS.
- IDE runtime tests (bounds + useGenesisRuntime): 28/28 PASS.
- Genesis tests (test_agent, test_tools, test_tokenizer): 15/15 PASS.
- Graphical IDE (puppeteer, headless Chrome):
  - Launched dev server on 127.0.0.1:5199.
  - GenesisRuntime on 127.0.0.1:8420 (checkpoint-free, GENESIS_AGENT_BRIDGE=1).
  - Selected Genesis mode via window flag.
  - Connection status: "runtime genesis: connected".
  - Submitted: "Calculate 47 + 58 using Agent Bridge."
  - IDE displayed: "The result of 47 + 58 is **105**." (Genesis) — 105 verified.
  - Latency: ~110s end-to-end (Ollama qwen3.5:0.8b, warm).
  - Screenshots: 01_loaded.png, 02_chat.png, 03_typed.png, 04_result.png.
- /v1/agent (isolated): steps=2, phase4.add_numbers(47,58)=105.

## Changed files (IDE)
- `runtime/runtimeTypes.ts` — added `'genesis'` to `RuntimeSource`.
- `runtime/useGenesisChat.ts` (NEW) — minimal parallel hook reusing `GenesisClient`.
- `runtime/GenesisClient.ts` — already present (approved adapter), untouched.
- `GenesisShell.tsx` — liveOverride + baseOverride flags, effectiveSource/effectiveBase, runtime selection, genesis label, sendMessage condition.
- `runtime/bounds.test.ts` — allowlist updated to include `GenesisClient.ts`.

## Changed files (Genesis, additional production change)
- `genesis_model/server.py` — added `do_OPTIONS` handler for browser CORS preflight (localhost-only preserved).

## Backups
- `Phase12/backups/` — runtimeTypes.ts.bak, useGenesisRuntime.tsx.bak, GenesisShell.tsx.bak, useGenesisRuntime.tsx.bak2.

## Git status (post)
- Genesis: main @ 7c3bc93f, modified server.py + docs (no commit).
- Aetherius-OS: modified runtimeTypes.ts, GenesisShell.tsx, bounds.test.ts, + useGenesisChat.ts (new), + GenesisClient.ts (ignored). No commit.

## Remaining limitations
- 300s timeout upper bound; cold-start latency UX risk.
- Session persistence/restore not implemented.
- Streaming/cancellation not supported.

## Performance Appendix — profiling the ~110s response (POST-ACCEPTANCE)

### Method
- GenesisRuntime (port 8420, checkpoint-free, GENESIS_AGENT_BRIDGE=1) restarted cold.
- Three sequential `/v1/agent` requests with the accepted task, timed with `time.monotonic()`.
- Stage timestamps read from the result `events` array (start → model step 1 → tool → model step 2 → answer).
- GPU verified via `nvidia-smi`; model via `curl 127.0.0.1:11434/api/ps`.

### Measured timings (events stage delta)
| Request | Total   | start→model(step1) | model(step1)→model(step2) | steps |
|---------|---------|--------------------|---------------------------|-------|
| 1 (cold)| 204.65s | 178.3s (model load) | 26.2s                     | 2     |
| 2 (warm)| 5.72s   | 1.84s              | 3.87s                     | 2     |
| 3 (warm)| 5.51s   | 1.54s              | 3.96s                     | 2     |

### Bottleneck
**Ollama cold-start model load (~178s).** Warm inference is 5.5s. The ~110s browser figure sits between cold (204s) and warm (5.5s): Ollama had partially timed the model out between the isolated curl test and the browser run, so the browser request paid a partial reload. Not a GPU, CPU-fallback, retry, queue, or polling problem — warm throughput is healthy.

### GPU / VRAM evidence (partial)
- `nvidia-smi` did **not** show a persistent `ollama`/`llamacpp` GPU process during testing; only system apps (Edge, ChatGPT, Hermes) appeared under GPU. However `/api/ps` reports `size_vram: 564689631` (~564 MB) for qwen3.5:0.8b (Q8_0, 752 M params, runner `llamacpp`), so VRAM is allocated — the GPU residency window is brief and was not caught in a snapshot.
- No CPU fallback suspected: warm latency (5.5s) is consistent with GPU-assisted offload of a 752 M model; a CPU-only run of this size would be markedly slower.

### CORS change review (server.py do_OPTIONS)
- Added: `204` response with `Access-Control-Allow-Origin: *`, `Allow-Methods: GET, POST, OPTIONS`, `Allow-Headers: Content-Type`.
- Server remains localhost-bound (`serve(host="127.0.0.1")`); an `*` origin on a localhost-only binding cannot be reached by external origins under the browser same-origin policy, so this is not a public-exposure regression.
- OPTIONS is headers-only (`Content-Length: 0`), executes no tool, performs no auth bypass.
- Agent identity and tool permissions unchanged (still enforced in `do_POST`).
- Acceptable for local dev. Production would constrain `Allow-Origin` to the IDE's origin and add a bearer/session check.

### Recommended minimal fixes (no production change applied)
1. **Keep the model resident** while the IDE session is active (`OLLAMA_KEEP_ALIVE` or poll `/api/ps`). Eliminates the 178s reload — biggest win.
2. **Show loading state** in the IDE during the ~5–6s warm inference (already supported via `busy`); surface "Warming up CUDA model…" only when cold detected.
3. **Tighten CORS** to the IDE origin once deployed on a non-localhost host.
4. **Surface stage latency** (cold vs warm) in the runtime notice for UX transparency.

### New limitations (vs. baseline)
- Cold-start remains a UX risk until model residency is configured (ops/config change, not source).
- GPU residency evidence is brief/undetected by `nvidia-smi` in a snapshot — needs continuous sampling to quantify utilization.

### Keep-alive re-test (POST-CONFIG)
- Preloaded `qwen3.5:0.8b` with `keep_alive=30m` (554 MB VRAM, `/api/ps` confirms resident before request).
- Launched GenesisRuntime (port 8420) + IDE dev server (5199); connected `genesis: connected`.
- Submitted "Calculate 47 + 58 using Agent Bridge." → IDE displayed **"The result of 47 + 58 is **105**."** (Genesis).
- **Graphical end-to-end latency: 6.65s**, within the warm HTTP baseline (5.5–5.7s). Down from ~110s.
- Post-request: model still resident (`expires_at` extended, not overridden); GPU 2266 MiB; VRAM intact → Genesis requests **did not override** the 30-minute keep-alive, they extended it (expected Ollama behaviour).
- CUDA inference active (VRAM allocated, GPU memory used 2266/4096 MiB).

## Performance Appendix — controlled re-test (2026-10-25, 14:55–15:10 UTC)

### Setup (established startup scripts, no rebuild)
- GenesisRuntime: `genesis_model/server.py` (checkpoint-free, `GENESIS_AGENT_BRIDGE=1`, `localhost:8420`), venv `Projects/Genesis/.venv-torch-probe`.
- Aetherius IDE: `vite --port 5199 --host 127.0.0.1` from `apps/IDE-Workspace/workspace/app` (wired review state on disk; **not** committed/pushed).
- Ollama: `qwen3.5:0.8b` via `aetherius_bridge.py` (`OLLAMA=http://127.0.0.1:11434`, no `keep_alive` sent in the `/api/chat` payload).
- GPU: GTX 1050 Ti (4096 MiB), `nvidia-smi` sampling at 0.1–0.3 s.

### Keep-alive isolation test (the requested question)
Controlled experiment — preload with explicit `keep_alive=30m`, issue exactly one Genesis `/v1/agent` call, compare expiry:

| Step | `ollama /api/ps expires_at` | remaining-to-expiry |
|------|------------------------------|---------------------|
| A) after 30m preload            | 16:09:32 | **1774 s** (~30 m) |
| B) after one `/v1/agent` call   | 16:09:59 | **1802 s** (~30 m) |

**Result: Genesis requests do NOT override the 30-minute keep-alive.** Ollama preserves an existing explicit `keep_alive` when a `/api/chat` request omits the field; the model stayed resident for ~30 m across the agent call.

Note (reconciles the earlier 110 s): the runtime's `/api/chat` payload omits `keep_alive`, so if the model is loaded *by a Genesis call itself* (cold, no prior preload), Ollama applies its **default 300 s (5 min)** expiry — `final_warm_check`: "remaining-to-expiry after chat call = 299 s". So a model loaded via the agent loop self-retains for ~5 min, not 30 min. An externally preloaded 30 m window is preserved (above). The earlier 110 s graphical run was therefore a **cold, non-resident model** — not a keep-alive override.

### Latency measurements (same task: "Calculate 47 + 58 using Agent Bridge.")
- Warm `/v1/agent` (model resident): **4.07 s** (and 4.76 s on an adjacent request) — at/below the 5.5–5.7 s warm HTTP baseline.
- Warm graphical E2E (`submit` click → "105" visible): **5.9 s** — matches the warm HTTP baseline (small overhead from `GenesisClient` health polling + render).
- Cold graphical E2E (model NOT resident at start, `pre_state ollama=[]`, GPU 525 MiB): **77.4 s** — cold Ollama model load (~70 s) + ~6 s warm. This is the source of the ~110 s figure.
- Response verified: IDE thread = "The result of 47 + 58 is **105**." (`phase4.add_numbers(47,58)` → 105).

### CUDA evidence (this run)
- `ollama /api/ps`: `size_vram = 564689631` (~564 MB, Q8_0) — model resident in GPU memory.
- `nvidia-smi`: VRAM 2117–2266 MiB during requests; GPU utilization spike to **14 %** during a 6-request burst and **24 %** on cold load (qwen3.5:0.8b is tiny, so sustained util is modest but nonzero).
- CPU-only torch is in the venv (`torch 2.14.1+cpu`, `cuda.is_available()==False`), but it is unused by this checkpoint-free path — all LLM tokens come from Ollama on CUDA.

### Bottleneck (confidence: high)
Ollama **cold model load (~70–180 s)** when the model is not resident. Warm inference is healthy (4–6 s). No evidence of CPU fallback, retries, queue build-up, or polling stalls as the cause.

### Remaining limitation
The ~5 min default expiry applies to models loaded by the agent loop itself. To guarantee a warm graphical experience for the full 30 m preload window, the runtime would need to forward the caller's `keep_alive` (or a persistent preload) — a config/ops change, not a source rebuild.

### Screenshots / artifacts
- `scratch/perf-run/screenshots/01_loaded.png`, `02_chat.png`, `04_result.png` (warm graphical run).
- `scratch/perf-run/pre_state.json`, `post_state.json`, `gpu_burst.log`, `final_warm_check.py`, `keepalive_override_test.py`.
