"""Read-only project discovery (Stage 01). Filesystem reads + public
search/status APIs only. Never mutates. Never pointed at real Genesis
in this stage (tests use the synthetic sandbox).
"""
from __future__ import annotations

import fnmatch
import re
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any

from genesis_runtime.config import DEFAULT_PROTECTED_PATHS, SECRET_PATTERNS

SOURCE_EXT = {".py": "SOURCE", ".js": "SOURCE", ".ts": "SOURCE",
              ".java": "SOURCE", ".go": "SOURCE", ".rs": "SOURCE",
              ".c": "SOURCE", ".cpp": "SOURCE", ".cs": "SOURCE"}
TEST_HINTS = ("test_", "_test", "/tests/", "/test/")
CONFIG_EXT = {".json", ".yaml", ".yml", ".toml", ".ini", ".cfg", ".env"}
DOC_EXT = {".md", ".rst", ".txt"}
BINARY_EXT = {".pyc", ".exe", ".dll", ".so", ".png", ".jpg", ".zip",
              ".whl", ".bin"}
GENERATED_HINTS = ("__pycache__", ".bridge", "node_modules", ".git/")


def classify_file(rel: str, head: str = "") -> str:
    low = rel.lower()
    if any(g in low for g in GENERATED_HINTS):
        return "GENERATED"
    if any(fnmatch.fnmatch(low, f"*{e}") for e in BINARY_EXT):
        return "BINARY"
    for _name, rx in SECRET_PATTERNS:
        if re.search(rx, head, re.M):
            return "SECRET_SUSPECT"
    if any(h in low for h in TEST_HINTS):
        return "TEST"
    ext = Path(rel).suffix.lower()
    if ext in SOURCE_EXT:
        return "SOURCE"
    if ext in CONFIG_EXT or Path(rel).name.startswith(".env"):
        return "CONFIG"
    if ext in DOC_EXT:
        return "DOC"
    if ext in (".csv", ".db", ".sqlite", ".dat"):
        return "DATA"
    return "UNKNOWN"


def scan_secrets(text: str) -> list[dict[str, str]]:
    """Warning layer only: kinds + locations, NEVER values."""
    hits = []
    for i, line in enumerate(text.splitlines(), 1):
        for kind, rx in SECRET_PATTERNS:
            if re.search(rx, line):
                hits.append({"kind": kind, "line": i,
                             "hint": "redacted"})
                break
    return hits


@dataclass
class GenesisProjectManifest:
    path: str = ""
    name: str = ""
    languages: list = field(default_factory=list)
    frameworks: list = field(default_factory=list)
    entry_points: list = field(default_factory=list)
    test_system: str = ""
    git_present: bool = False
    important_dirs: list = field(default_factory=list)
    protected_paths: list = field(default_factory=list)
    file_count: int = 0
    total_bytes: int = 0
    runtime_compatible: bool = True
    discovered_at: str = ""

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


def _is_protected(rel: str, protected: list[str]) -> bool:
    low = rel.lower().replace("\\", "/")
    for p in protected:
        pl = p.lower().rstrip("/")
        if low == pl or low.startswith(pl + "/") or low == pl.lstrip("./"):
            return True
    return False


def discover(project: Path, protected: list[str] | None = None,
             max_files: int = 2000, excerpt_chars: int = 400) -> dict[str, Any]:
    """Bounded read-only discovery. Skips protected paths' CONTENTS
    (lists names only) and caps work for LOW_RESOURCE."""
    import datetime as _dt
    protected = protected or list(DEFAULT_PROTECTED_PATHS)
    project = Path(project).resolve()
    files: list[dict[str, Any]] = []
    total = 0
    counts: dict[str, int] = {}
    for p in sorted(project.rglob("*")):
        if len(files) >= max_files:
            break
        if not p.is_file():
            continue
        try:
            rel = str(p.relative_to(project)).replace("\\", "/")
        except ValueError:
            continue
        size = 0
        try:
            size = p.stat().st_size
        except OSError:
            continue
        total += size
        head = ""
        secret_hits: list[dict[str, str]] = []
        is_prot = _is_protected(rel, protected)
        if not is_prot and size < 200_000 and Path(rel).suffix.lower() not in BINARY_EXT:
            try:
                head = p.read_text(encoding="utf-8", errors="strict")[:excerpt_chars]
                secret_hits = scan_secrets(head)
            except (OSError, ValueError, UnicodeDecodeError):
                head = ""
        kind = classify_file(rel, head if not is_prot else "")
        if is_prot and kind != "GENERATED":
            kind = "SECRET_SUSPECT" if "secret" in rel.lower() or "env" in rel.lower() \
                else kind
        counts[kind] = counts.get(kind, 0) + 1
        files.append({"path": rel, "kind": kind, "bytes": size,
                      "protected": is_prot,
                      "excerpt": "" if is_prot else head[:excerpt_chars],
                      "secret_warnings": secret_hits})
    langs = sorted({{"py": "python", "js": "javascript", "ts": "typescript",
                     "go": "go", "rs": "rust"}.get(Path(f["path"]).suffix.lower().lstrip("."),
                     Path(f["path"]).suffix.lower() or "?")
                    for f in files if f["kind"] == "SOURCE"})
    entry = [f["path"] for f in files
             if Path(f["path"]).name.lower() in
             ("main.py", "app.py", "__main__.py", "cli.py", "server.py",
              "index.js", "main.go")]
    tests = [f["path"] for f in files if f["kind"] == "TEST"]
    test_system = "pytest/unittest" if any(t.endswith(".py") for t in tests) else (
        "npm" if any(t.endswith(".js") for t in tests) else ("present" if tests else "none"))
    man = GenesisProjectManifest(
        path=str(project), name=project.name, languages=langs,
        frameworks=["none-detected"],
        entry_points=entry[:10], test_system=test_system,
        git_present=(project / ".git").exists(),
        important_dirs=sorted({f["path"].split("/")[0] for f in files
                               if "/" in f["path"]})[:20],
        protected_paths=[f["path"] for f in files if f["protected"]][:50],
        file_count=len(files), total_bytes=total, runtime_compatible=True,
        discovered_at=_dt.datetime.now().isoformat(timespec="seconds"))
    return {"manifest": man.to_dict(), "files": files, "kinds": counts,
            "note": "read-only discovery; protected contents never read"}


def index_summary(discovery: dict[str, Any], task_hint: str = "",
                  max_entries: int = 40) -> dict[str, Any]:
    """Task-relevant bounded index: paths + metadata + small excerpts."""
    hint_words = {w.lower() for w in re.findall(r"\w+", task_hint) if len(w) > 3}
    scored = []
    for f in discovery.get("files", []):
        score = 0
        hay = (f["path"] + " " + f.get("excerpt", "")).lower()
        score += sum(2 for w in hint_words if w in hay)
        if f["kind"] in ("SOURCE", "TEST", "CONFIG"):
            score += 1
        if f.get("protected"):
            score -= 5
        scored.append((score, f))
    scored.sort(key=lambda t: -t[0])
    return {"entries": [{"path": f["path"], "kind": f["kind"],
                         "bytes": f["bytes"],
                         "excerpt": f["excerpt"][:200]} for _, f in scored[:max_entries]],
            "total_indexed": len(scored)}
