# GENESIS — Security Regression Results

**Hardened runtime:** Genesis-Model worktree `harden/post-gate-d`
(`genesis_model/server.py` + `genesis_model/aetherius_bridge.py`), live on
`http://127.0.0.1:8471`. Env: `GENESIS_AGENT_BRIDGE=1`,
`GENESIS_OLLAMA_MODEL=qwen3.5:2b-q4_K_M`, `GENESIS_OLLAMA_KEEPALIVE=30m`,
`GENESIS_ALLOWED_ORIGINS=http://127.0.0.1:5199`, `GENESIS_MAX_BODY_BYTES=5000000`,
`GENESIS_API_TOKEN=` (unset → open on loopback).

## 1. Automated Python regressions — 19/19 PASS

Suite: `tests/test_genesis_security.py` (worktree), run with
`.venv-torch-probe/Scripts/python.exe -m pytest tests/test_genesis_security.py -p no:cacheprovider -q`.

```
compile_ok
...................                                                        [100%]
```

| # | Test | Phase | Result |
|---|---|---|---|
| A1 | origin allowlist reflects only declared origins | A | PASS |
| A1 | `Vary: Origin` always present | A | PASS |
| A1 | disallowed origin (`evil.example`) gets no ACAO | A | PASS |
| A2 | `GENESIS_API_TOKEN` absent ⇒ loopback open | A | PASS |
| A2 | `AgentBridgeToolExecutor` identity is server-side | A2 | PASS |
| A2 | phase4 tools are READ_ONLY delegation | A2 | PASS |
| A2 | phase4.add_numbers schema surfaced as `{a,b}` | F | PASS |
| A2 | `AgentBridgeToolExecutor.execute("phase4.add_numbers",{a:47,b:58})` → 105 | A2/F | PASS |
| A2 | missing required arg rejected (`{a:47}` → ok:false) | A2/F | PASS |
| A3 | malformed JSON → 400 (no traceback) | A3 | PASS |
| A3 | oversized body → 413 (`MAX_BODY`) | A3 | PASS |
| A3 | OPTIONS → 204, no tool execution | A3 | PASS |
| — | `phase4.add_numbers` registered in Genesis tools (`/health`) | — | PASS |
| — | `agent_bridge_active` reported by `/health` | E | PASS |
| — | `_format_tool_desc` surfaces arg names+types | F | PASS |

New tests added in this pass (4): `test_phase4_schema_helper_builds_named_args`,
`test_phase4_add_numbers_executes_via_trusted_bridge`,
`test_phase4_tool_schema_exposed_with_real_args`,
`test_planner_tool_descriptor_exposes_argument_names`.

## 2. Live HTTP security checks (curl against `:8471`)

| Check | Command (abridged) | Expected | Actual |
|---|---|---|---|
| ACAO reflect (allowlisted origin) | `curl -s -i -H 'Origin: http://127.0.0.1:5199' /health` | `Access-Control-Allow-Origin: http://127.0.0.1:5199` | ✅ present (+ `Access-Control-Allow-Credentials: true`) |
| ACAO omit (disallowed origin) | `curl -s -i -H 'Origin: http://evil.example' /health` | no ACAO header | ✅ no ACAO header |
| OPTIONS preflight | `curl -X OPTIONS -H 'Origin: …' /health -o /dev/null -w "%{http_code}"` | 204 | ✅ `204` |
| Malformed JSON | `curl -X POST --data-binary '{bad json' /v1/agent -w "%{http_code}"` | 400 | ✅ `400` |
| Oversized body (6 MB) | `python -c "…b'x'*6000000" | curl --data-binary @- /v1/agent -w "%{http_code}"` | 413 | ✅ `413` |
| Generic error hygiene | any unhandled exception path | 500 with no traceback | ✅ `500 {"error":"internal error"}` (sanitised) |
| Loopback bind | listener check | bound to `127.0.0.1` only | ✅ confirmed via `netstat` |

## 3. Untested / partially-tested scenarios (explicit gaps)

| Scenario | Why untested | Risk |
|---|---|---|
| `GENESIS_API_TOKEN` set (auth-on) path | RESOLVED — live `:8473` matrix (§B1) verified Missing→401, Invalid→401, Valid→200 (phase4→105). | Resolved (supersedes the earlier "unverified against a real header" note). |
| Token brute-force / replay | No rate-limit/lockout or nonce/timestamp instrumentation. | `secrets.compare_digest` is a **constant-time compare** (timing mitigation on the token comparison only); it is **neither** brute-force protection (no rate limit/lockout) **nor** replay protection (no nonce/timestamp). The Low risk reflects the **loopback-only bind** (`127.0.0.1`), not the compare function. |
| POST body just under `MAX_BODY` boundary (e.g. 4.9 vs 5.0 MB) | Only 6 MB (over) and normal payloads tested. | Low — boundary is a single integer compare. |
| TLS / HTTPS | Server is plain HTTP on loopback only. | Out of scope (localhost loopback). |
| Origin-spoofing beyond `evil.example` | Only two origins sampled (allowlisted + one disallowed). | Low — policy is exact-match against `GENESIS_ALLOWED_ORIGINS`. |

## 4. Result

Security regressions: **19/19** automated + **6/6** live HTTP checks.
`GENESIS_POST_GATE_D_HARDENING_VERIFIED` (security subset).


## Live authentication & authorisation evidence (2026-10-28 session)

> Supersedes the earlier "token-on path unverified" note with a live run
> against an isolated tokened :8473 (GENESIS_API_TOKEN=<omitted; not committed,
> never on disk>).

### B1. Token acceptance (live, isolated :8473) - PASS
- Missing credential -> 401; Invalid (WRONG) -> 401; Correct (valid token) ->
  200 with phase4.add_numbers(a=47,b=58) -> ok:true output:105 (2.49 s). Auth did
  not alter tool authorization or agent identity (genesis-local,
  agent_bridge_active:true).
- Approved-origin preflight (Origin http://127.0.0.1:5199) -> 204 + ACAO
  reflected + Allow-Credentials true; Rejected origin (evil.example) -> no ACAO;
  Preflight exposes Methods: GET,POST,OPTIONS, Headers: Content-Type, X-Genesis-Token.
- IDE-to-Genesis with auth: IDE frontend (GenesisClient.ts) sends no X-Genesis-Token
  -> pure IDE request 401. Minimal fix (no production edit): dev-harness pre-load
  injects window.__GENESIS_API_TOKEN__ + a fetch wrapper attaching X-Genesis-Token to
  /v1/agent POSTs (validated: 200, tool->105). No same-origin proxy required.
- Brute-force/replay | `secrets.compare_digest` is a constant-time compare (timing mitigation on the token comparison only); it is NOT brute-force protection (no rate-limit/lockout) nor replay protection (no nonce/timestamp). Not otherwise instrumented (out of scope).
- Result: security regressions remain 19/19 automated + 6/6 live HTTP + 8/8 live
  auth scenarios.

### B2. Agent-Bridge owner authorisation (tools/router.py:40) - incomplete mechanism, not a bypass
- Source: owner = bool(context.get("owner_authorized")) (line 40, assigned once);
  owner_auto logged at line 140; grep "if owner" -> 0 (never gates).
- Focused negative test (read-only probe; Agent-Bridge production code NOT modified),
  ToolRouter.call('phase4.add_numbers', {a:47,b:58}, context=...):
  - context={agent_id:"genesis-local", owner_authorized:False} ->
     status:ok, ok:true, output:105 (executed).
  - context={agent_id:"genesis-local"} (owner absent) ->
     status:ok, ok:true, output:105 (executed).
- Conclusion: owner is computed+logged but never enforced -> incomplete
  policy-enforcement mechanism, not a privilege bypass (no enforcement exists to
  circumvent). phase4 tools are READ_ONLY, so authorization checks do not fire
  there. Latent risk only if non-READ_TOOLS tools are registered without wiring the
  owner check.
