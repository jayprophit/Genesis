"""Emit the Stage-01 audit report (machine + human readable)."""
import json

from genesis_runtime import audit as audit_mod

report = audit_mod.build(
    version="0.1.0",
    runtime_version="0.5",
    api_version="v1",
    test_summary={"automated_tests": 44, "result": "OK",
                  "live_e2e": 8, "live_result": "OK"},
    boundary={"runtime": "read-only dependency",
              "allowed_imports": ["client"],
              "forbidden": ["executor", "policy", "protocol", "providers",
                            "checkpoints", "bridge", "sandbox internals",
                            "approval internals"],
              "transport": "HTTP 127.0.0.1 /v1",
              "filesystem": "sandbox_genesis synthetic only"},
    e2e={"plan": "zero mutation, COMPLETED",
         "approval": "deny-by-default enforced; approve-once via policy",
         "rollback": "bytes restored",
         "revision": "linked child, history preserved",
         "cancel": "CANCELLED, no later mutation",
         "synthetic": "get_status added, tests pass, gate accepted",
         "failure": "contained, no proof claims",
         "verification": "scorecard categorical, no numeric score"},
    limitations=[
        "v0.5 sessions are non-interactive over HTTP: interactive "
        "approve-once cannot be granted live (deny-by-default proven; "
        "v0.6 needs per-session interactive flag)",
        "3B model needs rigid single-line task wording; slow periods observed",
        "PRECIOUS live mutation awaits interactive sessions (v0.6)",
        "no cloud, no browser, no avatar (out of scope)",
    ],
)
open("audit_report.json", "w").write(json.dumps(report, indent=1))
open("audit_report.md", "w").write(audit_mod.render_text(report))
print("AUDIT_OK", report["generated_at"])
