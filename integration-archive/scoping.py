"""Task scoping + change budgets (integration level).

Scope derivation is advisory: it never grants permission by itself.
Runtime quotas + approvals stay authoritative underneath.
"""
from __future__ import annotations

from typing import Any

from genesis_runtime.config import CHANGE_BUDGETS


def derive_scope(task_text: str, index: dict[str, Any],
                 protected: list[str]) -> dict[str, Any]:
    import re
    words = {w.lower() for w in re.findall(r"\w+", task_text) if len(w) > 3}
    likely, tests = [], []
    for e in index.get("entries", [])[:60]:
        hay = (e["path"] + " " + e.get("excerpt", "")).lower()
        if any(w in hay for w in words):
            (tests if e["kind"] == "TEST" else likely).append(e["path"])
    text = task_text.lower()
    risk = "low"
    if any(k in text for k in ("delete", "remove all", "rewrite everything",
                              "drop", "reset", "production")):
        risk = "high"
    elif any(k in text for k in ("modify", "patch", "add", "move", "rename",
                                 "install")):
        risk = "medium"
    prot_hits = [p for p in protected
                 if any(p.lower().rstrip("/") in (e.get("path", "").lower())
                        for e in index.get("entries", []))]
    return {"likely_files": likely[:15], "protected_files": prot_hits,
            "test_files": tests[:10],
            "expected_commands": ["python -m pytest"] if tests else [],
            "risk": risk,
            "note": "advisory scope only; runtime policy decides"}


class ChangeBudget:
    def __init__(self, profile: str = "ASSISTED_BUILD"):
        self.limits = dict(CHANGE_BUDGETS.get(profile, CHANGE_BUDGETS["ASSISTED_BUILD"]))
        self.profile = profile

    def check_files(self, created: list, modified: list) -> tuple[bool, str]:
        if len(created) > self.limits["max_files_created"]:
            return False, f"budget exceeded: {len(created)} files created " \
                          f"(max {self.limits['max_files_created']})"
        if len(created) + len(modified) > self.limits["max_files_changed"]:
            return False, "budget exceeded: too many files changed"
        return True, ""

    def check_commands(self, n: int) -> tuple[bool, str]:
        if n > self.limits["max_commands"]:
            return False, f"budget exceeded: {n} commands"
        return True, ""
