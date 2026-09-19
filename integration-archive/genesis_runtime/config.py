"""Genesis permission profiles, protected paths, budgets, secrets.

Profiles MAP to runtime policy; they never replace it. Protected paths may
restrict further, never weaken. Runtime remains authoritative on every
decision.
"""
from __future__ import annotations

# profile -> runtime mapping (requested; runtime decides)
PERMISSION_PROFILES = {
    "SAFE_EXPLORATION": {"mode": "plan", "approval": "READ_ONLY",
                         "review": False, "human_gate": "NONE",
                         "collision": "REFUSE"},
    "ASSISTED_BUILD": {"mode": "build", "approval": "ASK_ALL_WRITES",
                       "review": True, "human_gate": "ON_REVIEW_REJECT",
                       "collision": "REQUIRE_APPROVAL"},
    "AUTONOMOUS_SANDBOX": {"mode": "hybrid", "approval": "AUTO_SAFE",
                           "review": True, "human_gate": "NONE",
                           "collision": "REQUIRE_APPROVAL"},
    "PRECIOUS_PROJECT": {"mode": "build", "approval": "ASK_ALL_WRITES",
                         "review": True, "human_gate": "BEFORE_COMPLETE",
                         "collision": "REQUIRE_APPROVAL"},
}

# exact profile for the eventual real-Genesis workflow (locked, documented)
PRECIOUS_PROJECT_PROFILE = {
    "approval": "ASK_ALL_WRITES",
    "human_gate": "BEFORE_COMPLETE",
    "collision": "REQUIRE_APPROVAL",
    "review": True,
    "rollback": True,
    "checkpoint_preview": True,
    "network": "LOCAL_MODEL_NETWORK",
    "shell_profile": "dev",
    "test_profile": "python",
    "permanent_delete": "elevated-approval-only",
}

DEFAULT_PROTECTED_PATHS = [".env", "secrets/", "keys/", "production/",
                           "data/", "wallet/", "identity/", "private/"]

SECRET_PATTERNS = [
    ("api_key", r"(?i)(api[_-]?key|apikey)\s*[:=]\s*['\"]?([\w\-]{8,})['\"]?"),
    ("token", r"(?i)(token|bearer)\s*[:=]\s*['\"]?([\w\-\.]{8,})['\"]?"),
    ("password", r"(?i)(password|passwd|pwd)\s*[:=]\s*['\"]?([^\s'\"]{4,})['\"]?"),
    ("private_key", r"-----BEGIN [A-Z ]*PRIVATE KEY-----"),
    ("env_secret", r"(?i)^(AWS_|GITHUB_|OPENAI_|SECRET_)[A-Z_]*=.+"),
]

# conservative per-profile change budgets (integration level; runtime
# quotas stay authoritative underneath)
CHANGE_BUDGETS = {
    "SAFE_EXPLORATION": {"max_files_changed": 0, "max_files_created": 0,
                         "max_lines_modified": 0, "max_commands": 0,
                         "max_task_seconds": 600},
    "ASSISTED_BUILD": {"max_files_changed": 10, "max_files_created": 5,
                       "max_lines_modified": 500, "max_commands": 20,
                       "max_task_seconds": 1200},
    "AUTONOMOUS_SANDBOX": {"max_files_changed": 25, "max_files_created": 15,
                           "max_lines_modified": 2000, "max_commands": 40,
                           "max_task_seconds": 1800},
    "PRECIOUS_PROJECT": {"max_files_changed": 8, "max_files_created": 3,
                         "max_lines_modified": 300, "max_commands": 15,
                         "max_task_seconds": 1200},
}
