# Genesis Integration Stage 01

First controlled proof that a Genesis-style application layer can consume
the verified Local Agent Runtime v0.5 through PUBLIC interfaces only.
v0.5 is treated as a read-only external dependency (never modified here).
This is NOT the real Genesis application.

```
GENESIS APPLICATION LAYER (this workspace)
        ↓  AgentRuntimeClient / HTTP /v1 only
AGENT RUNTIME v0.5 (external, read-only)
        ↓  session / mode / policy / router
LOCAL MODEL (Ollama) → SANDBOXED EXECUTION → VERIFICATION
```

## Layout

```
genesis_runtime/      adapter, models, events, status, approvals, config,
                      discovery, scoping, memory, selfcheck, audit, v05client
sandbox_genesis/      synthetic Genesis-style project (fixture ONLY)
tests/                architecture + components + live E2E
make_audit.py         audit report generator
audit_report.json/md  generated audit artefacts
```

## Permission profiles → runtime mapping

- SAFE_EXPLORATION → plan + READ_ONLY (strictly no mutation)
- ASSISTED_BUILD → build + ASK_ALL_WRITES
- AUTONOMOUS_SANDBOX → hybrid + AUTO_SAFE
- PRECIOUS_PROJECT → ASK_ALL_WRITES + BEFORE_COMPLETE gate + backups
  (locked spec in `genesis_runtime/config.py`)

Profiles map; runtime decides. A user demanding "rewrite everything" only
sets intent — destructive execution still needs runtime approval.

## What was proven (all executed)

- Plan-only: inspection + plan, ZERO project mutation.
- Assisted build + final gate: approved-style execution, real tests,
  review, explicit accept (no auto-complete).
- Deny: ASK_ALL_WRITES/PRECIOUS mutations denied by default; file
  byte-identical; APPROVAL_DENIED in exported evidence.
- Rollback: approved change → preview → rollback → original bytes.
- Revision: linked child task, original history preserved, rerun + review.
- Cancel: CANCELLED, no later mutation, events flushed, rollback available.
- Synthetic E2E: `get_status()` added, existing functions preserved, tests
  exit 0, gate accepted, independently verified.
- Failure: unprovable demand contained, no proof claims.
- Discovery/manifest/search/classify/secrets/budgets/scoping all live-tested.

## Known honest limitation

v0.5 sessions are non-interactive over HTTP, so interactive approve-once
cannot be granted live (deny-by-default proven instead). PRECIOUS live
mutation therefore awaits a v0.6 per-session interactive flag. The
approve-once mechanics themselves are covered by the runtime's own lineage.

## Verification

```powershell
python -m unittest tests.test_architecture tests.test_components tests.test_selfcheck_audit
python -m unittest tests.test_e2e_live.TestCancel  # + other live classes
python make_audit.py
```

No cloud, no telemetry, no real-Genesis access. Sandbox evidence under
`sandbox_genesis/.bridge`. See `audit_report.md` and the final stage report.
Future work: Stage 02 real-Genesis READ-ONLY discovery (recommendation §32).
