"""Genesis native tool protocol.

This is the built-in tool layer. It does not require Agent-Bridge. Agent-Bridge
remains the external adapter for devices, browsers, IDEs and third-party
services; Genesis must still be able to reason and act with its own tools when
no bridge is present.

Invariants:
  * the model emits a PROPOSAL, never an authorisation;
  * the registry validates arguments before a handler runs;
  * path tools are confined to an explicit workspace;
  * risky tools require an approval decision;
  * every call returns a structured result and an audit record.
"""
from __future__ import annotations

import ast
import json
import math
import operator
import re
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Callable, Iterable, Sequence

TOOL_CALL_RE = re.compile(r"<\|tool_call\|>(.*?)<\|/tool_call\|>", re.DOTALL)
FENCED_JSON_RE = re.compile(r"```(?:json)?\s*(\{.*?\})\s*```", re.DOTALL)


class ToolError(Exception):
    def __init__(self, code: str, message: str) -> None:
        super().__init__(message)
        self.code = code
        self.message = message


@dataclass
class ToolCall:
    name: str
    arguments: dict[str, Any] = field(default_factory=dict)
    call_id: str = ""


@dataclass
class ToolResult:
    name: str
    ok: bool
    output: Any = None
    error: str = ""
    error_code: str = ""
    duration_ms: int = 0

    def to_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "ok": self.ok,
            "output": self.output,
            "error": self.error,
            "error_code": self.error_code,
            "duration_ms": self.duration_ms,
        }


@dataclass
class ToolContext:
    workspace: Path
    allow_network: bool = False
    max_bytes: int = 2_000_000
    approver: Callable[[ToolCall, "ToolSpec"], bool] | None = None

    def __post_init__(self) -> None:
        self.workspace = Path(self.workspace).resolve()

    def resolve(self, relative: str) -> Path:
        candidate = (self.workspace / relative).resolve()
        try:
            candidate.relative_to(self.workspace)
        except ValueError as exc:
            raise ToolError("path_outside_workspace", f"{relative!r} escapes the workspace") from exc
        return candidate


Handler = Callable[[dict[str, Any], ToolContext], Any]


@dataclass
class ToolSpec:
    name: str
    description: str
    parameters: dict[str, Any]
    handler: Handler
    risk: str = "low"  # low | medium | high
    requires_approval: bool = False
    enabled: bool = True

    def schema(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "description": self.description,
            "parameters": self.parameters,
            "risk": self.risk,
        }


class ToolRegistry:
    def __init__(self, tools: Iterable[ToolSpec] | None = None) -> None:
        self._tools: dict[str, ToolSpec] = {}
        for tool in tools or []:
            self.register(tool)

    def register(self, tool: ToolSpec) -> None:
        if tool.name in self._tools:
            raise ValueError(f"tool already registered: {tool.name}")
        self._tools[tool.name] = tool

    def get(self, name: str) -> ToolSpec:
        try:
            return self._tools[name]
        except KeyError as exc:
            raise ToolError("unknown_tool", f"no such tool: {name}") from exc

    def names(self) -> list[str]:
        return sorted(name for name, tool in self._tools.items() if tool.enabled)

    def schemas(self) -> list[dict[str, Any]]:
        return [tool.schema() for tool in self._tools.values() if tool.enabled]

    def call(self, name: str, arguments: dict[str, Any], context: ToolContext) -> ToolResult:
        started = time.perf_counter()
        try:
            tool = self.get(name)
            if not tool.enabled:
                raise ToolError("tool_disabled", f"tool is disabled: {name}")
            _validate_arguments(tool.parameters, arguments)
            call = ToolCall(name=name, arguments=arguments)
            if tool.requires_approval or tool.risk == "high":
                if context.approver is None or not context.approver(call, tool):
                    raise ToolError("approval_required", f"approval refused for {name}")
            output = tool.handler(arguments, context)
            return ToolResult(
                name=name, ok=True, output=output,
                duration_ms=int((time.perf_counter() - started) * 1000),
            )
        except ToolError as exc:
            return ToolResult(
                name=name, ok=False, error=exc.message, error_code=exc.code,
                duration_ms=int((time.perf_counter() - started) * 1000),
            )
        except Exception as exc:  # handler bug or unexpected value
            return ToolResult(
                name=name, ok=False, error=f"{type(exc).__name__}: {exc}",
                error_code="tool_failed",
                duration_ms=int((time.perf_counter() - started) * 1000),
            )


# -- schema validation -------------------------------------------------------

_JSON_TYPES = {
    "string": str,
    "integer": int,
    "number": (int, float),
    "boolean": bool,
    "array": list,
    "object": dict,
}


def _validate_arguments(schema: dict[str, Any], arguments: dict[str, Any]) -> None:
    if not isinstance(arguments, dict):
        raise ToolError("invalid_arguments", "arguments must be a JSON object")
    properties = schema.get("properties", {})
    required = set(schema.get("required", []))
    missing = required - set(arguments)
    if missing:
        raise ToolError("invalid_arguments", f"missing required: {sorted(missing)}")
    if schema.get("additionalProperties") is False:
        extra = set(arguments) - set(properties)
        if extra:
            raise ToolError("invalid_arguments", f"unexpected: {sorted(extra)}")
    for key, value in arguments.items():
        spec = properties.get(key)
        if not spec:
            continue
        expected = spec.get("type")
        if expected in _JSON_TYPES and not isinstance(value, _JSON_TYPES[expected]):
            raise ToolError("invalid_arguments", f"{key} must be {expected}")
        if expected == "integer" and isinstance(value, bool):
            raise ToolError("invalid_arguments", f"{key} must be integer, not boolean")
        if "enum" in spec and value not in spec["enum"]:
            raise ToolError("invalid_arguments", f"{key} must be one of {spec['enum']}")
        if expected == "array" and "items" in spec:
            item_type = spec["items"].get("type")
            if item_type in _JSON_TYPES:
                for item in value:
                    if not isinstance(item, _JSON_TYPES[item_type]):
                        raise ToolError("invalid_arguments", f"{key} items must be {item_type}")


# -- built-in handlers -------------------------------------------------------

_BIN_OPS = {
    ast.Add: operator.add,
    ast.Sub: operator.sub,
    ast.Mult: operator.mul,
    ast.Div: operator.truediv,
    ast.FloorDiv: operator.floordiv,
    ast.Mod: operator.mod,
    ast.Pow: operator.pow,
}
_UNARY_OPS = {ast.UAdd: operator.pos, ast.USub: operator.neg}


def _safe_eval(node: ast.AST) -> float:
    if isinstance(node, ast.Expression):
        return _safe_eval(node.body)
    if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
        return node.value
    if isinstance(node, ast.BinOp) and type(node.op) in _BIN_OPS:
        left = _safe_eval(node.left)
        right = _safe_eval(node.right)
        if isinstance(node.op, ast.Pow) and abs(right) > 12:
            raise ToolError("unsafe_expression", "exponent too large")
        return _BIN_OPS[type(node.op)](left, right)
    if isinstance(node, ast.UnaryOp) and type(node.op) in _UNARY_OPS:
        return _UNARY_OPS[type(node.op)](_safe_eval(node.operand))
    raise ToolError("unsafe_expression", f"unsupported expression: {type(node).__name__}")


def calculator(arguments: dict[str, Any], context: ToolContext) -> dict[str, Any]:
    expression = str(arguments["expression"])
    if len(expression) > 200:
        raise ToolError("unsafe_expression", "expression too long")
    value = _safe_eval(ast.parse(expression, mode="eval"))
    if isinstance(value, float) and not math.isfinite(value):
        raise ToolError("invalid_result", "non-finite result")
    return {"value": value}


def list_dir(arguments: dict[str, Any], context: ToolContext) -> dict[str, Any]:
    path = context.resolve(str(arguments.get("path", ".")))
    if not path.is_dir():
        raise ToolError("not_a_directory", f"not a directory: {path.name}")
    entries = []
    for child in sorted(path.iterdir(), key=lambda p: (p.is_file(), p.name.lower()))[:200]:
        entries.append({
            "name": child.name,
            "type": "file" if child.is_file() else "directory",
            "size": child.stat().st_size if child.is_file() else None,
        })
    return {"path": str(path.relative_to(context.workspace)), "entries": entries}


def read_file(arguments: dict[str, Any], context: ToolContext) -> dict[str, Any]:
    path = context.resolve(str(arguments["path"]))
    if not path.is_file():
        raise ToolError("not_a_file", f"not a file: {path.name}")
    if path.stat().st_size > context.max_bytes:
        raise ToolError("file_too_large", f"file exceeds {context.max_bytes} bytes")
    return {"path": str(path.relative_to(context.workspace)),
            "content": path.read_text(encoding="utf-8", errors="replace")}


def write_file(arguments: dict[str, Any], context: ToolContext) -> dict[str, Any]:
    path = context.resolve(str(arguments["path"]))
    content = str(arguments["content"])
    if len(content.encode("utf-8")) > context.max_bytes:
        raise ToolError("file_too_large", "content exceeds limit")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    return {"path": str(path.relative_to(context.workspace)), "bytes": len(content.encode("utf-8"))}


def search_files(arguments: dict[str, Any], context: ToolContext) -> dict[str, Any]:
    import fnmatch

    pattern = str(arguments.get("pattern", "*"))
    root = context.resolve(str(arguments.get("path", ".")))
    limit = int(arguments.get("limit", 50))
    matches = []
    for path in root.rglob("*"):
        if path.is_file() and fnmatch.fnmatch(path.name, pattern):
            matches.append(str(path.relative_to(context.workspace)))
            if len(matches) >= limit:
                break
    return {"matches": matches}


def now(arguments: dict[str, Any], context: ToolContext) -> dict[str, Any]:
    return {"unix": time.time(), "utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())}


def http_get(arguments: dict[str, Any], context: ToolContext) -> dict[str, Any]:
    if not context.allow_network:
        raise ToolError("network_denied", "network is not enabled for this context")
    import urllib.request

    url = str(arguments["url"])
    if not url.startswith(("https://", "http://")):
        raise ToolError("invalid_url", "url must be http(s)")
    request = urllib.request.Request(url, headers={"User-Agent": "Genesis/0.1"})
    with urllib.request.urlopen(request, timeout=10) as response:  # noqa: S310
        body = response.read(min(context.max_bytes, 500_000))
    return {"status": response.status, "body": body.decode("utf-8", errors="replace")[:500_000]}


def default_registry() -> ToolRegistry:
    registry = ToolRegistry()
    registry.register(ToolSpec(
        name="calculator",
        description="Evaluate a basic arithmetic expression. Use instead of mental arithmetic.",
        parameters={
            "type": "object",
            "properties": {"expression": {"type": "string", "description": "Arithmetic expression"}},
            "required": ["expression"],
            "additionalProperties": False,
        },
        handler=calculator,
    ))
    registry.register(ToolSpec(
        name="list_dir",
        description="List files and directories inside the workspace.",
        parameters={
            "type": "object",
            "properties": {"path": {"type": "string"}},
            "additionalProperties": False,
        },
        handler=list_dir,
    ))
    registry.register(ToolSpec(
        name="read_file",
        description="Read a UTF-8 text file inside the workspace.",
        parameters={
            "type": "object",
            "properties": {"path": {"type": "string"}},
            "required": ["path"],
            "additionalProperties": False,
        },
        handler=read_file,
    ))
    registry.register(ToolSpec(
        name="write_file",
        description="Write a UTF-8 text file inside the workspace. Requires approval.",
        parameters={
            "type": "object",
            "properties": {"path": {"type": "string"}, "content": {"type": "string"}},
            "required": ["path", "content"],
            "additionalProperties": False,
        },
        handler=write_file,
        risk="high",
        requires_approval=True,
    ))
    registry.register(ToolSpec(
        name="search_files",
        description="Find files by glob pattern inside the workspace.",
        parameters={
            "type": "object",
            "properties": {"pattern": {"type": "string"}, "path": {"type": "string"}},
            "required": ["pattern"],
            "additionalProperties": False,
        },
        handler=search_files,
    ))
    registry.register(ToolSpec(
        name="now",
        description="Return the current UTC time.",
        parameters={"type": "object", "properties": {}, "additionalProperties": False},
        handler=now,
    ))
    registry.register(ToolSpec(
        name="http_get",
        description="Fetch a URL. Disabled unless the context explicitly enables network.",
        parameters={
            "type": "object",
            "properties": {"url": {"type": "string"}},
            "required": ["url"],
            "additionalProperties": False,
        },
        handler=http_get,
        risk="high",
        requires_approval=True,
    ))
    return registry


# -- model output parsing ----------------------------------------------------

def parse_tool_calls(text: str) -> list[ToolCall]:
    """Extract tool proposals from model output.

    Accepts the native control-token form and a fenced JSON fallback (useful for
    small models still learning the exact format). Malformed blocks are ignored
    and surfaced by the caller as a format error.
    """
    calls: list[ToolCall] = []
    for match in TOOL_CALL_RE.finditer(text):
        call = _parse_call_json(match.group(1))
        if call:
            calls.append(call)
    if not calls:
        for match in FENCED_JSON_RE.finditer(text):
            call = _parse_call_json(match.group(1))
            if call:
                calls.append(call)
    return calls


def _parse_call_json(raw: str) -> ToolCall | None:
    raw = raw.strip()
    if not raw:
        return None
    try:
        data = json.loads(raw)
    except json.JSONDecodeError:
        return None
    if not isinstance(data, dict):
        return None
    name = data.get("name") or data.get("tool")
    arguments = data.get("arguments", data.get("args", {}))
    if not isinstance(name, str) or not isinstance(arguments, dict):
        return None
    return ToolCall(name=name, arguments=arguments)


def render_tool_result(result: ToolResult) -> str:
    return "<|tool_result|>" + json.dumps(result.to_dict(), ensure_ascii=False) + "<|/tool_result|>"
