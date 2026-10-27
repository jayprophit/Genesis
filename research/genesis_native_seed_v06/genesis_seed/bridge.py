"""Reference capability boundary. Does NOT make Agent-Bridge tool calls."""
from __future__ import annotations
from dataclasses import dataclass
from typing import Protocol, Any

@dataclass(frozen=True)
class ActionRequest:
    operation:str
    arguments:dict[str,Any]
    action_id:str

@dataclass(frozen=True)
class Authorization:
    action_id:str
    approved_operations:frozenset[str]
    granted_by_user:bool=False

class ExecutionProvider(Protocol):
    def execute(self, request:ActionRequest)->dict[str,Any]:...


def authorized_execute(request:ActionRequest,approval:Authorization|None,
                       provider:ExecutionProvider)->dict[str,Any]:
    if approval is None or not approval.granted_by_user or approval.action_id!=request.action_id or request.operation not in approval.approved_operations:
        raise PermissionError('Action denied: independent authorization required')
    return provider.execute(request)

class TestEchoProvider:
    """In-memory verification only, no shell/file/network execution."""
    def execute(self,request:ActionRequest)->dict[str,Any]:
        return {'action_id':request.action_id,'operation':request.operation,
                'arguments':request.arguments,'executed_by':'test-only-echo'}
