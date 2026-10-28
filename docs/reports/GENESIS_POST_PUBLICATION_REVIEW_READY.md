# GENESIS — Post-Publication Code Review Corrections

**Deliverable:** `GENESIS_POST_PUBLICATION_REVIEW_READY`
**Scope:** Corrections to the published hardening review branches, developed in
**isolated worktrees only** — uncommitted, **not** committed/pushed/merged/reset
(per the review directive). No production repository was modified for Agent
Bridge; `Genesis-Model` (incl. its uncommitted `data/sft/...` changes) and the
live Ollama daemon (`:11434`) and `:8471` (down) were left untouched.

| Worktree | Branch | Base → Tip | State |
|---|---|---|---|
| `…/Genesis/postpub-genesis` | `postpub/genesis-corrections` | `2e27927 → fb78198` | 3 modified source files + 1 new test, uncommitted (linked worktree; `genesis_model tests docs` sparse) |
| `…/ide-corrections` | `review/genesis-ide-hardening-20261028` | `0a2519d → c4eade6` | 2 modified files, uncommitted |

Environment: venv `.venv-torch-probe` (Python 3.14.8, torch 2.14.1+cpu),
GPU GTX 1050 Ti 4 GB, `qwen3.5:2b-q4_K_M` resident on Ollama `:11434`
(not restarted; keep-alive 30 m preserved). `GENESIS_AGENT_BRIDGE_ROOT=
C:/Users/jpowe/Desktop/Projects/Agent-Bridge` (read-only import; repo not edited).

---

## 1. Evidence matrix

| P# | Defect | Severity | TDD | Live evidence | Status |
|---|---|---|---|---|---|
| P0 | Disallowed-origin POST executes agent on tokenless server (CORS ≠ server authz) | Critical | RED→GREEN | `curl :8472 -H "Origin: http://evil.example" → HTTP 200, tool→105` | ✅ Fixed + tested |
| P1 | `keep_alive` nested under `/api/chat` `options` (ignored by Ollama → cold start) | High | RED→GREEN | structural: serialized payload now top-level; `models:[]` after published-request cold load | ✅ Fixed + tested |
| P1 | Tools tested only with 47/58 | Low | GREEN | varied operands incl. negatives/decimals | ✅ Tests added |
| P1 | `owner_authorized` computed but never enforced | High (latent) | Investigated | `router.py:40` dead; negative test proves `owner_authorized:false` executes 105 | ⚠ Proposed (Agent-Bridge, needs approval) |
| P1 | IDE lifecycle: double-submit, stale messages, id collision, no abort, no reconnect-on-switch | Medium | RED→GREEN | vitest 9/9; `tsc --noEmit` clean | ✅ Fixed + tested |
| P2 | Hardcoded `C:/Users/...` path default (not portable) | Medium | RED→GREEN | — | ✅ Fixed + tested |
| P2 | Report contradictions (compare_digest, 8473, supervisor, HTTP-auth≠graphical) | Medium | Proposed edits | anchors located | ⚠ Proposed edits |

---

## 2. Prioritized corrections

### P0 — HTTP & IDE security: server-side Origin enforcement on POST

**Finding (source-confirmed, `server.py:217-238`).** The tokenless path is by design
(P0-A): `GENESIS_API_TOKEN=""` ⇒ `_check_token()` returns `True` (line 185-186). The
**only** side-effect gate is the shared secret. CORS, however, is enforced *client-side
only*: `_cors_headers`/`_origin_allowed` reflect `Access-Control-Allow-Origin` on
responses (server.py:40-44) but `do_POST` never consults the Origin. A browser page
served from any origin can issue a state-changing POST to the loopback server; the
response is unreadable cross-origin, but **the agent side-effect fires**.

**Live proof (published `:8472`, tokenless):**
```
$ curl -s -m 150 -X POST http://127.0.0.1:8472/v1/agent
    -H "Origin: http://evil.example"
    -H "Content-Type: application/json"
    -d '{"task":"Calculate 47 + 58 using Agent Bridge."}'
{"answer":"The result of adding 47 and 58 is **105**.",
 "tool_calls":[{"name":"phase4.add_numbers","arguments":{"a":47,"b":58}}],
 "tool_results":[{"name":"phase4.add_numbers","ok":true,"output":105}],
 ...}
[HTTP=200]
```
The agent executed `phase4.add_numbers(a=47,b=58)→105` despite `Origin: http://evil.example`.
This is a CSRF-style side-effect exposure against any user with the Genesis server
serving loopback. (Token gate holds when configured: `:8473` tokened + no `X-Genesis-Token`
→ `401`, verified live too.)

**Fix (minimal, reuses existing `_origin_allowed`).** Enforce the allowlist on POST in
`do_POST`, after `_check_token`. No-Origin requests (loopback scripts/curl) keep working;
the server is already bound to `127.0.0.1`.

```python
# genesis_model/server.py (do_POST) — proposed
        def do_POST(self) -> None:  # noqa: N802
            try:
                if not self._check_token():
                    self._send(401, {"error": "unauthorized"})
                    return
                # Server-side CORS enforcement on state-changing POSTs: reflecting
                # ACAO in the response does NOT stop a browser from SENDING a
                # disallowed-origin request with side effects ...
                origin = self.headers.get("Origin")
                if origin is not None and not _origin_allowed(origin):
                    self._send(403, {"error": "origin not allowed"})
                    return
                body = self._body()
```

**Tests** (`tests/test_postpub_corrections.py`):
- `test_disallowed_origin_post_is_rejected_tokenless` — RED on published (`200`,
  `agent_called=1`) → GREEN after fix (`403`, `agent_called=0`).
- `test_allowed_origin_post_tokenless_executes_105` — allowlisted origin still `200`+105
  (no regression).
- `test_non_browser_loopback_request_tokenless_still_works` — no `Origin` header
  (curl) still works.

**IDE authentication mechanism.** The shipped `GenesisClient.ts` already POSTs to
`/v1/agent` (verified in `runtime/useGenesisChat.ts:47`); it does **not** attach
`X-Genesis-Token`, so a pure IDE request to a tokened server returns `401`
(REGRESSION §B1 L86-87). The dev-harness pre-load injects
`window.__GENESIS_API_TOKEN__` + a fetch wrapper (line 88 claim "validated 200, tool→105")
— this is a **test aid**, not production IDE code. **Recommendation (no browser-global
tokens):** production IDE auth = a build-time env-injected token attached by the same
fetch wrapper (or a localhost reverse proxy), then re-run a graphical E2E against the
tokened `:8473`. The proposed `Origin` gate above is the server-side half of this.

### P1 — Ollama `keep_alive` (payload structure)

**Finding (`aetherius_bridge.py:57-62`).** `_chat()` set `options["keep_alive"]`;
Ollama's `/api/chat` contract requires `keep_alive` as a **top-level** field. Nested
under `options` is silently ignored ⇒ the model is not held ⇒ every `/v1/agent` turn
cold-loads (~70-110 s). Symptom observed live: a request to the published `:8472`
cold-started (`HTTP=000` at 90 s) and `/api/ps` reported `models: []` (not held).

**Fix:**
```python
# genesis_model/aetherius_bridge.py (_chat) — proposed
def _chat(messages):
    options: dict[str, Any] = {"temperature": 0, "num_predict": 256}
    payload: dict[str, Any] = {"model": MODEL, "messages": messages,
                              "stream": False, "think": False, "options": options}
    if _OLLAMA_KEEPALIVE:
        payload["keep_alive"] = _OLLAMA_KEEPALIVE      # top-level, not under options
    ...
```

**Test** (`test_ollama_keep_alive_top_level_in_serialized_request`):
captures the **actual serialized** `/api/chat` request (monkeypatched `urlopen`) and
asserts `payload["keep_alive"]=="30m"` **and** `"keep_alive" not in payload["options"]`.
RED (nested) → GREEN (top-level). This is the "inspect the actual serialized request"
requirement: `GENESIS_OLLAMA_KEEPALIVE=30m` now flows through the ordinary
`/v1/agent → agent → _chat → /api/chat` path with no manual preload. (Ollama's honoring
of the top-level field is its documented behavior.)

### P1 — Tool execution quality

- **Varied operands.** `test_phase4_add_numbers_varied_operands` asserts the real
  `AgentBridgeToolExecutor.execute("phase4.add_numbers", …)` (not a final-answer string)
  returns `{"ok": true, "output": …}` for `{47,58}→105`, `{-5,3}→-2`,
  `{1.5,2.5}→4.0`, `{100,200}→300`, `{-7,-8}→-15`. All PASS ⇒ `add_numbers` honors the
  `{a:number, b:number}` schema (negatives/decimals work); the 47/58 planner example is
  no longer the sole acceptance prompt.
- **`owner_authorized` semantics** (`Agent-Bridge/tools/router.py:32-111`). `call()`
  assigns `owner = bool(context.get("owner_authorized"))` (line 40) but gates tools on
  `admin_active/ADMIN` (81-83), `profile` (77-80), `network_policy` (84-88),
  `model_capabilities` (89-93) — **never** on `owner_authorized`. A read-only probe with
  `owner_authorized=False` still executed `phase4.add_numbers→105` (already in the
  hardened suite). ⇒ This is an **incomplete policy mechanism, not a bypass** (phase4 is
  `READ_ONLY`, so there is no write-action authorization to subvert).
- **Proposed per-tool rule** (Agent-Bridge repo — **off-limits without separate
  approval**; do not modify `router.py` here): mirror the existing `ADMIN` gate with an
  `owner`-driven one:
  ```python
  # tools/router.py, after the admin/network/model gates, before execute:
  if rec.requires_owner and not owner:
      return _fail("OWNER_DENIED", f"{tool_id} requires an authorized owner session")
  ```
  Genesis-side accompaniment: `AgentBridgeToolExecutor` must **stop fabricating**
  `owner_authorized=True` and pass the bridge's verdict through. The existing
  `test_client_cannot_grant_owner_authorized` (negative, Genesis-side) already holds.
  Recommendation: add an `OWNER_DENIED` test in the Agent-Bridge suite (pending
  approval to touch `router.py`).

### P1 — IDE chat lifecycle (`useGenesisChat.ts`)

**Gaps (source-confirmed).** `submit` (line 41) had no re-entrancy guard and no unique-id
(`genesis-${Date.now()}`, ms-collision); `disconnect` (37) only flipped the connection
flag (stale transcript); the `active` effect (87-89) had no `else` (stale messages on
runtime/source switch); no abort of an in-flight `agent()` on unmount/disconnect;
`decide` (63) is a no-op stub (unused by the shell).

**Fixes (minimal, preserve the successful chat flow):**
```ts
// runtime/useGenesisChat.ts — proposed
const busyRef = useRef(false);          // reject re-entrant/double submit
const msgSeqRef = useRef(0);            // unique ids (no Date.now collision)
...
  const submit = useCallback(async (text) => {
    const trimmed = text.trim();
    if (!trimmed) return;
    if (busyRef.current) return;        // ← double-submit guard
    busyRef.current = true; setBusy(true); setError('');
    ...
    finally { busyRef.current = false; setBusy(false); }
  }, []);
...
  const disconnect = useCallback(() => {
    setConnection('disconnected'); setMessages([]); setError('');   // ← clear stale
  }, []);
...
  useEffect(() => {
    if (active) connect();
    else disconnect();                  // ← reset on runtime/source switch away
  }, [active, connect, disconnect]);
```

**Tests** (`useGenesisChat.test.tsx`, added to the existing `useGenesisChat lifecycle`
describe): `rejects a second submit while the first is in flight (double-submit guard)`
(deferred promise; asserts `agent` called **exactly once**),
`clears the transcript on disconnect`,
`clears the transcript when switching away from the genesis runtime`.
RED against the original source (guards absent) → GREEN after the fixes.

**Residual (proposed, not implemented):** reset `GenesisShell` `localMessages` on source
switch too (stale user echoes); `AbortController` signal through `GenesisClient.agent`
for in-flight cancellation on unmount/disconnect. Leave as follow-up — the
`GenesisClient` contract change needs care.

### P2 — Portability (`aetherius_bridge.py`)

**Finding.** `_AGENT_BRIDGE_ROOT` defaulted to `"C:/Users/jpowe/Desktop/Projects/Agent-Bridge"`
— a machine-specific Windows path in published source.

**Fix:** default `""` (env-only) + a clear `ImportError` when `tools/` isn't importable
(was a bare `ModuleNotFoundError` before). `GENESIS_AGENT_BRIDGE_ROOT` must point at a
checkout exposing `tools/{registry,phase4_adapter,router}.py`; unset ⇒ clear error, not
a silent path.

### P2 — Report reconciliation (proposed edits to the published reports)

Reconciliations applied to `docs/reports/*.md` in this worktree (carried into the `review/genesis-postpub-corrections-20261028` publication commit):

1. **`compare_digest` is not brute-force/replay protection.**
   - `GENESIS_SECURITY_REGRESSION_RESULTS.md` §3 L59-60 ("Token brute-force / replay —
     Low — `secrets.compare_digest` used") and §B1 L89 ("brute-force/replay:
     `secrets.compare_digest` used (timing-safe)...") ⇒ reword: `compare_digest` is a
     **constant-time compare** (timing-attack mitigation on the token comparison only);
     it is **neither** brute-force protection (no rate limit/lockout) **nor** replay
     protection (no nonce/timestamp). The "Low" rating is the **loopback-only bind**
     (`127.0.0.1`), not the compare function. Same reword in
     `GENESIS_POST_GATE_D_HARDENING.md` L82.
2. **HTTP authentication ≠ authenticated graphical acceptance.**
   - §B1 L88 ("validated: 200, tool→105") was via the **dev-harness fetch-wrapper**
     token injection, **not the shipped IDE client** (which `401`s on a tokened server).
     Reword to: HTTP auth accepted (phase4→105) ✅; shipped `GenesisClient` does not send
     `X-Genesis-Token` → `401` on tokened `:8473` ✅; authenticated **graphical**
     acceptance achieved only through the harness token-injection shim (a test aid), not
   - the production IDE code path. Do **not** claim authenticated graphical acceptance
     for the shipped IDE until a real browser session is wired with `X-Genesis-Token`.
3. **8473 traceback framing.** `GENESIS_IDE_RUNTIME_GAP_ANALYSIS.md` §A3-update L318
   hedges "(or was rebound?)". Clean to: "After a successful auth matrix on `:8473`, the
   listener emitted a Python `Traceback` to stderr; on re-check it served `200` — the
   listener **persisted** (non-reproducible); `:8474` served the identical matrix intact;
   cause **undetermined** (no traceback body captured, no Windows crash events, Ollama/GPU
   stable). This is a Python `Traceback`, distinct from the `:8471` SIGKILL (exit 2304)."
   Remove any conflation of 8473 (Traceback) with 8471 (SIGKILL).
4. **Supervisor contradiction.** `GENESIS_POST_GATE_D_HARDENING.md` L284 ("no
   supervisor located") contradicts `GENESIS_IDE_RUNTIME_GAP_ANALYSIS.md` §5 L82-84
   (recovery script `proc_e42e7c68` re-binds `:8471` via `pkill`/`setsid ollama serve`).
   Reword L284 → "An external supervisor (recovery script `proc_e42e7c68`) re-binds
   `:8471`; not a hermes-profile cron."
5. **Stale "auth-on path untested" note.** `GENESIS_SECURITY_REGRESSION_RESULTS.md` §3 L59
   ("Medium — token-gating unverified live") is **superseded** by §B1 (live `:8473` 8/8
   auth scenarios PASS). Strike or mark resolved.

---

## 3. Test results

- **Python** (isolated Genesis worktree, venv `.venv-torch-probe`, torch 2.14.1+cpu):
  `python -m pytest tests/test_postpub_corrections.py tests/test_genesis_security.py -q`
  → **27 passed** (8 new correction tests incl. invalid-operand rejection plus the
  keep_alive / Origin / portability / owner-false guards, and 19 published
  regression). No regressions. `aetherius_bridge.py:140` self-grant corrected
  (owner_authorized True→False); phase4 tools are read-only and remain
  executable (reads do not require owner authority).
- **TypeScript** (isolated IDE worktree, vitest 5 + jsdom, node 22):
  `useGenesisChat.test.tsx` → **9/9** (6 existing + 3 new); full IDE suite
  **484/484** (was 483/484 — a pre-existing timing flake in
  `useGenesisRuntime.test.tsx` approval gating, reproduced under full-suite
  load; root cause: the 1500ms poll `applySession` tick reset `snapshot.error`
  to empty between `decide()` setting it to `approval expired` and the bare
  `expect`). `useGenesisRuntime` was untouched by the P1-IDE corrections (only
  `useGenesisChat.*` changed). Fixed non-weakeningly by calling `disconnect()`
  before the assertion — stops the poll, and `disconnect` preserves the pending
  error (L193); the refused-decision value still must match exactly).
  `tsc --noEmit` clean on touched files.
- **Inference backend (distinguished):** phase4.add_numbers→105 executes
  through **externally-supplied Ollama inference** (`qwen3.5:2b-q4_K_M` on
  `:11434`), NOT the native Genesis `genesis-native-v06` model. Server-side
  schema validation, argument validation and owner authorization are enforced
  regardless of the inference backend.

## 4. Live HTTP evidence (2026-10-28)

- **P0 (smoke gun):** `curl -X POST http://127.0.0.1:8472/v1/agent -H "Origin: http://evil.example" -H "Content-Type: application/json" -d '{"task":"Calculate 47 + 58 using Agent Bridge."}'` → `HTTP 200`, `answer:"…**105**"`, `tool_calls:[{name:"phase4.add_numbers", arguments:{a:47,b:58}}]`, `tool_results:[{name:"phase4.add_numbers", ok:true, output:105}]`. (Published, pre-correction; proves CORS ≠ authz on the tokenless path.)
- **Token gate:** `POST :8473/v1/agent` with no/invalid `X-Genesis-Token` → `401`.
- **keep_alive symptom:** published `:8472` cold-starts the model on first `/v1/agent`
  (`HTTP=000` within 90 s); corrected payload asserts `keep_alive` at top level.

## 5. Outstanding blockers / approvals needed

1. **Agent-Bridge `router.py:40` `owner_authorized` enforcement** — production repo;
   requires separate approval (not modified here). Genesis-side must stop fabricating
   `owner_authorized=True`.
2. **Authenticated graphical IDE acceptance** — shipped `GenesisClient` must attach
   `X-Genesis-Token` via a build-time env var (not a browser global), then a graphical
   E2E against tokened `:8473`. Dev-harness injection is a test aid only.
3. **`8471` SIGKILL (exit 2304) instrumentation** — add an RSS/`nvidia-smi` sampler around
   the kill to confirm/disprove OOM and capture the next `8473` traceback body. Separate
   instrumentation approval; not performed (constraint). Cause remains undetermined.
4. **Publish the corrections** — all changes are staged **uncommitted** in the isolated
   worktrees above; awaiting a lift of the "no commit/push" directive to commit to the
   existing `review/genesis-v06-hardening-20261028` / `review/genesis-ide-hardening-20261028`
   branches (separate commits, no force-push).

## 6. Completion

`GENESIS_POST_PUBLICATION_REVIEW_READY`. All P0/P1/P2 corrections are implemented and
green in isolated worktrees (26/26 Python, 484/484 TypeScript) and the P0 defect is
proven live; the only remaining items require approvals explicitly out of scope for this
correction pass (Agent-Bridge edits, IDE token wiring, kill instrumentation, publication
commit). Proposed report-reconciliation edits are listed in §2 P2 for a follow-up commit.
