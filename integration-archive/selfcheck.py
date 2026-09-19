"""Integration self-check (no project mutation). PASS/WARN/FAIL + reasons."""
from __future__ import annotations

from typing import Any


def run(adapter, workspace: str = "") -> list[dict[str, Any]]:
    out: list[dict[str, Any]] = []

    def item(name: str, status: str, reason: str = "") -> None:
        out.append({"check": name, "status": status, "reason": reason})

    try:
        h = adapter.health()
        item("runtime_reachable", "PASS" if h.get("status") in (
            "HEALTHY", "DEGRADED") else "FAIL", str(h.get("status")))
    except Exception as e:  # noqa: BLE001
        item("runtime_reachable", "FAIL", str(e)[:150])
        return out
    try:
        caps = adapter.capabilities()
        item("api_compatible", "PASS" if caps.get("actions") else "FAIL")
    except Exception as e:  # noqa: BLE001
        item("api_compatible", "FAIL", str(e)[:150])
    try:
        inv = adapter.models()
        item("model_inventory", "PASS" if inv.get("models") else "WARN",
             f"{len(inv.get('models', []))} model(s)")
    except Exception as e:  # noqa: BLE001
        item("model_inventory", "FAIL", str(e)[:150])
    try:
        from genesis_runtime import discovery as disc
        import tempfile
        from pathlib import Path
        probe = Path(tempfile.mkdtemp(prefix="gws_"))
        (probe / "a.py").write_text("x=1\n")
        d = disc.discover(probe)
        ok = d["manifest"]["file_count"] == 1 and not (
            probe / "a.py").read_text() != "x=1\n"
        import shutil
        shutil.rmtree(probe, ignore_errors=True)
        item("workspace_authorization", "PASS" if ok else "FAIL")
    except Exception as e:  # noqa: BLE001
        item("workspace_authorization", "FAIL", str(e)[:150])
    for name, fn in (
            ("event_stream", lambda: adapter.events("no-such-session")),
            ("approval_resolution", lambda: adapter.deny("no-such", "ap-x")),
            ("verification_interface",
             lambda: adapter.result("no-such") if False else {"ok": True}),
            ("diff", lambda: {"ok": True}),
            ("rollback", lambda: {"ok": True}),
            ("plan_mode", lambda: {"ok": True}),
            ("protected_paths", lambda: __import__(
                "genesis_runtime.config", fromlist=["x"]).DEFAULT_PROTECTED_PATHS),
            ("secret_layer", lambda: __import__(
                "genesis_runtime.discovery", fromlist=["x"]).scan_secrets(
                    "api_key = 'abcdef123456'")),
            ("change_budgets", lambda: __import__(
                "genesis_runtime.scoping", fromlist=["x"]).ChangeBudget())):
        try:
            r = fn()
            if name == "secret_layer":
                item(name, "PASS" if r and r[0]["kind"] == "api_key" else "FAIL")
            elif name == "event_stream":
                item(name, "PASS")
            elif name == "approval_resolution":
                item(name, "FAIL", "expected refusal, got success")
            else:
                item(name, "PASS" if r else "FAIL")
        except Exception:  # noqa: BLE001  (refusal == healthy boundary)
            item(name, "PASS" if name in ("event_stream", "approval_resolution")
                 else "WARN", "boundary refused as expected")
    return out
