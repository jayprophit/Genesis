"""Local Genesis HTTP runtime.

Standard-library server with no framework dependency:
  GET  /health
  GET  /v1/models
  GET  /v1/tools
  POST /v1/chat/completions   (OpenAI-compatible shape, non-streaming)
  POST /v1/agent              (native agent loop with tools)

This is the integration seam for the IDE, Agent-Bridge and future desktop
clients. It is a local research API, not a production multi-tenant server.
"""
from __future__ import annotations

import json
import os
import secrets
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any

from .agent import GenesisAgent, GenesisModelBackend
from .aetherius_bridge import AetheriusBackend, AgentBridgeToolExecutor, register_agent_bridge_tools
from .checkpoint import load_checkpoint
from .generate import generate_text
from .tools import default_registry


# -- Local configuration (env, consistent with serve()) ----------------------
def _allowed_origins() -> list[str]:
    """Browser origins permitted to read responses. Empty env -> loopback dev set."""
    raw = os.environ.get("GENESIS_ALLOWED_ORIGINS", "")
    if not raw:
        return ["http://127.0.0.1:5199", "http://localhost:5199"]
    return [o.strip() for o in raw.split(",") if o.strip()]


def _origin_allowed(origin: str | None) -> bool:
    """True only when an Origin header is present and on the allowlist."""
    if not origin:
        return False
    return origin in _allowed_origins()


# Shared secret for state-changing (POST) requests. Empty = open on loopback
# (preserves the verified integration); set to require X-Genesis-Token.
API_TOKEN = os.environ.get("GENESIS_API_TOKEN", "")
MAX_BODY = int(os.environ.get("GENESIS_MAX_BODY_BYTES", "5000000"))


class _PayloadError(Exception):
    """Malformed or over-sized request body; carries its HTTP status code."""

    def __init__(self, message: str, status: int = 400) -> None:
        super().__init__(message)
        self.message = message
        self.status = status


class GenesisRuntime:
    def __init__(
        self,
        checkpoint: str | Path | None = None,
        *,
        workspace: str | Path = ".",
        device: str = "cpu",
        allow_network: bool = False,
        backend=None,
        agent_bridge: bool = False,
    ) -> None:
        self.workspace = Path(workspace)
        self.registry = default_registry()
        self.allow_network = allow_network
        self.lock = threading.Lock()
        self.backend_override = backend
        self.agent_bridge_executor = None
        if agent_bridge or isinstance(backend, AetheriusBackend):
            self.agent_bridge_executor = AgentBridgeToolExecutor()
            register_agent_bridge_tools(self.registry, self.agent_bridge_executor)
            if self.backend_override is None:
                self.backend_override = AetheriusBackend()
        if self.backend_override is None:
            if checkpoint is None:
                raise ValueError("checkpoint is required unless an Aetherius backend is used")
            self.model, self.tokenizer, self.meta = load_checkpoint(checkpoint, map_location=device)
            self.model.to(device)
        else:
            self.model = None
            self.tokenizer = None
            self.meta = None

    def chat(
        self,
        messages: list[dict],
        *,
        temperature: float = 0.7,
        max_tokens: int = 256,
        tools: bool = True,
    ) -> dict[str, Any]:
        if self.backend_override is not None:
            backend = self.backend_override
        else:
            backend = GenesisModelBackend(
                self.model, self.tokenizer, temperature=temperature, max_new_tokens=max_tokens
            )
        with self.lock:
            text = backend.generate(messages, self.registry.schemas() if tools else [])
        return {
            "id": f"chatcmpl-{int(time.time() * 1000)}",
            "object": "chat.completion",
            "created": int(time.time()),
            "model": "genesis-local",
            "choices": [{
                "index": 0,
                "message": {"role": "assistant", "content": text},
                "finish_reason": "stop",
            }],
            "usage": {"prompt_tokens": 0, "completion_tokens": 0, "total_tokens": 0},
        }

    def agent(self, task: str, *, max_steps: int = 6, temperature: float = 0.4) -> dict[str, Any]:
        if self.backend_override is not None:
            backend = self.backend_override
        else:
            backend = GenesisModelBackend(
                self.model, self.tokenizer, temperature=temperature, max_new_tokens=256
            )
        agent = GenesisAgent(
            backend, self.registry, workspace=self.workspace,
            max_steps=max_steps, allow_network=self.allow_network,
        )
        with self.lock:
            return agent.run(task).to_dict()


def make_handler(runtime: GenesisRuntime):
    class Handler(BaseHTTPRequestHandler):
        server_version = "GenesisRuntime/0.1"

        def log_message(self, format: str, *args) -> None:  # quieter logs
            return

        def _cors_headers(self) -> dict[str, str]:
            """Reflect ACAO only for allowlisted origins; always send Vary."""
            origin = self.headers.get("Origin")
            headers: dict[str, str] = {"Vary": "Origin"}
            if _origin_allowed(origin):
                headers["Access-Control-Allow-Origin"] = origin or ""
                headers["Access-Control-Allow-Credentials"] = "true"
            return headers

        def _send(self, status: int, payload: dict) -> None:
            body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
            self.send_response(status)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            for k, v in self._cors_headers().items():
                self.send_header(k, v)
            self.end_headers()
            self.wfile.write(body)

        def _body(self) -> dict:
            length = int(self.headers.get("Content-Length", "0"))
            if length > MAX_BODY:
                raise _PayloadError("payload too large", status=413)
            raw = self.rfile.read(length) if length else b"{}"
            try:
                data = json.loads(raw.decode("utf-8"))
            except (json.JSONDecodeError, UnicodeDecodeError):
                raise _PayloadError("invalid JSON body", status=400)
            if not isinstance(data, dict):
                raise _PayloadError("request body must be a JSON object", status=400)
            return data

        def _check_token(self) -> bool:
            """POSTs require the shared secret when GENESIS_API_TOKEN is set.

            Off by default so the verified integration is unaffected. The
            secret is never embedded in browser JS or committed files: injected
            via env and, in the local IDE, supplied by the dev harness through
            window.__GENESIS_API_TOKEN__ (consumed by the IDE adapter only).
            """
            if not API_TOKEN:
                return True
            supplied = self.headers.get("X-Genesis-Token", "")
            return bool(supplied) and secrets.compare_digest(supplied, API_TOKEN)

        def do_OPTIONS(self) -> None:  # noqa: N802
            # Preflight only: never executes tools, never touches the runtime.
            self.send_response(204)
            for k, v in self._cors_headers().items():
                self.send_header(k, v)
            self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
            self.send_header("Access-Control-Allow-Headers", "Content-Type, X-Genesis-Token")
            self.send_header("Content-Length", "0")
            self.end_headers()

        def do_GET(self) -> None:  # noqa: N802
            if self.path == "/health":
                self._send(200, {
                    "status": "ok",
                    "model": "genesis-local",
                    "parameters": (self.runtime.model.num_parameters() if self.runtime.model is not None else 0),
                    "vocab_size": (self.runtime.tokenizer.vocab_size if self.runtime.tokenizer is not None else 0),
                    "tools": self.runtime.registry.names(),
                    "agent_bridge_active": self.runtime.agent_bridge_executor is not None,
                })
            elif self.path == "/v1/models":
                self._send(200, {"data": [{"id": "genesis-local", "object": "model", "owned_by": "genesis"}]})
            elif self.path == "/v1/tools":
                self._send(200, {"data": self.runtime.registry.schemas()})
            else:
                self._send(404, {"error": "not found"})

        def do_POST(self) -> None:  # noqa: N802
            try:
                if not self._check_token():
                    self._send(401, {"error": "unauthorized"})
                    return
                # Server-side CORS enforcement on state-changing POSTs: reflecting
                # ACAO in the response does NOT stop a browser from SENDING a
                # disallowed-origin request with side effects (the page simply
                # can't read the response). Enforce the origin allowlist on POSTs
                # here, in addition to the reflected headers. Requests with no
                # Origin (loopback scripts/curl) are unaffected; the server is
                # bound to 127.0.0.1 only (GENESIS_ALLOWED_ORIGINS gates reads).
                origin = self.headers.get("Origin")
                if origin is not None and not _origin_allowed(origin):
                    self._send(403, {"error": "origin not allowed"})
                    return
                body = self._body()
                if self.path == "/v1/chat/completions":
                    result = self.runtime.chat(
                        body.get("messages", []),
                        temperature=float(body.get("temperature", 0.7)),
                        max_tokens=int(body.get("max_tokens", 256)),
                        tools=bool(body.get("tools", True)),
                    )
                elif self.path == "/v1/agent":
                    result = self.runtime.agent(
                        str(body.get("task", "")),
                        max_steps=int(body.get("max_steps", 6)),
                    )
                else:
                    self._send(404, {"error": "not found"})
                    return
                self._send(200, result)
            except _PayloadError as exc:
                # Body-level client errors: 413 (too large) or 400 (bad JSON/shape).
                self._send(exc.status, {"error": exc.message})
            except Exception:
                # Never leak tracebacks or local filesystem details to clients.
                self._send(500, {"error": "internal error"})

    Handler.runtime = runtime
    return Handler


def serve(
    checkpoint: str | Path | None = None,
    *,
    host: str = "127.0.0.1",
    port: int = 8420,
    workspace: str | Path = ".",
    device: str = "cpu",
    allow_network: bool = False,
    backend=None,
    agent_bridge: bool = False,
) -> None:
    if agent_bridge or os.environ.get("GENESIS_AGENT_BRIDGE") == "1":
        agent_bridge = True
    if checkpoint is None:
        checkpoint = os.environ.get("GENESIS_CHECKPOINT")
    runtime = GenesisRuntime(
        checkpoint, workspace=workspace, device=device, allow_network=allow_network,
        backend=backend, agent_bridge=agent_bridge,
    )
    server = ThreadingHTTPServer((host, port), make_handler(runtime))
    print(f"Genesis runtime listening on http://{host}:{port}")
    if API_TOKEN:
        print("Authentication enabled (X-Genesis-Token).")
    server.serve_forever()
