"""Single import point for the v0.5 PUBLIC client (HTTP only).

The integration never imports runtime internals. Architecture tests
enforce this: only `client` (plus stdlib) may come from the runtime tree.
"""
from __future__ import annotations

import sys
from pathlib import Path

_RUNTIME_DIR = Path(
    "C:/Users/jpowe/Desktop/OpenCode-Agent-Test/agent_bridge_v05")
if str(_RUNTIME_DIR) not in sys.path:
    sys.path.insert(0, str(_RUNTIME_DIR))

from client import AgentRuntimeClient, ClientError  # noqa: E402  public SDK

__all__ = ["AgentRuntimeClient", "ClientError", "RUNTIME_DIR"]
RUNTIME_DIR = _RUNTIME_DIR
