"""Genesis memory + verification facades (read-only, task-scoped).

Future long-term Genesis memory stays separate from execution truth:
this interface only surfaces what the runtime already recorded for a task.
Genesis can never edit protected runtime memory files through here.
"""
from __future__ import annotations

from typing import Any


def task_memory(status: dict[str, Any]) -> dict[str, Any]:
    tasks = status.get("tasks", {}) or {}
    return {
        "goal": status.get("task", ""),
        "decisions": [],
        "completed_actions": status.get("files_touched", []) or [],
        "files_touched": status.get("files_touched", []) or [],
        "test_results": status.get("tests", {}) or {},
        "review_findings": status.get("review", "") or "",
        "unresolved_issues": [e for e in status.get("errors", []) or []],
        "note": "read-only view; runtime memory files are not writable here",
    }


def cache_info() -> dict[str, Any]:
    return {"status": "runtime-managed",
            "note": "no direct cache access in Stage 01 (future capability)"}


def verification_summary(scorecard: dict[str, Any]) -> dict[str, Any]:
    cats = (scorecard.get("scorecard", {}) or {}).get("categories", {}) or {}
    return {k: v.get("status", "UNKNOWN") for k, v in cats.items()}
