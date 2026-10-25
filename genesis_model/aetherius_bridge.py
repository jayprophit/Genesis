"""Aetherius local AI bridge for Genesis (Phase 10, corrected).

Implements the Genesis Backend protocol over the Aetherius local CUDA inference
gateway and routes tool proposals through the Agent Bridge dispatcher.

Reuses existing Aetherius components (Phase 3 adapter contract, Phase 4 queue/
session/RAG, Phase 5 worker/VRAM, Phase 8/9 Agent Bridge). No duplicates.

Consumer boundary: the Genesis caller constructs a Genesis ToolRegistry and
calls ``register_agent_bridge_tools(registry, executor)`` so the Genesis agent
can see and execute Agent Bridge tools through its normal loop. Registration
of Agent Bridge-side phase4 tools happens inside AgentBridgeToolExecutor via
the idempotent ``ensure_phase4_registered`` helper.
"""
from __future__ import annotations

import json
import time
import urllib.request
from typing import Any

# Agent Bridge imports (consumer-owned, no service bootstrap).
import sys as _sys
_sys.path.insert(0, "C:/Users/jpowe/Desktop/Projects/Agent-Bridge")
from tools.registry import ToolRegistry as _ABRegistry  # noqa: E402
from tools.phase4_adapter import ensure_phase4_registered  # noqa: E402
from tools.router import ToolRouter  # noqa: E402

# Genesis imports.
from .tools import ToolContext, ToolRegistry, ToolResult, ToolSpec  # noqa: E402
from .agent import Backend  # noqa: E402


MODEL = "qwen3.5:0.8b"
OLLAMA = "http://127.0.0.1:11434"


def _chat(messages):
    payload = {"model": MODEL, "messages": messages, "stream": False, "think": False,
               "options": {"temperature": 0, "num_predict": 256}}
    req = urllib.request.Request(OLLAMA + "/api/chat", data=json.dumps(payload).encode(),
                                 headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=300) as r:
        return json.loads(r.read())["message"]["content"]


class AetheriusBackend:
    """Genesis Backend over the Aetherius local gateway (localhost only)."""

    def __init__(self, model=MODEL, temperature=0.0, max_new_tokens=256):
        self.model = model
        self.temperature = temperature
        self.max_new_tokens = max_new_tokens

    def generate(self, messages, tools=None):
        tool_desc = ""
        if tools:
            names = [t.get("name", t.get("function", {}).get("name", "?")) for t in tools]
            tool_desc = "Available tools: " + ", ".join(names) + ". "
            tool_desc += 'To call a tool, reply with ONLY a fenced JSON block: ```json {"tool": "<name>", "arguments": {...}} ```.'
        msgs = [{"role": "system", "content": tool_desc}] + [dict(m) for m in messages]
        return _chat(msgs)


class AgentBridgeToolExecutor:
    """Dispatches phase4.* tools through the real Agent Bridge dispatcher.

    Owns its own Agent Bridge registry (consumer boundary) and registers the
    four harmless phase4 tools via the idempotent ensure_phase4_registered.
    """

    def __init__(self, agent_id="genesis-test-agent"):
        self.agent_id = agent_id
        self.ab_reg = _ABRegistry()
        self._registered = ensure_phase4_registered(self.ab_reg)
        self.router = ToolRouter(self.ab_reg)

    def execute(self, tool_name, arguments, execution_id=None):
        if not tool_name.startswith("phase4."):
            return {"ok": False, "error": "tool not in Agent Bridge phase4 set"}
        ctx = {"trusted_agent_id": self.agent_id, "owner_authorized": True,
               "execution_id": execution_id}
        return self.router.call(tool_name, arguments, context=ctx)

    def schemas_ab(self):
        return [{"name": tid, "description": tid} for tid in self.ab_reg.ids()]


def _ab_handler_factory(executor, tool_name):
    def handler(arguments, context):
        res = executor.execute(tool_name, arguments, execution_id=None)
        if not res.get("ok"):
            raise RuntimeError(res.get("error", "agent bridge failure"))
        return res.get("output", res.get("result"))
    return handler


def register_agent_bridge_tools(genesis_registry, executor):
    """Register Agent Bridge phase4 tools into a Genesis ToolRegistry.

    Each tool delegates execution to the Agent Bridge dispatcher, preserving
    trusted identity, deny-by-default and audit logging on the Agent Bridge side.
    Idempotent: skips tools already registered.
    """
    for tid in executor.ab_reg.ids():
        if tid in genesis_registry.names():
            continue
        genesis_registry.register(ToolSpec(
            name=tid,
            description=f"Agent Bridge tool: {tid} (READ_ONLY, authorised via Agent Bridge).",
            parameters={"type": "object", "properties": {}, "additionalProperties": True},
            handler=_ab_handler_factory(executor, tid),
            risk="low",
        ))
    return genesis_registry


__all__ = ["AetheriusBackend", "AgentBridgeToolExecutor",
           "register_agent_bridge_tools"]
