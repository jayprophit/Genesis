"""Genesis event translation: raw runtime events -> stable GENESIS_* events.

IDs (action/session) preserved. Nothing is fabricated from model prose:
only runtime state transitions and verified outcomes become events.
"""
from __future__ import annotations

from typing import Any

_TRANSLATION = {
    "session.created": "GENESIS_SESSION_CREATED",
    "planning.started": "GENESIS_PLANNING",
    "planning.completed": "GENESIS_PLAN_READY",
    "execution.started": "GENESIS_ACTION_REQUESTED",
    "approval.requested": "GENESIS_APPROVAL_REQUIRED",
    "execution.completed": "GENESIS_EXECUTING",
    "execution.failed": "GENESIS_EXECUTING",
    "task.cancelled": "GENESIS_CANCELLED",
    "rollback.started": "GENESIS_ROLLBACK_STARTED",
    "rollback.completed": "GENESIS_ROLLED_BACK",
    "task.completed": "GENESIS_COMPLETED",
    "task.failed": "GENESIS_FAILED",
    "gate.waiting": "GENESIS_APPROVAL_REQUIRED",
    "review.started": "GENESIS_REVIEWING",
    "review.completed": "GENESIS_REVIEWING",
}


def translate(event: dict[str, Any]) -> dict[str, Any]:
    name = str(event.get("event", event.get("bus_event", "")))
    out: dict[str, Any] = {"genesis_event": _TRANSLATION.get(name, ""),
                           "session_id": event.get("session_id", ""),
                           "task_id": event.get("task_id", ""),
                           "action_id": event.get("action_id", ""),
                           "timestamp": event.get("timestamp", "")}
    action = event.get("action") or {}
    if isinstance(action, dict) and action.get("action"):
        out["genesis_event"] = out["genesis_event"] or "GENESIS_ACTION_REQUESTED"
        out["action"] = action.get("action")
        result = event.get("result") or {}
        if event.get("executed"):
            if result.get("ok"):
                if action.get("action") == "test":
                    out["genesis_event"] = ("GENESIS_TEST_PASSED"
                                            if result.get("passed", True)
                                            else "GENESIS_TEST_FAILED")
                elif action.get("action") in ("write", "edit", "patch"):
                    out["genesis_event"] = "GENESIS_FILE_CHANGED"
                else:
                    out["genesis_event"] = "GENESIS_EXECUTING"
            elif action.get("action") == "test":
                out["genesis_event"] = "GENESIS_TEST_FAILED"
                out["test_running"] = False
            else:
                out["genesis_event"] = "GENESIS_EXECUTING"
        elif action.get("action") == "test":
            out["genesis_event"] = "GENESIS_TEST_RUNNING"
    if event.get("verdict") in ("approve", "revise", "reject", "escalate"):
        out["genesis_event"] = "GENESIS_REVISION_REQUIRED" \
            if event.get("verdict") == "revise" else "GENESIS_REVIEWING"
        out["verdict"] = event.get("verdict")
    if event.get("finished"):
        out["genesis_event"] = "GENESIS_COMPLETED"
    return out


def translate_all(events: list[dict[str, Any]]) -> list[dict[str, Any]]:
    out = []
    for e in events:
        t = translate(e)
        if t["genesis_event"]:
            out.append(t)
    return out
