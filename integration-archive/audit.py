"""Machine-readable + human-readable integration audit report. No secrets."""
from __future__ import annotations

import json
from datetime import datetime
from typing import Any


def build(version: str, runtime_version: str, api_version: str,
          test_summary: dict[str, Any], boundary: dict[str, Any],
          e2e: dict[str, Any], limitations: list[str]) -> dict[str, Any]:
    return {
        "integration": "genesis_integration_01",
        "integration_version": version,
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "runtime_version": runtime_version,
        "api_version": api_version,
        "tests": test_summary,
        "security_boundary": boundary,
        "public_imports": ["client"],
        "model_used": "hhao/qwen2.5-coder-tools:3b (local Ollama)",
        "sandbox_used": "sandbox_genesis (synthetic only)",
        "approval_tests": e2e.get("approval", {}),
        "rollback_tests": e2e.get("rollback", {}),
        "verification": e2e.get("verification", {}),
        "e2e": e2e,
        "known_limitations": limitations,
    }


def render_text(report: dict[str, Any]) -> str:
    lines = [f"# Genesis Integration 01 audit — {report['generated_at']}", "",
             f"integration {report['integration_version']} | "
             f"runtime {report['runtime_version']} | api {report['api_version']}",
             "", "## Tests", json.dumps(report["tests"], indent=1),
             "", "## Security boundary",
             json.dumps(report["security_boundary"], indent=1),
             "", "## E2E", json.dumps(report["e2e"], indent=1)[:4000],
             "", "## Limitations"]
    lines += [f"- {l}" for l in report["known_limitations"]]
    return "\n".join(lines)
