import json
from pathlib import Path

import pytest

from genesis_model.tools import (
    ToolContext,
    ToolError,
    ToolRegistry,
    default_registry,
    parse_tool_calls,
)


def test_calculator_and_injection_rejected(tmp_path):
    registry = default_registry()
    context = ToolContext(workspace=tmp_path)
    result = registry.call("calculator", {"expression": "17 * 24"}, context)
    assert result.ok and result.output["value"] == 408
    bad = registry.call("calculator", {"expression": "__import__('os').system('echo x')"}, context)
    assert not bad.ok and bad.error_code == "unsafe_expression"


def test_path_escape_denied(tmp_path):
    registry = default_registry()
    context = ToolContext(workspace=tmp_path)
    result = registry.call("read_file", {"path": "../../secret.txt"}, context)
    assert not result.ok and result.error_code == "path_outside_workspace"


def test_write_requires_approval(tmp_path):
    registry = default_registry()
    context = ToolContext(workspace=tmp_path)
    denied = registry.call("write_file", {"path": "a.txt", "content": "x"}, context)
    assert not denied.ok and denied.error_code == "approval_required"
    approved = registry.call(
        "write_file", {"path": "a.txt", "content": "x"},
        ToolContext(workspace=tmp_path, approver=lambda call, tool: True),
    )
    assert approved.ok
    assert (tmp_path / "a.txt").read_text() == "x"


def test_schema_validation(tmp_path):
    registry = default_registry()
    context = ToolContext(workspace=tmp_path)
    result = registry.call("list_dir", {"path": 3}, context)
    assert not result.ok and result.error_code == "invalid_arguments"


def test_parse_native_and_fenced():
    native = 'text <|tool_call|>{"name":"calculator","arguments":{"expression":"1+1"}}<|/tool_call|>'
    calls = parse_tool_calls(native)
    assert len(calls) == 1 and calls[0].name == "calculator"
    fenced = '```json\n{"name":"now","arguments":{}}\n```'
    calls = parse_tool_calls(fenced)
    assert len(calls) == 1 and calls[0].name == "now"


def test_parse_rejects_malformed():
    assert parse_tool_calls("<|tool_call|>not json<|/tool_call|>") == []
    assert parse_tool_calls("<|tool_call|>{\"arguments\":{}}<|/tool_call|>") == []
