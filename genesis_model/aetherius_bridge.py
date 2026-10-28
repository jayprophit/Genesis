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
import os
import time
import urllib.request
from typing import Any

# Agent Bridge imports (consumer-owned, no service bootstrap).
# The bridge source root is configurable so this module is portable; the
# legacy local-only default preserves the verified workspace layout.
import sys as _sys

_AGENT_BRIDGE_ROOT = os.environ.get(
    "GENESIS_AGENT_BRIDGE_ROOT",
    "C:/Users/jpowe/Desktop/Projects/Agent-Bridge",
)
if _AGENT_BRIDGE_ROOT and _AGENT_BRIDGE_ROOT not in _sys.path:
    _sys.path.insert(0, _AGENT_BRIDGE_ROOT)


def _import_agent_bridge():
    from tools.registry import ToolRegistry as _ABRegistry  # noqa: E402
    from tools.phase4_adapter import ensure_phase4_registered  # noqa: E402
    from tools.router import ToolRouter  # noqa: E402
    return _ABRegistry, ensure_phase4_registered, ToolRouter


_ABRegistry, ensure_phase4_registered, ToolRouter = _import_agent_bridge()

# Genesis imports.
from .tools import ToolContext, ToolRegistry, ToolResult, ToolSpec  # noqa: E402
from .agent import Backend  # noqa: E402


# -- Ollama / external inference configuration --------------------------------
MODEL = os.environ.get("GENESIS_OLLAMA_MODEL", "qwen3.5:2b-q4_K_M")
OLLAMA = os.environ.get("GENESIS_OLLAMA_URL", "http://127.0.0.1:11434")
OLLAMA_TIMEOUT = int(os.environ.get("GENESIS_OLLAMA_TIMEOUT", "300"))
_OLLAMA_KEEPALIVE = os.environ.get("GENESIS_OLLAMA_KEEPALIVE", "")


def _chat(messages):
    options: dict[str, Any] = {"temperature": 0, "num_predict": 256}
    if _OLLAMA_KEEPALIVE:
        options["keep_alive"] = _OLLAMA_KEEPALIVE
    payload = {"model": MODEL, "messages": messages, "stream": False, "think": False,
               "options": options}
    req = urllib.request.Request(OLLAMA + "/api/chat", data=json.dumps(payload).encode(),
                                 headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=OLLAMA_TIMEOUT) as r:
        return json.loads(r.read())["message"]["content"]


def _format_tool_desc(tools):
    """Build the planner-facing tool list, surfacing each tool's argument
    names and types.

    Phase F root cause: previously only bare tool NAMES were surfaced
    (parameters discarded), so the planner hallucinated argument shapes
    (e.g. {"numbers": [...]}) and refused to call phase4 tools. With the
    canonical JSON-Schema argument surface exposed, the planner emits
    well-formed calls matching the bridge's expected args."""
    def _sig(t):
        nm = t.get("name", t.get("function", {}).get("name", "?"))
        params = (t.get("parameters") or t.get("function", {}).get("parameters")
                  or {}) or {}
        props = params.get("properties", {}) if isinstance(params, dict) else {}
        args = ", ".join(
            f"{k}: {v.get('type', 'any')}" if isinstance(v, dict) else k
            for k, v in props.items()
        )
        return f"{nm}({args})" if args else nm
    desc = ("Available tools: " + ", ".join(_sig(t) for t in tools) + ". "
            + 'To call a tool, reply with ONLY a fenced JSON block: ```json {"tool": "<name>", "arguments": {...}} ```.')
    desc += (' Example call for a two-argument tool: ```json {"tool": "phase4.add_numbers", "arguments": {"a": 47, "b": 58}} ```.')
    return desc


class AetheriusBackend:
    """Genesis Backend over the Aetherius local gateway (localhost only)."""

    def __init__(self, model=MODEL, temperature=0.0, max_new_tokens=256):
        self.model = model
        self.temperature = temperature
        self.max_new_tokens = max_new_tokens

    def generate(self, messages, tools=None):
        tool_desc = _format_tool_desc(tools) if tools else ""
        msgs = [{"role": "system", "content": tool_desc}] + [dict(m) for m in messages]
        return _chat(msgs)


class AgentBridgeToolExecutor:
    """Dispatches phase4.* tools through the real Agent Bridge dispatcher.

    Owns its own Agent Bridge registry (consumer boundary) and registers the
    four harmless phase4 tools via the idempotent ensure_phase4_registered.
    """

    def __init__(self, agent_id="genesis-test-agent"):
        self.agent_id = os.environ.get("GENESIS_AGENT_ID", agent_id)
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


def _to_json_schema(input_schema):
    """Convert an Agent-Bridge phase4 input_schema ({arg: type}) into a JSON
    Schema object for the LLM function-calling surface.

    Without the real argument names/types the planner cannot emit a well-formed
    tool call and refuses it (observed: the 2B model declined phase4.add_numbers
    and computed the sum manually). The declared schema is reconstructed here
    from the canonical Agent-Bridge spec instead of being left empty."""
    props = {}
    required = []
    for k, t in (input_schema or {}).items():
        props[k] = {"type": t}
        required.append(k)
    schema = {"type": "object", "properties": props}
    if required:
        schema["required"] = required
    return schema


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
        rec = executor.ab_reg.get(tid)
        genesis_registry.register(ToolSpec(
            name=tid,
            description=f"Agent Bridge tool: {tid} (READ_ONLY, authorised via Agent Bridge).",
            parameters=_to_json_schema(rec.input_schema),
            handler=_ab_handler_factory(executor, tid),
            risk="low",
        ))
    return genesis_registry


__all__ = ["AetheriusBackend", "AgentBridgeToolExecutor",
           "register_agent_bridge_tools"]
