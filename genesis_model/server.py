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
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any

from .agent import GenesisAgent, GenesisModelBackend
from .checkpoint import load_checkpoint
from .generate import generate_text
from .tools import default_registry


class GenesisRuntime:
    def __init__(
        self,
        checkpoint: str | Path,
        *,
        workspace: str | Path = ".",
        device: str = "cpu",
        allow_network: bool = False,
    ) -> None:
        self.model, self.tokenizer, self.meta = load_checkpoint(checkpoint, map_location=device)
        self.model.to(device)
        self.workspace = Path(workspace)
        self.registry = default_registry()
        self.allow_network = allow_network
        self.lock = threading.Lock()

    def chat(
        self,
        messages: list[dict],
        *,
        temperature: float = 0.7,
        max_tokens: int = 256,
        tools: bool = True,
    ) -> dict[str, Any]:
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

        def do_GET(self) -> None:  # noqa: N802
            if self.path == "/health":
                self._send(200, {
                    "status": "ok",
                    "model": "genesis-local",
                    "parameters": self.runtime.model.num_parameters(),
                    "vocab_size": self.runtime.tokenizer.vocab_size,
                    "tools": self.runtime.registry.names(),
                })
            elif self.path == "/v1/models":
                self._send(200, {"data": [{"id": "genesis-local", "object": "model", "owned_by": "genesis"}]})
            elif self.path == "/v1/tools":
                self._send(200, {"data": self.runtime.registry.schemas()})
            else:
                self._send(404, {"error": "not found"})

        def do_POST(self) -> None:  # noqa: N802
            try:
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
            except Exception as exc:
                self._send(400, {"error": f"{type(exc).__name__}: {exc}"})

    Handler.runtime = runtime
    return Handler


def serve(
    checkpoint: str | Path,
    *,
    host: str = "127.0.0.1",
    port: int = 8420,
    workspace: str | Path = ".",
    device: str = "cpu",
    allow_network: bool = False,
) -> None:
    runtime = GenesisRuntime(
        checkpoint, workspace=workspace, device=device, allow_network=allow_network
    )
    server = ThreadingHTTPServer((host, port), make_handler(runtime))
    print(f"Genesis runtime listening on http://{host}:{port}")
    server.serve_forever()
