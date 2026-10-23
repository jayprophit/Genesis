"""Tokenize a local corpus into a flat memmap shard.

Example:
    python scripts/tokenize_corpus.py ^
        --corpus data/corpus/genesis_seed.txt ^
        --out data/tokenized ^
        --vocab-size 8192

Then:
    python -m genesis_model.cli train --preset local_6m ^
        --shard data/tokenized/shard_0000.bin ^
        --tokenizer data/tokenized/tokenizer.json ^
        --out runs/local_6m
"""
from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from genesis_model.corpus import (  # noqa: E402
    corpus_manifest,
    document_sample,
    expand_paths,
    iter_documents,
    sha256_file,
)
from genesis_model.data import tokenize_to_shard  # noqa: E402
from genesis_model.tokenizer import Tokenizer  # noqa: E402


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--corpus", action="append", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--vocab-size", type=int, default=8192)
    parser.add_argument("--max-tokenizer-chars", type=int, default=500_000)
    parser.add_argument("--shard-name", default="shard_0000.bin")
    parser.add_argument("--dtype", default="uint32", choices=["uint16", "uint32"])
    parser.add_argument("--reuse-tokenizer", action="store_true")
    args = parser.parse_args()

    corpus = expand_paths(args.corpus)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    tokenizer_path = out / "tokenizer.json"

    started = time.time()
    if args.reuse_tokenizer and tokenizer_path.exists():
        tokenizer = Tokenizer.load(tokenizer_path)
        print(f"loaded tokenizer: vocab={tokenizer.vocab_size}", flush=True)
    else:
        sample = document_sample(corpus, max_chars=args.max_tokenizer_chars)
        if not sample:
            raise SystemExit("no documents found")
        chars = sum(len(document) for document in sample)
        print(f"training tokenizer on {chars:,} characters...", flush=True)
        tokenizer = Tokenizer.train(sample, vocab_size=args.vocab_size, verbose=True)
        tokenizer.save(tokenizer_path)
        print(f"tokenizer: vocab={tokenizer.vocab_size} -> {tokenizer_path}", flush=True)

    shard_path = out / args.shard_name
    print(f"tokenizing into {shard_path} ...", flush=True)
    count = tokenize_to_shard(
        iter_documents(corpus), tokenizer, shard_path, dtype=args.dtype
    )

    manifest = {
        "format_version": 1,
        "created_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "duration_s": round(time.time() - started, 2),
        "shard": str(shard_path),
        "shard_sha256": sha256_file(shard_path),
        "dtype": args.dtype,
        "tokens": count,
        "shard_bytes": shard_path.stat().st_size,
        "tokenizer": str(tokenizer_path),
        "tokenizer_vocab": tokenizer.vocab_size,
        "sources": corpus_manifest(corpus),
    }
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(json.dumps(manifest, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
