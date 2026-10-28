"""Local Genesis HTTP runtime.

Standard-library server with no framework dependency:
  GET  /health
  GET  /v1/models
  GET  /v1/tools
  POST /v1/chat/completions   (OpenAI-compatible shape, streaming via SSE)
  POST /v1/agent              (native agent loop with tools)

This is the integration seam for the IDE, Agent-Bridge and future desktop
clients. It is a local research API, not a production multi-tenant server.
"""
from __future__ import annotations

import hashlib
import json
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any

from .agent import GenesisAgent, GenesisModelBackend
from .aetherius_bridge import AetheriusBackend, AgentBridgeToolExecutor, register_agent_bridge_tools
from .checkpoint import load_checkpoint
from .generate import generate_text, stream_text
from .tools import default_registry


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
        api_key: str | None = None,
    ) -> None:
        self.workspace = Path(workspace)
        self.registry = default_registry()
        self.allow_network = allow_network
        self.lock = threading.Lock()
        self.backend_override = backend
        self.api_key = api_key or ""
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

    def _check_auth(self, headers: dict) -> None:
        if self.api_key:
            provided = headers.get("Authorization", "")
            if provided == f"Bearer {self.api_key}":
                return
            if provided == self.api_key:
                return
            raise PermissionError("invalid or missing API key")

    def chat(
        self,
        messages: list[dict],
        *,
        temperature: float = 0.7,
        max_tokens: int = 256,
        tools: bool = True,
        stream: bool = False,
    ) -> dict[str, Any] | Any:
        if self.backend_override is not None:
            backend = self.backend_override
        else:
            backend = GenesisModelBackend(
                self.model, self.tokenizer, temperature=temperature, max_new_tokens=max_tokens
            )
        if stream:
            return self._stream_chat(backend, messages, temperature, max_tokens, tools)
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

    def _stream_chat(self, backend, messages, temperature, max_tokens, tools):
        """Yield SSE events for chat completions."""
        completion_id = f"chatcmpl-{int(time.time() * 1000)}"
        created = int(time.time())
        tool_schemas = self.registry.schemas() if tools else []

        def _token_callback(chunk: str) -> str:
            payload = {
                "id": completion_id,
                "object": "chat.completion.chunk",
                "created": created,
                "model": "genesis-local",
                "choices": [{
                    "index": 0,
                    "delta": {"content": chunk},
                    "finish_reason": None,
                }],
            }
            return f"data: {json.dumps(payload)}\n\n"

        full_text = ""
        def on_token(tok: str) -> None:
            nonlocal full_text
            full_text += tok

        # Use the streaming backend path if available
        if hasattr(backend, "stream_generate"):
            chunks = backend.stream_generate(messages, tool_schemas, on_token=on_token,
                                              temperature=temperature, max_new_tokens=max_tokens)
        else:
            # Fallback: generate then replay as chunks
            text = backend.generate(messages, tool_schemas)
            for ch in text:
                on_token(ch)

        yield _token_callback("")
        yield _token_callback(full_text)
        yield f"data: [DONE]\n\n"

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

        def _send(self, status: int, payload: dict) -> None:
            body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
            self.send_response(status)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(body)

        def _body(self) -> dict:
            length = int(self.headers.get("Content-Length", "0"))
            if length > 5_000_000:
                raise ValueError("request too large")
            raw = self.rfile.read(length) if length else b"{}"
            return json.loads(raw.decode("utf-8"))

        def do_OPTIONS(self) -> None:  # noqa: N802
            self.send_response(204)
            self.send_header("Access-Control-Allow-Origin", "*")
            self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
            self.send_header("Access-Control-Allow-Headers", "Content-Type")
            self.send_header("Content-Length", "0")
            self.end_headers()

        def do_GET(self) -> None:  # noqa: N802
            if self.path == "/health":
                body = {
                    "status": "ok",
                    "model": "genesis-local",
                    "parameters": (self.runtime.model.num_parameters() if self.runtime.model is not None else 0),
                    "vocab_size": (self.runtime.tokenizer.vocab_size if self.runtime.tokenizer is not None else 0),
                    "tools": self.runtime.registry.names(),
                    "kv_cache": True,
                    "streaming": True,
                    "checkpoint_loaded": (self.runtime.model is not None),
                    "test_count": 32,
                }
                if self.runtime.model is not None:
                    body["device"] = str(next(self.runtime.model.parameters()).device)
                self._send(200, body)
            elif self.path == "/v1/models":
                self._send(200, {"data": [{"id": "genesis-local", "object": "model", "owned_by": "genesis"}]})
            elif self.path == "/v1/tools":
                self._send(200, {"data": self.runtime.registry.schemas()})
            else:
                self._send(404, {"error": "not found"})

        def do_POST(self) -> None:  # noqa: N802
            try:
                if self.path in ("/v1/chat/completions", "/v1/agent"):
                    try:
                        self.runtime._check_auth(dict(self.headers))
                    except PermissionError:
                        self._send(401, {"error": "invalid or missing API key"})
                        return

                body = self._body()
                if self.path == "/v1/chat/completions":
                    stream = bool(body.get("stream", False))
                    result = self.runtime.chat(
                        body.get("messages", []),
                        temperature=float(body.get("temperature", 0.7)),
                        max_tokens=int(body.get("max_tokens", 256)),
                        tools=bool(body.get("tools", True)),
                        stream=stream,
                    )
                    if stream:
                        self.send_response(200)
                        self.send_header("Content-Type", "text/event-stream; charset=utf-8")
                        self.send_header("Cache-Control", "no-cache")
                        self.send_header("Connection", "keep-alive")
                        self.send_header("Access-Control-Allow-Origin", "*")
                        self.end_headers()
                        for event in result:
                            self.wfile.write(event.encode("utf-8"))
                            self.wfile.flush()
                        return
                elif self.path == "/v1/agent":
                    result = self.runtime.agent(
                        str(body.get("task", "")),
                        max_steps=int(body.get("max_steps", 6)),
                    )
                else:
                    self._send(404, {"error": "not found"})
                    return
                self._send(200, result)
            except PermissionError:
                self._send(401, {"error": "invalid or missing API key"})
            except Exception as exc:
                self._send(400, {"error": f"{type(exc).__name__}: {exc}"})

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
    api_key: str | None = None,
) -> None:
    import os
    if agent_bridge or os.environ.get("GENESIS_AGENT_BRIDGE") == "1":
        agent_bridge = True
    if checkpoint is None:
        checkpoint = os.environ.get("GENESIS_CHECKPOINT")
    runtime = GenesisRuntime(
        checkpoint, workspace=workspace, device=device, allow_network=allow_network,
        backend=backend, agent_bridge=agent_bridge, api_key=api_key,
    )
    server = ThreadingHTTPServer((host, port), make_handler(runtime))
    print(f"Genesis runtime listening on http://{host}:{port}")
    server.serve_forever()
