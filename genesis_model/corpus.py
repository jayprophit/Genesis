"""Local corpus discovery and streaming.

The local-first path should work with ordinary folders of text, Markdown and
JSONL files. Nothing here downloads data: the owner decides what enters the
corpus, and every file is hashed before training.
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Iterator, Sequence

DOCUMENT_SUFFIXES = {".txt", ".md", ".markdown", ".jsonl", ".json"}


def expand_paths(
    paths: Sequence[str | Path],
    suffixes: set[str] | None = None,
) -> list[Path]:
    """Expand files and directories into a sorted, de-duplicated file list."""
    allowed = suffixes or DOCUMENT_SUFFIXES
    files: list[Path] = []
    seen: set[Path] = set()
    for raw in paths:
        path = Path(raw)
        if path.is_dir():
            candidates = sorted(
                p for p in path.rglob("*")
                if p.is_file() and p.suffix.lower() in allowed
            )
        elif path.is_file():
            candidates = [path]
        else:
            raise FileNotFoundError(f"corpus path not found: {path}")
        for candidate in candidates:
            resolved = candidate.resolve()
            if resolved not in seen:
                seen.add(resolved)
                files.append(candidate)
    if not files:
        raise ValueError("no corpus files found")
    return files


def iter_documents(paths: Sequence[str | Path]) -> Iterator[str]:
    """Yield text documents from .txt/.md/.jsonl/.json files.

    JSONL records use a ``text`` or ``content`` field. JSON files may be a
    single object, a list of objects, or a list of strings. Records without
    text are skipped rather than serialised into the corpus.
    """
    for path in expand_paths(paths):
        suffix = path.suffix.lower()
        if suffix == ".jsonl":
            with path.open("r", encoding="utf-8") as handle:
                for line in handle:
                    line = line.strip()
                    if not line:
                        continue
                    record = json.loads(line)
                    text = _record_text(record)
                    if text:
                        yield text
        elif suffix == ".json":
            data = json.loads(path.read_text(encoding="utf-8"))
            items = data if isinstance(data, list) else [data]
            for item in items:
                text = item if isinstance(item, str) else _record_text(item)
                if text:
                    yield text
        else:
            yield path.read_text(encoding="utf-8", errors="replace")


def _record_text(record: object) -> str | None:
    if not isinstance(record, dict):
        return None
    for key in ("text", "content", "document"):
        value = record.get(key)
        if isinstance(value, str) and value.strip():
            return value
    return None


def sha256_file(path: str | Path) -> str:
    digest = hashlib.sha256()
    with Path(path).open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def document_sample(
    paths: Sequence[str | Path],
    *,
    max_chars: int,
) -> list[str]:
    """Take a bounded sample for tokenizer training."""
    sample: list[str] = []
    total = 0
    for document in iter_documents(paths):
        sample.append(document)
        total += len(document)
        if total >= max_chars:
            break
    return sample


def corpus_manifest(paths: Sequence[str | Path]) -> list[dict]:
    return [
        {
            "path": str(path),
            "bytes": path.stat().st_size,
            "sha256": sha256_file(path),
        }
        for path in expand_paths(paths)
    ]
