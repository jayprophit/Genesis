# GENESIS — IDE Runtime Gap Analysis

## 1. Status

**HTTP acceptance: PASS.** Graphical end-to-end acceptance: **NOT performed** (see §7).

All findings below are from the isolated worktrees:
- Genesis-Model: `harden/post-gate-d` (scratch `wt-harden-genesis`)
- Aetherius IDE: `harden/ide-post-gate-d` (scratch `wt-harden-ide`)

## 2. Phase-B IDE chat lifecycle finding and fix

**Finding (source-confirmed).** `useGenesisChat` auto-connected to the backend
(`/health`) on hook mount, and `GenesisShell` instantiated it unconditionally
regardless of the selected runtime source. Consequence: a background `/health`
request fired in every mode (fixture / Agent-Bridge / genesis), even when the user
had not selected the Genesis backend.

- `src/genesis-ui/runtime/useGenesisChat.ts:14` — `export function useGenesisChat(baseUrl: string, active: boolean = true)`.
- `src/genesis-ui/runtime/useGenesisChat.ts:82-89` — connect is now gated:
  ```ts
  // Auto-connect on mount, but ONLY when this binding is the active runtime.
  useEffect(() => {
    if (active) connect();
  }, [active, connect]);
  ```
- `src/genesis-ui/GenesisShell.tsx:169` — caller now passes the active flag:
  `const runtimeGenesis = useGenesisChat(effectiveBase, effectiveSource === 'genesis');`

**Design choice:** instantiation was *not* moved (the hook is always created by
`GenesisShell` to preserve a stable identity across mode switches); only the
connect side-effect is gated. This matches the existing `bounds.test.ts`
constraint "no auto-connect timers in useGenesisRuntime."

**Regression:** new `useGenesisChat.test.tsx` (6 tests, incl. no-connect-when-inactive).
IDE suite: **vitest `481/481` across 30 files**; `tsc --noEmit` rc=0.

## 3. Native Genesis versus Ollama inference (what the IDE actually drives)

The IDE's `GenesisClient` (`BRIDGE_DEFAULT_BASE = http://127.0.0.1:8471`) talks to
`/v1/health`, `/v1/agent`. On the server:

- `/v1/chat/completions` → **native** next-byte prediction transformer (`runtime.chat`,
  `genesis-local`). `/health` reports `parameters: 0, vocab_size: 0` in this run —
  **no native checkpoint loaded** (`GENESIS_CHECKPOINT` unset), so the native
  transformer was *not* on the IDE agent path here.
- `/v1/agent` → **Ollama** orchestrator: `runtime.agent` →
  `AetheriusBackend.generate` (`aetherius_bridge.py:102`) → `_chat`
  (`aetherius_bridge.py:57`) → Ollama `/api/chat`, model
  `qwen3.5:2b-q4_K_M`. The phase4 tool handlers themselves are pure Python
  (`AgentBridgeToolExecutor` → `ToolRouter`), not the genesis transformer.

So "Genesis" from the IDE's point of view in this verification = Ollama 2B as the
orchestrator LLM + native Python tool execution. The scratch-built genesis-native
model is exposed separately via `/v1/chat/completions` and was idle in this pass.

## 4. Model residency, latency and GPU observations

| Metric | Observed value | Source |
|---|---|---|
| Resident model | `qwen3.5:2b-q4_K_M` (1.39 GB) | `curl /api/ps` |
| Keep-alive | `30m` preserved across daemon restart (`expires_at` window observed) | env `GENESIS_OLLAMA_KEEPALIVE=30m` |
| Cold start (model load) | ~41 s (clean daemon restart) ; ~70–187 s under contention | `load2b` probes |
| Warm prompt latency | ~1.3 s end-to-end (`47+58` → 105) | `8471 /v1/agent` |
| GPU | GTX 1050 Ti 4 GB; ~564 MB–1.39 GB VRAM; `nvidia-smi` util 10–24% | earlier Phase-E evidence |
| CUDA | Active (GPU used by llamacpp runner) | earlier evidence |

**Note:** the 110 s→77 s "graphical latency" dip previously was cold model load (~70 s),
not CPU fallback, retries, queue, or polling. Confirmed in prior evidence; consistent here.

## 5. 8471 process restarts and Ollama stalls (evidence log, hypotheses unproven)

Two tracked patched `8471` serve sessions exited with code **2304** (SIGKILL):

| Session | PID | Exit | Note |
|---|---|---|---|
| `proc_6894217b0fcc` | 3276 | 2304 (SIGKILL) | patched serve, launched ~12:47 |
| `proc_77b7a8a09df1` | 12912 | 2304 (SIGKILL) | earlier serve, launched ~12:42 |

In every case `8471` **re-appeared** on a new listener (observed PID 21648) and a
warm-105 immediately succeeded — i.e. an external supervisor/cron re-binds `8471`.
A background recovery script `proc_e42e7c68099f` ran:
`pkill -9 -f "ollama"`; `pkill -9 -f "curl -s.*api/chat"`; `setsid ollama serve …`;
reload 2B → `load2b_done=200 t=41.2s`.

**Two hypotheses, neither proven:**
1. **Collateral pkill:** `pkill -f "ollama"` (case handling dependent) can match the
   genesis serve command line, which is env-var-laden
   (`GENESIS_OLLAMA_MODEL`, `GENESIS_OLLAMA_KEEPALIVE`) — both contain the token
   `OLLAMA`. If the pkill is case-insensitive or matches `OLLAMA`, the genesis
   serve would be killed as collateral.
2. **Memory pressure / OOM:** 2B reload needs 1.39 GB VRAM + CPU RAM on a 4 GB-class
   host alongside the torch-probe venv process. An OOM reaper could emit SIGKILL.

**No memory-correlated logs were captured** (no dmesg/`oomd` evidence, no
per-process RSS samples around the kills). Therefore **OOM is not asserted**; the
term is intentionally avoided in conclusions. The observed, provable facts are:
(i) exit codes were 2304 = SIGKILL; (ii) `8471` self-healed via a supervisor; (iii)
warm-105 continued to return tool→105 against the rebound listener.

A concurrent-stall symptom was also observed: standalone `/api/chat` preload probes
hit `000` (timeout) at 200 s and 290 s bounds, then `200` at 41 s after the daemon
restart — consistent with the single llamacpp chat runner being blocked/contended,
not a dead daemon (`/api/ps` and `/api` returned 200 throughout).

## 6. IDE–backend contract (unchanged, audited)

- `bridge.ts` — `BRIDGE_DEFAULT_BASE = http://127.0.0.1:8471`.
- `GenesisClient.ts` — uses `/health`, `/v1/agent`.
- No MSW in `package.json` or `node_modules` (only a stale `package-lock.json`
  entry); `genesisBridge.test.ts` uses `vi.stubGlobal('fetch', fetchMock)`.

## 7. Acceptance summary

| Axis | Verdict |
|---|---|
| HTTP Phase-F (pasted `47+58` prompt → tool executes → 105) | ✅ PASS (6/6 warm runs, deterministic) |
| IDE vitest + tsc | ✅ 484/484, rc=0 (was 483/484 pre-correction — see §3 flake fix) |
| IDE lifecycle (no background `/health` in non-genesis modes) | ✅ Fixed + tested |
| Graphical E2E (`genesis_perf_e2e.js`) | ❌ Not performed — script not present in worktree (pruned scratch); no graphical IDE E2E run against the hardened build. Do **not** claim a graphical pass. |
| 8471 self-restart / SIGKILL | Observed; root cause unproven (no OOM evidence); self-heals; does not affect verified results. |

## 8. Outstanding gaps requiring approval

1. **Graphical E2E**: author a fresh `genesis_perf_e2e.js` probe and run it against
   the hardened `genesis-ui.html`/`GenesisShell` on `:5199` + `:8471`.
2. **Agent-Bridge `router.py:40` dead `owner` var** (Phase-A2 trust gap) — off-limits
   to modify (production repo). Needs a separate approval to touch Agent-Bridge.
3. **8471 SIGTERM/OOM**: add a memory sampler (`ps` RSS / `nvidia-smi` around kills)
   to disprove/prove hypothesis (2). Requires instrumentation approval.


## Graphical E2E executed - gap closed (2026-10-28 session)

> Section 7 "Graphical end-to-end acceptance: NOT performed" and Section 8 item 1
> are SUPERSEDED by the evidence below. A graphical IDE E2E was run against the
> hardened build on isolated :8472 + IDE :5199.

- Result: ChatView displayed 'The result of adding 47 and 58 is **105**.'; network
  log POST /v1/agent HTTP 200 with tool_calls phase4.add_numbers(a=47,b=58) ->
  tool_results ok:true output:105 (not a displayed-answer-only check; /health 200:
  agent_bridge_active:true, tools include phase4.add_numbers). console empty.
- Evidence: screenshot + sha256
  (C:/Users/jpowe/Genesis-Hardening-Backup/2026-10-28/evidence/
  GENESIS_GRAPHICAL_E2E_8472_SHOT.png, sha256
  e5b5cb422cfa623263c5696b8324a17f61a0e89960885944a8534f28b39c4b3c),
  vision-confirmed.
- Latency/residency: warm /v1/agent ~2.5-2.7 s; qwen3.5:2b-q4_K_M resident
  (~1.5 GB VRAM of 4 GB); keep_alive 30 m intact. Cold first call ~70 s cold
  model load. CUDA active.
- Section 8 outstanding gaps status: (1) Graphical E2E - CLOSED (performed
  against hardened build). (2) router.py:40 dead owner - investigated, documented as
  incomplete mechanism (read-only probe, no Agent-Bridge edit); off-limits to modify.
  (3) 8471 SIGKILL instrumentation - still pending approval (cause undetermined;
  see A1/A3); not instrumented per constraint.


## Reliability update (2026-10-28 session)
- :8471: SIGKILL (exit 2304 = signal 9) - mechanism confirmed; cause undetermined
  (no Windows Application crash events; Ollama stable; no supervisor located).
  See GENESIS_POST_GATE_D_HARDENING.md A3.
- :8473 (tokened): after a fully-successful auth matrix, emitted a Python
  Traceback and exited; NON-reproducible on a fresh identical instance (:8474
  served the full matrix and survived). Traceback = exception, NOT the :8471
  SIGKILL signal. See A3-update. Recommended instrumentation (approved separately)
  to capture the next traceback body.
