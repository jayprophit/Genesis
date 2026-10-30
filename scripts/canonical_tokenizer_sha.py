#!/usr/bin/env python3
"""Compute the canonical tokenizer digest for a Genesis tokenizer.json.

Canonical algorithm (must match genesis_model/checkpoint._canonical_tokenizer_sha):

    sha256(json.dumps(json.loads(tokenizer_file), sort_keys=True, ensure_ascii=False))

This differs from a raw file-byte SHA-256: identical tokenizers serialized with
different whitespace compare equal, while a single changed merge breaks the digest.

Usage:
    python scripts/canonical_tokenizer_sha.py <path/to/tokenizer.json>

Exit codes: 0 = success (prints digest), 1 = error.
"""
from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path


def canonical_tokenizer_sha(tokenizer_path: str | Path) -> str:
    """Return the canonical SHA-256 of a tokenizer JSON file."""
    data = json.loads(Path(tokenizer_path).read_bytes())
    return hashlib.sha256(
        json.dumps(data, sort_keys=True, ensure_ascii=False).encode("utf-8")
    ).hexdigest()


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("Usage: canonical_tokenizer_sha.py <path/to/tokenizer.json>",
              file=sys.stderr)
        return 1
    try:
        digest = canonical_tokenizer_sha(argv[1])
    except (OSError, json.JSONDecodeError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1
    print(digest)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
