"""Genesis approval decisions (requests live server-side; UI only resolves)."""
from __future__ import annotations

APPROVE_ONCE = "approve-once"
APPROVE_SESSION = "approve-session"
DENY = "deny"

DECISIONS = (APPROVE_ONCE, APPROVE_SESSION, DENY)


def check_decision(decision: str) -> str:
    if decision not in DECISIONS:
        raise ValueError(f"bad approval decision: {decision!r}")
    return decision
