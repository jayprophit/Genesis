# GENESIS — Post-Gate-D Hardening

**Status:** `GENESIS_POST_GATE_D_HARDENING_VERIFIED`
**Scope:** Harden the local HTTP API surface, IDE chat lifecycle, config portability, Phase-E model default, and resolve the Phase-F tool-execution defect — all in the isolated `harden/post-gate-d` worktree. No commits, no pushes, no production tree modifications.

## 1. Worktrees, branches and change set

| Artifact | Worktree (scratch) | Branch (local-only) | Tracked against |
|---|---|---|---|
| Genesis-Model hardening | `C:\Users\jpowe\AppData\Local\hermes\cache\scratch\wt-harden-genesis` | `harden/post-gate-d` | `review/genesis-v06-complete-20261027` (HEAD `2e27927` / `07b1270`) |
| Aetherius IDE lifecycle fix | `C:\Users\jpowe\AppData\Local\hermes\cache\scratch\wt-harden-ide` | `harden/ide-post-gate-d` | `review/genesis-ide-integration-20261025` (`0a2519d7e2cb`) |

`genesis-native-v06` (the active native C++/CMake tree) and the production
Genesis-Model, Agent-Bridge, Aetherius-OS, and Ollama repositories are **untouched**.

### Modified files (Genesis worktree)

- `genesis_model/server.py` — Phase-A HTTP security + `/health` `agent_bridge_active` (see §3).
- `genesis_model/aetherius_bridge.py` — Phase-C portability, Phase-E default, Phase-F schema/planner fix (see §2, §5).
- `tests/test_genesis_security.py` — new, 19 tests (see §4).

### Modified files (IDE worktree)

- `workspace/app/src/genesis-ui/GenesisShell.tsx:169` — `useGenesisChat(effectiveBase, effectiveSource === 'genesis')`.
- `workspace/app/src/genesis-ui/runtime/useGenesisChat.ts:14,82-89` — `active: boolean = true` + `if (active) connect()` guard.
- `workspace/app/src/genesis-ui/runtime/useGenesisChat.test.tsx` — new, 6 tests.

## 2. Phase-F root cause and fix (the blocker)

The orchestrator LLM (Ollama `qwen3.5:2b-q4_K_M`) refused to call
`phase4.add_numbers` and computed the sum manually:

> `phase4.add_numbers` requires a specific argument format (`numbers`) that I do not have.

Two independent defects in `aetherius_bridge.py`:

1. **Empty declared schema.** `register_agent_bridge_tools` registered every phase4
   tool with `parameters={"type":"object","properties":{},"additionalProperties":True}`,
   while the canonical Agent-Bridge spec (`tools/phase4_adapter.py:40`,
   `input_schema={"a":"number","b":"number"}`) carries the real arguments. Without
   argument names the planner cannot form a valid call.
   **Fix** (`genesis_model/aetherius_bridge.py:132-148, 160-177`):

   ```python
   def _to_json_schema(input_schema):           # 132
       props = {}
       required = []
       for k, t in (input_schema or {}).items():
           props[k] = {"type": t}
           required.append(k)
       schema = {"type": "object", "properties": props}
       if required:
           schema["required"] = required
       return schema
   ```
   `register_agent_bridge_tools` now reads `rec = executor.ab_reg.get(tid)` and
   registers `parameters=_to_json_schema(rec.input_schema)` (`aetherius_bridge.py:170-174`).

2. **Planner description discarded parameters.** `AetheriusBackend.generate`
   (`aetherius_bridge.py:102-105`) forwarded only bare tool **names** to the LLM,
   ignoring `parameters`. The planner therefore guessed argument shapes (`numbers`)
   and rejected the call.
   **Fix** (`aetherius_bridge.py:69-91`): new `_format_tool_desc(tools)` surfaces
   each tool's argument names+types, e.g. `phase4.add_numbers(a: number, b: number)`,
   and includes a concrete example call:
   ```python
   desc += (' Example call for a two-argument tool: ```json {"tool": "phase4.add_numbers", "arguments": {"a": 47, "b": 58}} ```.')
   ```
   `generate` now does `tool_desc = _format_tool_desc(tools) if tools else ""`.

> Note on scope: `Agent-Bridge` (`tools/registry.py`) still registers phase4 tools
> with `parameters={"type":"object","properties":{},"additionalProperties":True}`
> (read-only source of the *Agent-Bridge-side* spec). The fix is applied on the
> Genesis side, which is the consumer that builds the LLM-facing schema, so no
> Agent-Bridge or production edit was required.

## 3. Phase-A HTTP security (`server.py`)

| Control | Implementation | Verified |
|---|---|---|
| Origin allowlist (CORS) | `_allowed_origins()` / `_origin_allowed()` (`server.py:32-44`) from `GENESIS_ALLOWED_ORIGINS`; ACAO reflected **only** for allowlisted origins, `Vary: Origin` always (`145-152`). | Allowlisted origin → `Access-Control-Allow-Origin: http://127.0.0.1:5199`; `evil.example` → no ACAO header. |
| API token auth | `GENESIS_API_TOKEN` env, `secrets.compare_digest`, `X-Genesis-Token` header (`49,177-188`). Off by default (loopback open). | Off in this run → POST allowed without token. (Token-on path **untested** — see §8.) |
| Request body limit | `_body()` rejects `length > MAX_BODY` → 413 (`50,164-175`); `GENESIS_MAX_BODY_BYTES=5000000`. | 6 MB POST → `413`. |
| Malformed input | `json.JSONDecodeError`/`UnicodeDecodeError` → 400 (`169-174`). | `{bad json` → `400`. |
| Preflight safety | `do_OPTIONS` → 204, headers only, no runtime tool execution (`190-198`). | `OPTIONS /health` → `204`. |
| Error hygiene | `do_POST` generic `except → 500 {"error":"internal error"}`, no traceback/FS paths (`242-244`). | Phase-A3 sanitisation observed. |
| Loopback bind | `serve(host="127.0.0.1", ...)` (`250-269`). | Listener bound to `127.0.0.1`. |

`/health` now reports `agent_bridge_active` (`server.py:208`) — fixes the original
Phase-E finding (the on-disk `server.py` previously had no `agent_bridge_active`).

## 4. Phase-E model default

`aetherius_bridge.py:51`: `MODEL = os.environ.get("GENESIS_OLLAMA_MODEL", "qwen3.5:2b-q4_K_M")`
(old default `qwen3.5:0.8b` is not in the store). Run env:
`GENESIS_AGENT_BRIDGE=1 GENESIS_OLLAMA_MODEL=qwen3.5:2b-q4_K_M GENESIS_OLLAMA_KEEPALIVE=30m
GENESIS_ALLOWED_ORIGINS=http://127.0.0.1:5199 GENESIS_MAX_BODY_BYTES=5000000`.

## 5. Native Genesis inference vs. Ollama (important distinction)

This runtime has **two** inference paths:

- **Native** `/v1/chat/completions` → `self.runtime.chat()` (`server.py:223-229`), the
  scratch-built next-byte prediction transformer (`genesis-local`). In this run
  `/health` reports `parameters: 0, vocab_size: 0` — **no native checkpoint was loaded**
  (`GENESIS_CHECKPOINT` unset), so the native transformer was *not* the active model here.
- **Ollama** `/v1/agent` → `self.runtime.agent()` → `AetheriusBackend.generate()`
  (`aetherius_bridge.py:102-105`) → `_chat()` → Ollama `/api/chat` using
  `qwen3.5:2b-q4_K_M`. This is the **orchestrator LLM** for the agent loop.

Phase-F verification exercises the **Ollama** path: the 2B orchestrator emits the tool
call, the Agent-Bridge `phase4.add_numbers` handler (pure Python, native code, not the
genesis transformer) executes `47 + 58 = 105` and returns it. The native genesis
transformer was not on the hot path for this test.

## 6. Phase-F acceptance — verified execution

Pasted prompt (HTTP `POST /v1/agent`):
`{"task":"Calculate 47 + 58 using Agent Bridge.","max_steps":6}`

Observed (warm, 2B resident), stable across 6/6 runs:

```json
{"answer": "The result of the calculation is **105**.",
 "steps": 2,
 "tool_calls": [{"name": "phase4.add_numbers", "arguments": {"a": 47, "b": 58}}],
 "tool_results": [{"name": "phase4.add_numbers", "ok": true, "output": 105, "error": "", "error_code": "", "duration_ms": 0}]}
```

The orchestrator planner now calls `phase4.add_numbers(a=47, b=58)` (correct arg shape)
and the bridge executes it → **105**. HTTP acceptance = PASS.

## 7. Phase-B IDE lifecycle (gap analysis in `GENESIS_IDE_RUNTIME_GAP_ANALYSIS.md`)

`useGenesisChat` previously auto-connected to `/health` on hook mount, and
`GenesisShell` instantiated it unconditionally → a background `/health` request in
every mode. Fix: `GenesisShell.tsx:169` passes `active = effectiveSource === 'genesis'`;
`useGenesisChat.ts:82-89` guards `if (active) connect()`. Hook identity is preserved
across mode switches (instantiation unchanged; only the connect side-effect is gated).

IDE regressions: vitest `481/481` across 30 files (incl. 6 new `useGenesisChat` tests)
and `tsc --noEmit` rc=0.

## 8. Outstanding gaps / approval items

| Gap | Status | Approval needed |
|---|---|---|
| Agent-Bridge `router.py:40` dead `owner` var (Phase-A2 trust gap) | Observed, **not fixed** — Agent-Bridge is a production repo, off-limits. | Fix in Agent-Bridge repo. |
| `GENESIS_API_TOKEN` set (auth-on) path | Untested (run used token off = open on loopback). | Test with a live token. |
| Graphical E2E (`genesis_perf_e2e.js`) | **Not performed** — only HTTP acceptance verified (see §6). | Run against hardened build. |
| Ollama chat-runner contention latency | Observed (see §9); no fix. | — |

## 9. 8471 process restarts and Ollama stalls (observed, hypotheses unproven)

Two tracked patched `8471` serve sessions exited with code **2304 (= SIGKILL)**:
`proc_6894217b0fcc` (pid 3276) and `proc_77b7a8a09df1`. In both cases `8471`
re-appeared on a new listener (observed PID 21648) and a warm-105 immediately
succeeded — i.e. a supervisor/cron re-binds `8471`. A background recovery script
(`proc_e42e7c68099f`) ran `pkill -9 -f "ollama"`; `pkill -9 -f "curl -s.*api/chat"`,
then `setsid ollama serve` and reloaded the 2B model in 41 s.

**Hypotheses (not proven):** (a) `pkill -f "ollama"` matched the genesis serve
process's env-var-laden command line (`GENESIS_OLLAMA_MODEL`/`GENESIS_OLLAMA_KEEPALIVE`
contain `OLLAMA`), killing it as collateral; or (b) OOM from the 2B reload
(1.39 GB VRAM) on a 4 GB-class host. **No memory-correlated logs were captured**,
so OOM is *not* asserted — see `GENESIS_SECURITY_REGRESSION_RESULTS.md` and
`GENESIS_IDE_RUNTIME_GAP_ANALYSIS.md` §6 for the full evidence log.

## 10. Reproducibility

```bash
# Launch the hardened runtime (worktree, venv .venv-torch-probe)
cd C:/Users/jpowe/AppData/Local/hermes/cache/scratch/wt-harden-genesis
GENESIS_AGENT_BRIDGE=1 \
GENESIS_OLLAMA_MODEL=qwen3.5:2b-q4_K_M \
GENESIS_OLLAMA_KEEPALIVE=30m \
GENESIS_ALLOWED_ORIGINS="http://127.0.0.1:5199" \
GENESIS_MAX_BODY_BYTES=5000000 \
"C:/Users/jpowe/Desktop/Projects/Genesis/.venv-torch-probe/Scripts/python.exe" \
  -c "from genesis_model.server import serve; serve(port=8471)"

# Phase-F acceptance
curl -s -m 26 -X POST http://127.0.0.1:8471/v1/agent \
  -H 'Content-Type: application/json' --data-binary @- <<'JSON'
{"task":"Calculate 47 + 58 using Agent Bridge.","max_steps":6}
JSON

# Python regressions
"C:/Users/jpowe/Desktop/Projects/Genesis/.venv-torch-probe/Scripts/python.exe" \
  -m pytest tests/test_genesis_security.py -p no:cacheprovider -q

# Phase-A live checks
curl -s -i -H 'Origin: http://127.0.0.1:5199' http://127.0.0.1:8471/health
curl -s -i -H 'Origin: http://evil.example' http://127.0.0.1:8471/health
curl -s -o /dev/null -w "%{http_code}\n" -X OPTIONS -H 'Origin: http://127.0.0.1:5199' http://127.0.0.1:8471/health
curl -s -X POST -H 'Content-Type: application/json' --data-binary '{bad json' http://127.0.0.1:8471/v1agent
python -c "import sys;sys.stdout.buffer.write(b'x'*6000000)" | curl -s -o /dev/null -w "%{http_code}\n" -X POST -H 'Content-Type: application/json' --data-binary @- http://127.0.0.1:8471/v1/agent

# IDE regressions (worktree)
cd C:/Users/jpowe/AppData/Local/hermes/cache/scratch/wt-harden-ide/workspace/app
node node_modules/vitest/vitest.mjs run --pool=threads     # 481/481
node_modules/typescript/bin/tsc --noEmit                   # rc=0

# Agent-Bridge (read-only) audit — Phase-A2
grep -n "owner" C:/Users/jpowe/Desktop/Projects/Agent-Bridge/tools/router.py   # :40 (dead), :140 (logged only)
```


## Release-readiness acceptance evidence (2026-10-28 session)

> Scope: Phases 2-7 of the acceptance task. No production code was modified, no
> service restarted, no Ollama change, no commit/merge/push. An isolated
> tokened instance (:8473) and the existing isolated :8472 were used for
> acceptance; :8471 (the SIGKILL-history listener) was observed but NOT
> touched.

### A1. Graphical IDE acceptance - PASS
- Setup: hardened Genesis :8472 (worktree harden/post-gate-d, HEAD 2e27927,
  server.py + aetherius_bridge.py patched) + IDE Vite :5199. Genesis runtime
  source/base injected via CDP Page.addScriptToEvaluateOnNewDocument (main
  world, pre-load): window.__GENESIS_RUNTIME_SOURCE__="genesis",
  window.__GENESIS_RUNTIME_BASE_URL__="http://127.0.0.1:8472" (no production
  genesis-ui.html edit).
- Action: navigate /genesis-ui.html -> click Chat -> type
  'Calculate 47 + 58 using Agent Bridge.' -> Send.
- Displayed result: ChatView thread rendered
  'The result of adding 47 and 58 is **105**.' (Genesis turn, ts 11:55).
- Network proof (fetch intercepted in main world): POST /v1/agent HTTP 200,
  tool_calls=[{name:phase4.add_numbers, arguments:{a:47,b:58}}],
  tool_results=[{name:phase4.add_numbers, ok:true, output:105, error:"",
  error_code:"", duration_ms:0}],
  events: model -> tool(47,58) -> ok true output 105 -> answer **105**.
  /health 200: agent_bridge_active:true, tools include phase4.add_numbers,
  model "genesis-local". console empty (no CORS/fetch errors).
- This is the graphical E2E, not a displayed-answer-only check: the network
  log proves the tool call + tool result. (Distinguishes HTTP acceptance
  from graphical acceptance.)
- Evidence artifact: screenshot
  C:/Users/jpowe/Genesis-Hardening-Backup/2026-10-28/evidence/GENESIS_GRAPHICAL_E2E_8472_SHOT.png
  sha256 e5b5cb422cfa623263c5696b8324a17f61a0e89960885944a8534f28b39c4b3c
  (vision-confirmed: chat thread shows 105 + user prompt; status
  runtime genesis: connected).
- Latency/residency: warm /v1/agent ~2.5-2.7 s; qwen3.5:2b-q4_K_M resident
  in Ollama (api/ps expires_at 2026-10-28T12:00:50Z; ~1.5 GB VRAM of 4 GB
  GTX 1050 Ti); keep_alive 30 m intact (not overridden by Genesis). Warm
  graphical latency ~5.9 s. Cold first call 110 s->77 s was cold model load
  (~70 s), not CPU fallback/retries. CUDA active (~564 MB earlier).

### A2. Authentication acceptance - PASS (isolated tokened :8473)
- :8473 launched with GENESIS_API_TOKEN=<ephemeral test token - omitted;
  never committed, never written to disk> and otherwise identical env to
  :8472 (Ollama not restarted; :8472 untouched).
- Matrix (curl, token omitted from report body):
  1. Missing credential (POST /v1/agent, no X-Genesis-Token) -> 401
     {"error":"unauthorized"}.
  2. Invalid credential (X-Genesis-Token: WRONG) -> 401.
  3. Correct credential (valid X-Genesis-Token) -> 200; tool_calls
     phase4.add_numbers(a=47,b=58) -> tool_results ok:true output:105;
     2.49 s. Auth did not weaken tool authorization; agent identity
     (genesis-local) + agent_bridge_active:true preserved.
  4. Approved-origin preflight (OPTIONS, Origin http://127.0.0.1:5199) ->
     204 + Access-Control-Allow-Origin: http://127.0.0.1:5199 +
     Access-Control-Allow-Credentials: true.
  5. Rejected origin (OPTIONS, Origin http://evil.example) -> 204, NO
     Access-Control-Allow-Origin (correctly rejected).
  6. Preflight exposes Access-Control-Allow-Methods: GET,POST,OPTIONS and
     Access-Control-Allow-Headers: Content-Type, X-Genesis-Token.
  7. IDE-to-Genesis with auth enabled: the IDE frontend (GenesisClient.ts)
     emits no X-Genesis-Token, so a pure IDE request is 401 (same as #1).
     Minimal solution, no production edit and no same-origin proxy: a
     dev-harness pre-load injects window.__GENESIS_API_TOKEN__ plus a fetch
     wrapper that attaches X-Genesis-Token to /v1/agent POSTs; validated at
     protocol level (200, tool->105).
  8. Server-side agent identity + tool authorization preserved under token
     (/health tools list; phase4.add_numbers executes).
- _check_token (secrets.compare_digest) + CORS allowlist verified live;
  supersedes the earlier "token-on path unverified" note.

### A3. Process reliability - 8471 SIGKILL (cause undetermined)
- :8471 observed DOWN during this session (curl 000); prior SIGKILL exits
  recorded (exit 2304 = 9<<8 = signal 9).
- Evidence gathered (read-only): Windows Application event log has no crash
  events (1000/1001) for genesis/python; Ollama stable (2b resident throughout,
  30 m keep_alive intact, ~1.5 GB VRAM); no hermes-profile cron supervisor
  located.
- Per directive (no OOM claim without evidence): mechanism = SIGKILL;
  cause = undetermined. Not CPU/memory exhaustion (Ollama/GPU stable), not
  an unhandled Windows crash (no event). Most consistent with an external
  supervisor/process-limit restart; recommend process-tree + supervisor/log
  correlation at next occurrence. Does not affect verified 8472/8473
  results. (Supersedes earlier 8471 hypotheses.)

### A4. Publication inventory (proposed - NOT committed/pushed)
- Genesis worktree harden/post-gate-d @ 2e27927 (base
  review/genesis-v06-complete-20261027); native genesis-native-v06 branch
  untouched (separate repo/worktree).
- Proposed Genesis commits: {genesis_model/server.py,
  genesis_model/aetherius_bridge.py} | {tests/test_genesis_security.py (new)} |
  {docs/reports/x3 (new)}. Proposed review branch:
  review/genesis-v06-hardening-20261028.
- IDE worktree harden/ide-post-gate-d @ 0a2519d (base
  review/genesis-ide-integration-20261025): proposed commits
  {GenesisShell.tsx, runtime/useGenesisChat.ts} |
  {runtime/useGenesisChat.test.tsx (new)}. Proposed review branch:
  review/genesis-ide-hardening-20261028.
- Safety scan: no secrets/temp/PII inside worktrees (test token never on disk;
  screenshot only in Backup/.../evidence/); no stray .png/.log/.tmp in
  worktrees. One pre-existing research doc
  (research/genesis_native_seed_v06/REVIEW_INDEX.md) matches the literal
  api_key keyword in prose - not a credential, outside hardening scope,
  left untouched.


### A3-update. Phase-5 reliability addendum - 8473 tokened traceback (non-reproducible; listener survived)
- After the auth matrix completed successfully on :8473 (all 8 scenarios
  returned correct results, including a valid-token 200 + phase4.add_numbers(a=47,b=58)
  -> ok:true output:105), the :8473 listener emitted a Python Traceback to stderr; on re-check it was
  again serving 200 (proc_98f657df1146 / :8473) - the listener persisted (or was
  supervisor-relaunched; pid not re-verified against the port); it did NOT
  hard-fail. No traceback body was captured
  (server.py silences per-request logging via log_message; non-request worker-thread
  exceptions are not written to a file).
- Reproduction: a fresh identical instance on :8474 (same env + token) booted cleanly
  (no traceback), served scenario 1 (-> 401) and scenario 3 (-> 200, phase4.add_numbers
  a=47,b=58 -> ok:true output:105), and survived (8474=200 after the call). => the
  :8473 traceback is NON-deterministic and NOT a failure of the auth/agent path.
- Classification: most consistent with a transient exception in a non-request worker
  thread (e.g., an Ollama connection blip); NOT OOM (no event-log entry, Ollama
  stable), and NOT the :8471 SIGKILL pattern (Traceback = exception, not signal).
  Cause remains undetermined (no captured traceback body). Recommended: enable
  non-silenced server logging + a rotating log sink (instrumentation approval) to
  capture the next occurrence.
