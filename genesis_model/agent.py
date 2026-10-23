"""Genesis native agent loop.

The loop is deliberately provider-neutral: a Backend only has to turn messages
into text. GenesisModelBackend is the first-party implementation; scripted
backends exist so the control flow, tool policy and audit trail can be tested
without a trained model.

Flow:
    goal -> model -> (tool proposal -> policy -> execute -> observation)* -> answer
"""
from __future__ import annotations

import json
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Protocol, Sequence

import torch

from .model import GenesisModel
from .tokenizer import ASSISTANT, EOS, Tokenizer
from .tools import (
    ToolCall,
    ToolContext,
    ToolRegistry,
    ToolResult,
    parse_tool_calls,
    render_tool_result,
)


class Backend(Protocol):
    def generate(self, messages: Sequence[dict], tools: Sequence[dict]) -> str: ...


class GenesisModelBackend:
    """First-party inference backend."""

    def __init__(
        self,
        model: GenesisModel,
        tokenizer: Tokenizer,
        *,
        temperature: float = 0.7,
        top_k: int = 50,
        top_p: float = 0.95,
        max_new_tokens: int = 256,
        images: torch.Tensor | None = None,
    ) -> None:
        self.model = model
        self.tokenizer = tokenizer
        self.temperature = temperature
        self.top_k = top_k
        self.top_p = top_p
        self.max_new_tokens = max_new_tokens
        self.images = images

    def generate(self, messages: Sequence[dict], tools: Sequence[dict]) -> str:
        ids = self.tokenizer.apply_chat_template(messages, tools=tools)
        generated = self.model.generate_ids(
            ids,
            max_new_tokens=self.max_new_tokens,
            temperature=self.temperature,
            top_k=self.top_k,
            top_p=self.top_p,
            eos_id=EOS,
            images=self.images,
            forbidden_ids=[self.tokenizer.id_of("<|pad|>")],
        )
        cut = len(generated)
        for index, token in enumerate(generated):
            if token in (EOS, self.tokenizer.id_of("<|end|>")):
                cut = index
                break
        return self.tokenizer.decode(generated[:cut], skip_special=False).strip()


class ScriptedBackend:
    """Deterministic backend for tests and offline control-flow verification."""

    def __init__(self, responses: Sequence[str]) -> None:
        self._responses = list(responses)
        self._index = 0
        self.prompts: list[list[dict]] = []

    def generate(self, messages: Sequence[dict], tools: Sequence[dict]) -> str:
        self.prompts.append([dict(m) for m in messages])
        if self._index >= len(self._responses):
            return "No further scripted response."
        response = self._responses[self._index]
        self._index += 1
        return response


@dataclass
class AgentEvent:
    kind: str
    detail: dict[str, Any]
    timestamp: float = field(default_factory=time.time)


@dataclass
class AgentResult:
    answer: str
    steps: int
    tool_calls: list[ToolCall]
    tool_results: list[ToolResult]
    events: list[AgentEvent]
    stopped_reason: str = "answer"

    def to_dict(self) -> dict[str, Any]:
        return {
            "answer": self.answer,
            "steps": self.steps,
            "tool_calls": [
                {"name": c.name, "arguments": c.arguments} for c in self.tool_calls
            ],
            "tool_results": [r.to_dict() for r in self.tool_results],
            "stopped_reason": self.stopped_reason,
            "events": [{"kind": e.kind, "detail": e.detail, "timestamp": e.timestamp} for e in self.events],
        }


class GenesisAgent:
    def __init__(
        self,
        backend: Backend,
        registry: ToolRegistry,
        *,
        workspace: str | Path = ".",
        system_prompt: str = "You are Genesis, a careful agent. Use tools when they help. Never invent a tool result.",
        max_steps: int = 6,
        approver=None,
        allow_network: bool = False,
    ) -> None:
        self.backend = backend
        self.registry = registry
        self.workspace = Path(workspace)
        self.system_prompt = system_prompt
        self.max_steps = max_steps
        self.context = ToolContext(
            workspace=self.workspace, allow_network=allow_network, approver=approver
        )

    def run(self, task: str) -> AgentResult:
        messages: list[dict] = [
            {"role": "system", "content": self.system_prompt},
            {"role": "user", "content": task},
        ]
        events: list[AgentEvent] = []
        calls: list[ToolCall] = []
        results: list[ToolResult] = []
        tools = self.registry.schemas()
        events.append(AgentEvent("start", {"task": task, "tools": [t["name"] for t in tools]}))

        for step in range(1, self.max_steps + 1):
            output = self.backend.generate(messages, tools)
            events.append(AgentEvent("model", {"step": step, "output": output}))
            proposed = parse_tool_calls(output)
            if not proposed:
                events.append(AgentEvent("answer", {"text": output}))
                return AgentResult(output, step, calls, results, events, "answer")

            messages.append({"role": "assistant", "content": output})
            for call in proposed:
                calls.append(call)
                result = self.registry.call(call.name, call.arguments, self.context)
                results.append(result)
                events.append(AgentEvent(
                    "tool",
                    {"name": call.name, "arguments": call.arguments, "result": result.to_dict()},
                ))
                messages.append({"role": "tool", "content": _result_payload(result)})

        events.append(AgentEvent("stop", {"reason": "max_steps"}))
        return AgentResult(
            "Stopped after the maximum number of steps without a final answer.",
            self.max_steps, calls, results, events, "max_steps",
        )


def _result_payload(result: ToolResult) -> str:
    return json.dumps(result.to_dict(), ensure_ascii=False)
