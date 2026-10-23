import json

from genesis_model.agent import GenesisAgent, ScriptedBackend
from genesis_model.tools import default_registry


def test_agent_executes_one_tool_then_answers(tmp_path):
    backend = ScriptedBackend([
        '<|tool_call|>{"name":"calculator","arguments":{"expression":"17 * 24"}}<|/tool_call|>',
        "17 * 24 is 408.",
    ])
    agent = GenesisAgent(backend, default_registry(), workspace=tmp_path)
    result = agent.run("What is 17 * 24?")
    assert result.answer == "17 * 24 is 408."
    assert result.steps == 2
    assert len(result.tool_calls) == 1
    assert result.tool_results[0].ok
    assert result.tool_results[0].output["value"] == 408


def test_agent_stops_at_max_steps(tmp_path):
    backend = ScriptedBackend(['<|tool_call|>{"name":"now","arguments":{}}<|/tool_call|>'] * 10)
    agent = GenesisAgent(backend, default_registry(), workspace=tmp_path, max_steps=2)
    result = agent.run("loop")
    assert result.stopped_reason == "max_steps"
    assert len(result.tool_calls) == 2


def test_agent_reports_tool_error_to_model(tmp_path):
    backend = ScriptedBackend([
        '<|tool_call|>{"name":"read_file","arguments":{"path":"missing.txt"}}<|/tool_call|>',
        "The file does not exist.",
    ])
    agent = GenesisAgent(backend, default_registry(), workspace=tmp_path)
    result = agent.run("read missing")
    assert result.tool_results[0].ok is False
    payload = json.loads(agent.backend.prompts[-1][-1]["content"])
    assert payload["error_code"] == "not_a_file"
