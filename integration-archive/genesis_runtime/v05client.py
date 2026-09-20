"""Single import point for the v0.5 PUBLIC client (HTTP only).

The integration never imports runtime internals. Architecture tests
enforce this: only `client` (plus stdlib) may come from the runtime tree.
"""
from __future__ import annotations

import os
import sys
from pathlib import Path


def _runtime_dir() -> Path:
    configured = os.environ.get("AGENT_BRIDGE_V05_ROOT", "")
    if not configured:
        raise RuntimeError(
            "AGENT_BRIDGE_V05_ROOT is not set: point it at an "
            "agent_bridge_v05 checkout (machine paths are not hardcoded).")
    return Path(configured)


_RUNTIME_DIR = _runtime_dir()
if str(_RUNTIME_DIR) not in sys.path:
    sys.path.insert(0, str(_RUNTIME_DIR))

from client import AgentRuntimeClient, ClientError  # noqa: E402  public SDK

__all__ = ["AgentRuntimeClient", "ClientError", "RUNTIME_DIR"]
RUNTIME_DIR = _RUNTIME_DIR
