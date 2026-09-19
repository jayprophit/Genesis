"""Genesis-facing data objects (presentation-neutral, no internals)."""
from __future__ import annotations

from dataclasses import asdict, dataclass, field
from typing import Any


@dataclass
class GenesisTask:
    goal: str
    workspace: str = ""
    mode: str = ""  # "" = from permission profile; else plan|build|hybrid
    constraints: list[str] = field(default_factory=list)
    protected_files: list[str] = field(default_factory=list)
    approval_profile: str = "ASSISTED_BUILD"  # safe default (profile name)
    model_profile: str = "LOW_RESOURCE"
    roles: dict = field(default_factory=dict)
    max_steps: int = 0
    review_enabled: bool = True
    revision_limit: int = 2
    dry_run: bool = False
    final_human_gate: str = "NONE"  # NONE|...|BEFORE_COMPLETE|ALWAYS
    idempotency_key: str = ""

    def __post_init__(self) -> None:
        if self.mode and self.mode not in ("plan", "build", "hybrid"):
            raise ValueError(f"bad mode: {self.mode!r}")
        if not self.goal or not self.goal.strip():
            raise ValueError("task goal is required")


@dataclass
class GenesisSessionInfo:
    session_id: str
    task_id: str = ""
    workspace: str = ""
    mode: str = ""
    status: str = ""
    role: str = ""
    model: str = ""
    step: int = 0


@dataclass
class GenesisStatus:
    runtime: str = ""
    ollama: str = ""
    session: str = ""
    task: str = ""
    mode: str = ""
    role: str = ""
    model: str = ""
    step: int = 0
    progress: str = ""
    context: str = ""
    files: list = field(default_factory=list)
    tests: dict = field(default_factory=dict)
    review: str = ""
    approvals: list = field(default_factory=list)
    verification: dict = field(default_factory=dict)
    errors: list = field(default_factory=list)
    network_policy: str = ""

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


@dataclass
class GenesisResult:
    session_id: str
    task_id: str
    status: str
    summary: str = ""
    files_created: list = field(default_factory=list)
    files_modified: list = field(default_factory=list)
    tests: dict = field(default_factory=dict)
    review: str = ""
    verification: dict = field(default_factory=dict)
    timeline: list = field(default_factory=list)
    errors: list = field(default_factory=list)
    duration_s: float = 0.0

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)
