# Genesis Integration 01 audit — 2026-09-10T10:40:41

integration 0.1.0 | runtime 0.5 | api v1

## Tests
{
 "automated_tests": 44,
 "result": "OK",
 "live_e2e": 8,
 "live_result": "OK"
}

## Security boundary
{
 "runtime": "read-only dependency",
 "allowed_imports": [
  "client"
 ],
 "forbidden": [
  "executor",
  "policy",
  "protocol",
  "providers",
  "checkpoints",
  "bridge",
  "sandbox internals",
  "approval internals"
 ],
 "transport": "HTTP 127.0.0.1 /v1",
 "filesystem": "sandbox_genesis synthetic only"
}

## E2E
{
 "plan": "zero mutation, COMPLETED",
 "approval": "deny-by-default enforced; approve-once via policy",
 "rollback": "bytes restored",
 "revision": "linked child, history preserved",
 "cancel": "CANCELLED, no later mutation",
 "synthetic": "get_status added, tests pass, gate accepted",
 "failure": "contained, no proof claims",
 "verification": "scorecard categorical, no numeric score"
}

## Limitations
- v0.5 sessions are non-interactive over HTTP: interactive approve-once cannot be granted live (deny-by-default proven; v0.6 needs per-session interactive flag)
- 3B model needs rigid single-line task wording; slow periods observed
- PRECIOUS live mutation awaits interactive sessions (v0.6)
- no cloud, no browser, no avatar (out of scope)