"""Post-Gate-D security + regression tests for the hardened Genesis HTTP runtime.

These exercises target the local-development security boundary added in the
harden/post-gate-d work (origin allowlist, optional X-Genesis-Token auth,
sanitised errors, body limits, OPTIONS safety, agent_bridge_active health).
They use a stub runtime so they need no Ollama/torch and run deterministically.

Run:  pytest tests/test_genesis_security.py -p no:cacheprovider
"""
from __future__ import annotations

import http.client
import json
import threading
from http.server import ThreadingHTTPServer
from pathlib import Path

import pytest

import sys as _sys
_REPO_ROOT = str(Path(__file__).resolve().parents[1])
if _REPO_ROOT not in _sys.path:
    _sys.path.insert(0, _REPO_ROOT)

import genesis_model.server as srv  # noqa: E402
from genesis_model.tools import default_registry, ToolContext  # noqa: E402

ALLOWED = "http://127.0.0.1:5199"
EVIL = "http://evil.example"


class FakeRegistry:
    def names(self):
        return ["phase4.add_numbers", "calculator", "write_file", "http_get", "read_file"]

    def schemas(self):
        return [{"name": "phase4.add_numbers"}, {"name": "calculator"}]


class FakeRuntime:
    """Stand-in for GenesisRuntime: enough surface for the HTTP handler."""

    def __init__(self, answer: int = 105):
        self.model = None
        self.tokenizer = None
        self.agent_bridge_executor = True
        self.registry = FakeRegistry()
        self.answer = answer
        self.agent_called = 0
        self._lock = threading.Lock()

    def agent(self, task, *, max_steps=6, temperature=0.4):
        with self._lock:
            self.agent_called += 1
        return {
            "stopped_reason": "stop",
            "steps": 1,
            "tool_calls": [],
            "tool_results": [{"tool": "phase4.add_numbers", "ok": True, "output": self.answer}],
            "answer": str(self.answer),
        }

    def chat(self, messages, **kw):
        return {"choices": [{"message": {"content": str(self.answer)}}]}


def _start(runtime, token: str = "", max_body: int = 5_000_000):
    srv.API_TOKEN = token
    srv.MAX_BODY = max_body
    httpd = ThreadingHTTPServer(("127.0.0.1", 0), srv.make_handler(runtime))
    port = httpd.server_address[1]
    threading.Thread(target=httpd.serve_forever, daemon=True).start()
    return f"127.0.0.1:{port}", httpd


def _stop(httpd):
    httpd.shutdown()
    httpd.server_close()


def _req(base, method, path, headers=None, body=None):
    conn = http.client.HTTPConnection(base)
    headers = dict(headers or {})
    if body is not None:
        if isinstance(body, (dict, list)):
            body = json.dumps(body)
            headers.setdefault("Content-Type", "application/json")
        headers["Content-Length"] = str(len(body.encode("utf-8")))
    conn.request(method, path, body=body, headers=headers)
    resp = conn.getresponse()
    data = resp.read().decode("utf-8", "replace")
    hdrs = {k.lower(): v for k, v in resp.getheaders()}
    conn.close()
    return resp.status, hdrs, data


# --------------------------------------------------------------------------- #
# Phase A1 / A3 — HTTP security boundary
# --------------------------------------------------------------------------- #
def test_health_reports_agent_bridge_active_and_allows_known_origin():
    base, httpd = _start(FakeRuntime())
    try:
        s, h, d = _req(base, "GET", "/health", {"Origin": ALLOWED})
        assert s == 200
        p = json.loads(d)
        assert p["status"] == "ok"
        assert p["agent_bridge_active"] is True
        assert h["access-control-allow-origin"] == ALLOWED  # reflected, not wildcard
        assert "access-control-allow-credentials" in h
        assert h.get("vary") == "Origin"
    finally:
        _stop(httpd)


def test_disallowed_origin_gets_no_acao():
    base, httpd = _start(FakeRuntime())
    try:
        s, h, d = _req(base, "GET", "/health", {"Origin": EVIL})
        assert s == 200  # server answers; the browser is what cannot read it
        assert "access-control-allow-origin" not in h
        assert h.get("vary") == "Origin"
    finally:
        _stop(httpd)


def test_wildcard_acao_never_sent():
    base, httpd = _start(FakeRuntime())
    try:
        s, h, d = _req(base, "GET", "/health", {"Origin": EVIL})
        assert h.get("access-control-allow-origin") != "*"
    finally:
        _stop(httpd)


def test_options_never_executes_tools():
    rt = FakeRuntime()
    base, httpd = _start(rt)
    try:
        s, h, d = _req(base, "OPTIONS", "/v1/agent", {"Origin": ALLOWED})
        assert s == 204
        assert "access-control-allow-headers" in h
        assert "x-genesis-token" in h["access-control-allow-headers"].lower()
        assert d == ""
        assert rt.agent_called == 0  # preflight must not touch the runtime
    finally:
        _stop(httpd)


def test_post_requires_token_when_configured():
    rt = FakeRuntime()
    base, httpd = _start(rt, token="test-secret")
    try:
        # no token -> 401, agent not called
        s, _, _ = _req(base, "POST", "/v1/agent", {"Origin": ALLOWED}, {"task": "47+58"})
        assert s == 401 and rt.agent_called == 0
        # wrong token -> 401
        s, _, _ = _req(base, "POST", "/v1/agent", {"Origin": ALLOWED, "X-Genesis-Token": "nope"}, {"task": "47+58"})
        assert s == 401 and rt.agent_called == 0
        # correct token -> 200, 105
        s, _, d = _req(base, "POST", "/v1/agent", {"Origin": ALLOWED, "X-Genesis-Token": "test-secret"},
                       {"task": "Calculate 47 + 58 using Agent Bridge."})
        assert s == 200 and rt.agent_called == 1
        assert "105" in d
    finally:
        srv.API_TOKEN = ""
        _stop(httpd)


def test_token_off_preserves_working_integration():
    rt = FakeRuntime()
    base, httpd = _start(rt, token="")  # default: open on loopback
    try:
        s, _, d = _req(base, "POST", "/v1/agent", {"Origin": ALLOWED}, {"task": "47+58"})
        assert s == 200 and rt.agent_called == 1 and "105" in d
    finally:
        _stop(httpd)


def test_malformed_json_body_rejected():
    base, httpd = _start(FakeRuntime())
    try:
        conn = http.client.HTTPConnection(base)
        conn.request("POST", "/v1/agent", body=b"not json{",
                     headers={"Content-Type": "application/json", "Content-Length": "9", "Origin": ALLOWED})
        r = conn.getresponse(); d = r.read().decode(); conn.close()
        assert r.status == 400
        assert json.loads(d)["error"] == "invalid JSON body"
    finally:
        _stop(httpd)


def test_non_object_body_rejected():
    base, httpd = _start(FakeRuntime())
    try:
        s, _, d = _req(base, "POST", "/v1/agent", {"Origin": ALLOWED}, body="[1,2,3]")
        assert s == 400 and json.loads(d)["error"] == "request body must be a JSON object"
    finally:
        _stop(httpd)


def test_oversized_body_rejected_413():
    base, httpd = _start(FakeRuntime(), max_body=100)
    try:
        s, _, d = _req(base, "POST", "/v1/agent", {"Origin": ALLOWED}, body="x" * 200)
        assert s == 413 and json.loads(d)["error"] == "payload too large"
    finally:
        _stop(httpd)


def test_no_traceback_or_path_leak_on_runtime_error():
    class Boom(FakeRuntime):
        def agent(self, task, **kw):
            raise RuntimeError("secret: /etc/passwd leak")
    base, httpd = _start(Boom())
    try:
        s, _, d = _req(base, "POST", "/v1/agent", {"Origin": ALLOWED}, {"task": "x"})
        assert s == 500
        p = json.loads(d)
        assert p["error"] == "internal error"  # no traceback, no path
        assert "passwd" not in d and "RuntimeError" not in d
    finally:
        _stop(httpd)


# --------------------------------------------------------------------------- #
# Phase A2 — Agent Bridge identity: the HTTP API never trusts caller-supplied
# identity; authorization is made server-side by the trusted bridge executor.
# --------------------------------------------------------------------------- #
def test_client_cannot_grant_owner_authorized():
    rt = FakeRuntime()
    base, httpd = _start(rt)
    try:
        s, _, d = _req(base, "POST", "/v1/agent", {"Origin": ALLOWED},
                       {"task": "Calculate 47 + 58 using Agent Bridge.",
                        "owner_authorized": True, "trusted_agent_id": "attacker"})
        assert s == 200 and rt.agent_called == 1 and "105" in d
        # identity is server-side; the request body fields above were ignored
    finally:
        _stop(httpd)


# --------------------------------------------------------------------------- #
# Phase A2/A3 — Genesis registry: deny-by-default, validation, approvals
# --------------------------------------------------------------------------- #
def test_tool_validation_rejects_missing_required_argument():
    reg = default_registry()
    ctx = ToolContext(workspace=Path("."), allow_network=False)
    res = reg.call("read_file", {}, ctx)
    assert not res.ok and res.error_code == "invalid_arguments"


def test_write_file_requires_approval():
    reg = default_registry()
    ctx = ToolContext(workspace=Path("."), allow_network=False)  # approver=None
    res = reg.call("write_file", {"path": "x", "content": "y"}, ctx)
    assert not res.ok and res.error_code == "approval_required"


def test_http_get_denied_when_network_disabled():
    reg = default_registry()
    # approval gate must pass first (approver returns True); then the handler
    # enforces the network boundary -> network_denied. This exercises BOTH gates.
    ctx = ToolContext(workspace=Path("."), allow_network=False, approver=lambda c, s: True)
    res = reg.call("http_get", {"url": "http://127.0.0.1:1/x"}, ctx)
    assert not res.ok and res.error_code == "network_denied"


def test_calculator_returns_105():
    reg = default_registry()
    ctx = ToolContext(workspace=Path("."), allow_network=False,
                      approver=lambda call, spec: True)
    res = reg.call("calculator", {"expression": "47 + 58"}, ctx)
    assert res.ok and res.output["value"] == 105


# --------------------------------------------------------------------------- #
# Phase A2 / A3 — Agent Bridge phase4 tool identity, schema and execution.
# These run fully in-process (real Agent-Bridge dispatcher, no Ollama), proving
# the privileged identity is server-side, args are validated, and the tool
# executes to 105. The schema case closes the Post-Gate-D gap where the planner
# was given an empty parameter set and refused the call.
# --------------------------------------------------------------------------- #
def test_phase4_schema_helper_builds_named_args():
    from genesis_model.aetherius_bridge import _to_json_schema
    s = _to_json_schema({"a": "number", "b": "number"})
    assert s["type"] == "object"
    assert list(s["properties"].keys()) == ["a", "b"]
    assert s["properties"]["a"] == {"type": "number"}
    assert s["required"] == ["a", "b"]
    # Empty schema still yields a valid object schema, never null/missing.
    assert _to_json_schema({}) == {"type": "object", "properties": {}}


def test_phase4_add_numbers_executes_via_trusted_bridge():
    from genesis_model.aetherius_bridge import AgentBridgeToolExecutor
    # Identity is the server-side default; the HTTP layer cannot inject it.
    ex = AgentBridgeToolExecutor(agent_id="genesis-test-agent")
    res = ex.execute("phase4.add_numbers", {"a": 47, "b": 58})
    assert res["ok"] is True
    assert res["output"] == 105
    # Missing required argument -> rejected by the bridge adapter (deny-by-default).
    bad = ex.execute("phase4.add_numbers", {"a": 47})
    assert bad["ok"] is False


def test_phase4_tool_schema_exposed_with_real_args():
    from genesis_model.aetherius_bridge import register_agent_bridge_tools, AgentBridgeToolExecutor
    from genesis_model.tools import ToolRegistry as GenesisRegistry
    ex = AgentBridgeToolExecutor(agent_id="genesis-test-agent")
    greg = GenesisRegistry()
    register_agent_bridge_tools(greg, ex)
    # The genesis-facing spec must declare a and b so the planner can call it.
    spec = greg.get("phase4.add_numbers")
    assert set(spec.parameters.get("properties", {}).keys()) == {"a", "b"}


def test_planner_tool_descriptor_exposes_argument_names():
    """Phase F acceptance: the LLM planner must see each tool's argument names
    and types so it can emit a well-formed tool call (instead of hallucinating
    a wrong shape like {"numbers":[...]})."""
    from genesis_model.aetherius_bridge import _format_tool_desc
    desc = _format_tool_desc([
        {"name": "phase4.add_numbers",
         "parameters": {"type": "object",
                        "properties": {"a": {"type": "number"}, "b": {"type": "number"}},
                        "required": ["a", "b"]}},
        {"name": "now",
         "parameters": {"type": "object", "properties": {}, "additionalProperties": False}},
    ])
    assert "phase4.add_numbers(a: number, b: number)" in desc
    assert "now" in desc  # zero-arg tool stays listed without a bogus signature
