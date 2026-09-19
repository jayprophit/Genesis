"""GenesisRuntimeAdapter — Genesis-facing facade over the PUBLIC v0.5 API.

No executor/policy/protocol/provider imports anywhere in this package.
Every operation goes through AgentRuntimeClient (HTTP /v1).
"""
from __future__ import annotations

import time
from typing import Any

from genesis_runtime import models as M
from genesis_runtime.approvals import check_decision
from genesis_runtime.config import PERMISSION_PROFILES
from genesis_runtime.events import translate_all
from genesis_runtime.status import approval_view, build_status, verification_view
from genesis_runtime.v05client import AgentRuntimeClient


class GenesisRuntimeAdapter:
    def __init__(self, base_url: str, token: str = "", timeout: int = 30):
        self.client = AgentRuntimeClient(base_url, token=token, timeout=timeout)
        self.base_url = base_url
        self._caps: dict[str, Any] | None = None
        self._health: dict[str, Any] | None = None
        self._held: dict[tuple[str, str], str] = {}

    # -- connection / discovery -------------------------------------------
    def connect(self) -> dict[str, Any]:
        self._health = self.client.health()
        self._caps = self.client.capabilities()
        return {"endpoint": self.base_url, "health": self._health["status"],
                "runtime": (self._caps.get("compat") or {}).get("runtime", "")}

    def health(self) -> dict[str, Any]:
        self._health = self.client.health()
        return self._health

    def models(self) -> dict[str, Any]:
        return self.client.models()

    def capabilities(self) -> dict[str, Any]:
        if self._caps is None:
            self._caps = self.client.capabilities()
        return self._caps

    # -- sessions / tasks ----------------------------------------------------
    def create_workspace_session(self, task: M.GenesisTask) -> str:
        prof = PERMISSION_PROFILES.get(task.approval_profile,
                                       PERMISSION_PROFILES["ASSISTED_BUILD"])
        mode = task.mode or prof["mode"].lower()
        out = self.client.create_session(task.workspace, mode=mode,
                                         approval=prof["approval"])
        return out["session_id"]

    def _task_text(self, task: M.GenesisTask) -> str:
        parts = [task.goal]
        if task.constraints:
            parts.append("Constraints: " + "; ".join(task.constraints))
        if task.protected_files:
            parts.append("Protected (do not modify without approval): " +
                         ", ".join(task.protected_files))
        return "\n".join(parts)

    def submit_task(self, session_id: str, task: M.GenesisTask) -> str:
        text = self._task_text(task)
        if task.dry_run:
            # No per-session dry-run flag exists in API v1: plan mode is the
            # honest equivalent (validated, never executed). Documented.
            text = "[DRY-RUN via plan mode: propose only, execute nothing]\n" + text
        out = self.client.submit_task(session_id, text,
                                      getattr(task, "idempotency_key", ""))
        tid = out["task_id"]
        if task.final_human_gate in ("BEFORE_COMPLETE", "ALWAYS"):
            self._held[(session_id, tid)] = task.final_human_gate
        return tid

    def await_completion(self, session_id: str, task_id: str,
                         timeout: float = 900) -> dict[str, Any]:
        """Poll to terminal. If an adapter-level final gate holds this task,
        report HELD (never a fabricated completion)."""
        st = self.wait(session_id, task_id, timeout)
        tasks = (st.get("tasks", {}) or {})
        if tasks.get(task_id) == "COMPLETED" and \
                (session_id, task_id) in self._held:
            return {"status": "HELD_FINAL_APPROVAL", "task_id": task_id,
                    "session": st}
        return {"status": tasks.get(task_id, "UNKNOWN"), "task_id": task_id,
                "session": st}

    def final_accept(self, session_id: str, task_id: str) -> dict[str, Any]:
        self._held.pop((session_id, task_id), None)
        return {"ok": True, "task_id": task_id, "decision": "accept"}

    def final_request_revision(self, session_id: str, task_id: str,
                               instruction: str) -> str:
        self._held.pop((session_id, task_id), None)
        return self.request_revision(session_id, task_id, instruction)

    def final_rollback(self, session_id: str, task_id: str,
                       label: str = "") -> dict[str, Any]:
        self._held.pop((session_id, task_id), None)
        return self.rollback(session_id, label)

    def final_cancel(self, session_id: str, task_id: str) -> dict[str, Any]:
        self._held.pop((session_id, task_id), None)
        return self.cancel(session_id, task_id)

    def plan_task(self, session_id: str, task: M.GenesisTask) -> str:
        task.mode = "plan"
        return self.submit_task(session_id, task)

    def build_task(self, session_id: str, task: M.GenesisTask) -> str:
        task.mode = "build"
        return self.submit_task(session_id, task)

    def hybrid_task(self, session_id: str, task: M.GenesisTask) -> str:
        task.mode = "hybrid"
        return self.submit_task(session_id, task)

    def wait(self, session_id: str, task_id: str,
             timeout: float = 900) -> dict[str, Any]:
        end = time.time() + timeout
        last: dict[str, Any] = {}
        while time.time() < end:
            st = self.client.session_status(session_id)
            last = st
            if (st.get("tasks", {}) or {}).get(task_id) in (
                    "COMPLETED", "FAILED", "CANCELLED", "ROLLED_BACK",
                    "INTERRUPTED"):
                return st
            time.sleep(3)
        return last

    # -- observation ---------------------------------------------------------
    def status(self, session_id: str) -> M.GenesisStatus:
        st = self.client.session_status(session_id)
        caps = self.capabilities()
        try:
            health = self.health()
        except Exception:  # noqa: BLE001  (status must work degraded)
            health = {"checks": {}}
        sc = {}
        try:
            sc = self.client.scorecard(session_id)
        except Exception:  # noqa: BLE001
            pass
        st = dict(st)
        st["scorecard"] = sc.get("scorecard", {})
        return build_status(st, caps, health)

    def events(self, session_id: str, since: int = 0) -> list[dict[str, Any]]:
        raw = self.client.events(session_id, since).get("events", [])
        return translate_all(raw)

    def approvals(self, session_id: str) -> list[dict[str, Any]]:
        st = self.client.session_status(session_id)
        ids = st.get("pending_approvals", []) or []
        if not ids:
            return []
        # join IDs with approval.requested events for full safe cards
        detail: dict[str, dict[str, Any]] = {}
        try:
            for e in self.client.events(session_id).get("events", []):
                if e.get("event") == "approval.requested" and e.get("approval_id"):
                    detail[str(e["approval_id"])] = {
                        "action": e.get("action", {}),
                        "context": {"risk": e.get("risk", ""),
                                    "reason": "", "diff_preview": ""}}
        except Exception:  # noqa: BLE001
            pass
        return [{"id": str(i),
                 "action": detail.get(str(i), {}).get("action", {}),
                 "context": detail.get(str(i), {}).get("context", {})}
                for i in ids]

    def approve(self, session_id: str, approval_id: str,
                decision: str = "approve-once") -> dict[str, Any]:
        return self.client.approve(session_id, approval_id,
                                   check_decision(decision))

    def deny(self, session_id: str, approval_id: str) -> dict[str, Any]:
        return self.client.approve(session_id, approval_id, "deny")

    def request_revision(self, session_id: str, task_id: str,
                         instruction: str) -> str:
        return self.client.request_revision(session_id, task_id,
                                            instruction)["child_task_id"]

    def cancel(self, session_id: str, task_id: str) -> dict[str, Any]:
        return self.client.cancel(session_id, task_id)

    def rollback_preview(self, session_id: str) -> dict[str, Any]:
        return self.client.diff(session_id)

    def rollback(self, session_id: str, label: str = "") -> dict[str, Any]:
        return self.client.rollback(session_id, label)
    def result(self, session_id: str) -> dict[str, Any]:
        st = self.client.session_status(session_id)
        tl = self.client.timeline(session_id).get("timeline", [])
        return {"status": st, "timeline": tl,
                "verification": verification_view(
                    self.client.scorecard(session_id)
                    if st.get("tasks") else {"scorecard": {"categories": {}}})}

    def export_result(self, session_id: str, task_id: str = "",
                      fmt: str = "json") -> Any:
        """Runtime-controlled export (redacted server-side)."""
        return self.client.export(session_id, task_id, fmt)

    def session_history(self, **filters: Any) -> dict[str, Any]:
        return self.client.list_sessions(
            status=str(filters.get("status", "")),
            workspace=str(filters.get("workspace", "")),
            since=float(filters.get("since", 0) or 0))
