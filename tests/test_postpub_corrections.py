"""Post-publication correction tests — isolated branch postpub/genesis-corrections.

These back the proposed diffs in the post-Gate-D review. They are NOT committed
or pushed; they live only in the isolated worktree. Run with the project venv
(torch 2.14) and GENESIS_AGENT_BRIDGE_ROOT -> the Agent-Bridge `tools/` package.
The stub runtime keeps them from needing a live model/Ollama unless stated.

pytest tests/test_postpub_corrections.py -p no:cacheprovider -q
"""
from __future__ import annotations

import http.client
import json
import threading
import urllib.request
from http.server import ThreadingHTTPServer
from pathlib import Path
import sys as _sys

_REPO = str(Path(__file__).resolve().parents[1])
if _REPO not in _sys.path:
    _sys.path.insert(0, _REPO)

import genesis_model.server as srv          # noqa: E402
import genesis_model.aetherius_bridge as ab  # noqa: E402
from genesis_model.aetherius_bridge import _chat  # noqa: E402

ALLOWED = "http://127.0.0.1:5199"
EVIL = "http://evil.example"


class FakeRegistry:
    def names(self):
        return ["phase4.add_numbers"]

    def schemas(self):
        return [{"name": "phase4.add_numbers"}]


class FakeRuntime:
    """Stub runtime: enough of the HTTP seam to assert authz/processing behaviour."""
    agent_called = 0

    def __init__(self):
        self.model = None
        self.tokenizer = None
        self.agent_bridge_executor = True
        self.registry = FakeRegistry()

    def agent(self, task, *, max_steps=6, temperature=0.4):
        type(self).agent_called += 1
        return {
            "stopped_reason": "stop",
            "steps": 1,
            "tool_calls": [],
            "tool_results": [{"tool": "phase4.add_numbers", "ok": True, "output": 105}],
            "answer": "105",
        }

    def chat(self, messages, **kw):
        return {"choices": [{"message": {"content": "105"}}]}


def _start(runtime, token: str = ""):
    srv.API_TOKEN = token
    FakeRuntime.agent_called = 0  # reset class-level counter per server start
    httpd = ThreadingHTTPServer(("127.0.0.1", 0), srv.make_handler(runtime))
    port = httpd.server_address[1]
    threading.Thread(target=httpd.serve_forever, daemon=True).start()
    return f"127.0.0.1:{port}", httpd


def _stop(h):
    h.shutdown()
    h.server_close()


def _post(base, origin, body, headers=None):
    conn = http.client.HTTPConnection(base)
    h = {"Content-Type": "application/json"}
    if origin is not None:
        h["Origin"] = origin
    h.update(headers or {})
    b = json.dumps(body).encode()
    h["Content-Length"] = str(len(b))
    conn.request("POST", "/v1/agent", body=b, headers=h)
    r = conn.getresponse()
    d = r.read().decode()
    conn.close()
    return r.status, {k.lower(): v for k, v in r.getheaders()}, d


# ---------------------------------------------------------------------------
# P1 — Ollama: keep_alive must be a TOP-LEVEL /api/chat field, not under options.
# ---------------------------------------------------------------------------
def test_ollama_keep_alive_top_level_in_serialized_request(monkeypatch):
    """Inspect the ACTUAL serialized request body sent to Ollama /api/chat."""
    captured: dict = {}

    class _Resp:
        def __init__(self, body):
            self._b = json.dumps(body).encode()

        def read(self):
            return self._b

        def __enter__(self):
            return self

        def __exit__(self, *a):
            return False

    def fake_urlopen(req, timeout=None):
        captured["payload"] = json.loads(req.data.decode("utf-8"))
        captured["url"] = req.full_url
        return _Resp({"message": {"content": "ok"}})

    monkeypatch.setattr(urllib.request, "urlopen", fake_urlopen)
    monkeypatch.setattr(ab, "_OLLAMA_KEEPALIVE", "30m")
    _chat([{"role": "user", "content": "hi"}])

    p = captured["payload"]
    assert captured["url"].endswith("/api/chat")
    assert p.get("keep_alive") == "30m"            # top-level (Ollama contract)
    assert "keep_alive" not in p.get("options", {})  # NOT nested under options


# ---------------------------------------------------------------------------
# P0 — server-side CORS enforcement on POST (CORS response ==/!= server authz).
# ---------------------------------------------------------------------------
def test_disallowed_origin_post_is_rejected_tokenless():
    """A disallowed-origin POST must NOT execute the runtime on a tokenless server."""
    rt = FakeRuntime()
    base, h = _start(rt)
    try:
        s, hdrs, d = _post(base, EVIL, {"task": "Calculate 47 + 58 using Agent Bridge."})
        assert s in (403, 401), f"disallowed-origin POST must be rejected, got {s}: {d}"
        assert FakeRuntime.agent_called == 0, "server must not run the agent for a rejected origin"
        assert hdrs.get("access-control-allow-origin") != EVIL  # no reflected ACAO
    finally:
        srv.API_TOKEN = ""
        _stop(h)


def test_allowed_origin_post_tokenless_executes_105():
    """Allowlisted-origin POST (tokenless, loopback) still works (105)."""
    rt = FakeRuntime()
    base, h = _start(rt)
    try:
        s, _, d = _post(base, ALLOWED, {"task": "Calculate 47 + 58 using Agent Bridge."})
        assert s == 200 and FakeRuntime.agent_called == 1 and "105" in d
    finally:
        srv.API_TOKEN = ""
        _stop(h)


def test_non_browser_loopback_request_tokenless_still_works():
    """No Origin header (loopback script/curl) is unaffected by the Origin gate."""
    rt = FakeRuntime()
    base, h = _start(rt)
    try:
        s, _, d = _post(base, None, {"task": "Calculate 47 + 58 using Agent Bridge."})
        assert s == 200 and FakeRuntime.agent_called == 1 and "105" in d
    finally:
        srv.API_TOKEN = ""
        _stop(h)


# ---------------------------------------------------------------------------
# P1-tool — varied operands (NOT only the 47/58 planner/IDE example).
# ---------------------------------------------------------------------------
def test_phase4_add_numbers_varied_operands():
    from genesis_model.aetherius_bridge import AgentBridgeToolExecutor
    ex = AgentBridgeToolExecutor(agent_id="genesis-test-agent")
    cases = [
        ({"a": 47, "b": 58}, 105),
        ({"a": -5, "b": 3}, -2),
        ({"a": 1.5, "b": 2.5}, 4.0),
        ({"a": 100, "b": 200}, 300),
        ({"a": -7, "b": -8}, -15),
    ]
    for args, expected in cases:
        res = ex.execute("phase4.add_numbers", args)
        assert res["ok"] is True, res
        assert res["output"] == expected, (args, res)


def test_phase4_add_numbers_invalid_arguments_rejected():
    """Invalid operands (missing / wrong type) are rejected with ok:false on the
    real Agent-Bridge execute path (tool_results, not a model answer)."""
    from genesis_model.aetherius_bridge import AgentBridgeToolExecutor
    ex = AgentBridgeToolExecutor(agent_id="genesis-test-agent")
    res = ex.execute("phase4.add_numbers", {"a": 47})  # missing required b
    assert res["ok"] is False, res
    res2 = ex.execute("phase4.add_numbers", {"a": "forty-seven", "b": 58})  # wrong type
    assert res2["ok"] is False, res2


# ---------------------------------------------------------------------------
# P2 — no machine-specific path; env-only configuration.
# ---------------------------------------------------------------------------
def test_no_hardcoded_local_path_in_published_source():
    src = Path(ab.__file__).read_text()
    assert "C:/Users/" not in src and "Desktop/Projects" not in src


def test_agent_bridge_root_default_is_env_only():
    src = Path(ab.__file__).read_text()
    assert 'os.environ.get("GENESIS_AGENT_BRIDGE_ROOT", "")' in src
