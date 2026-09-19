"""Genesis status + approval + verification facades (public data only)."""
from __future__ import annotations

from typing import Any

from genesis_runtime.models import GenesisStatus


def build_status(dashboard: dict[str, Any], caps: dict[str, Any],
                 health: dict[str, Any]) -> GenesisStatus:
    tasks = dashboard.get("tasks", {}) or {}
    active = [t for t, s in tasks.items() if isinstance(s, str) and s not in
              ("COMPLETED", "FAILED", "CANCELLED", "ROLLED_BACK")]
    files = dashboard.get("files_touched", []) or []
    tests = dashboard.get("tests", {}) or {}
    comp = dashboard.get("competence", {}) or {}
    progress = f"{len(files)} file(s), {tests.get('passed', 0)}/{tests.get('ran', 0)} tests"
    return GenesisStatus(
        runtime=str(caps.get("runtime", "")),
        ollama=str((health.get("checks") or {}).get("ollama", {}).get("status", "")),
        session=str(dashboard.get("session_id", "")),
        task=",".join(active) or ",".join(tasks),
        mode=str(dashboard.get("mode", "")),
        role="", model=str((dashboard.get("models") or {}).get("coder", "")),
        step=int(dashboard.get("step", 0) or 0), progress=progress,
        context="budget ok",
        files=[str(f) for f in files], tests=dict(tests),
        review=str(dashboard.get("review", "")),
        approvals=[str(a) for a in dashboard.get("pending_approvals", [])],
        verification={"scorecard": dashboard.get("scorecard", {})},
        errors=[str(e) for e in dashboard.get("errors", [])],
        network_policy=str(caps.get("network_policy", "")))


def approval_view(pending: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Sanitized approval cards (untrusted model strings stay inert data)."""
    cards = []
    for p in pending:
        action = p.get("action", {}) or {}
        ctx = p.get("context", {}) or {}
        cards.append({
            "id": str(p.get("id", "")),
            "risk": str(ctx.get("risk", ""))[:40],
            "action": str(action.get("action", ""))[:40],
            "target": str(action.get("path", action.get("command", "")))[:300],
            "reason": str(ctx.get("reason", ""))[:500],
            "diff_preview": str(ctx.get("diff_preview", ""))[:1500],
        })
    return cards


def verification_view(scorecard: dict[str, Any]) -> dict[str, Any]:
    cats = (scorecard.get("scorecard", {}) or {}).get("categories", {}) or {}
    return {k: {"status": v.get("status", "UNKNOWN"),
                "detail": str(v.get("detail", ""))[:200]}
            for k, v in cats.items()}
