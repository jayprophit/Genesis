import json
import random
from pathlib import Path

import pytest
import torch

from genesis_model.tokenizer import BASE_OFFSET, BOS, EOS, Tokenizer


def test_round_trip_multilingual():
    tokenizer = Tokenizer.train(["hello world hello world hello", "你好，世界。你好，世界。"], vocab_size=320)
    for text in ["hello world", "你好，世界。", "emoji: 🧬🚀", "tab\tnewline\n"]:
        ids = tokenizer.encode(text)
        assert tokenizer.decode(ids) == text


def test_special_tokens_round_trip():
    tokenizer = Tokenizer.train(["abc abc abc"], vocab_size=280)
    text = "<|system|>hi<|end|><|tool_call|>{\"name\":\"now\"}<|/tool_call|>"
    ids = tokenizer.encode(text)
    assert tokenizer.id_of("<|tool_call|>") in ids
    assert tokenizer.decode(ids) == text


def test_save_load(tmp_path):
    tokenizer = Tokenizer.train(["the quick brown fox jumps over the lazy dog"] * 3, vocab_size=300)
    path = tmp_path / "tokenizer.json"
    tokenizer.save(path)
    loaded = Tokenizer.load(path)
    assert loaded.vocab_size == tokenizer.vocab_size
    assert loaded.encode("the quick brown fox") == tokenizer.encode("the quick brown fox")


def test_chat_template_contains_roles():
    tokenizer = Tokenizer.train(["user assistant system"], vocab_size=280)
    ids = tokenizer.apply_chat_template([{"role": "user", "content": "hi"}])
    assert ids[0] == BOS
    assert tokenizer.id_of("<|user|>") in ids
    assert ids[-1] == tokenizer.id_of("<|assistant|>")


def test_rejects_bad_vocab():
    with pytest.raises(ValueError):
        Tokenizer.train(["x"], vocab_size=10)


def _naive_encode_bytes(tokenizer: Tokenizer, data: bytes) -> list[int]:
    """The reference rule: merge the lowest-ranked pair, all occurrences.

    ``Tokenizer._encode_bytes`` is a linked-list and heap implementation of
    this loop. Keeping the loop here means the fast path is pinned to the
    definition rather than to itself.
    """
    ids = list(data)
    while True:
        best_rank = None
        best_pair = None
        for pair in zip(ids, ids[1:]):
            rank = tokenizer._ranks.get(pair)
            if rank is not None and (best_rank is None or rank < best_rank):
                best_rank = rank
                best_pair = pair
        if best_pair is None:
            return ids
        first, second = best_pair
        new_id = BASE_OFFSET + best_rank
        merged: list[int] = []
        i = 0
        while i < len(ids):
            if i + 1 < len(ids) and ids[i] == first and ids[i + 1] == second:
                merged.append(new_id)
                i += 2
            else:
                merged.append(ids[i])
                i += 1
        ids = merged


def test_fast_encoder_matches_the_merge_rule():
    tokenizer = Tokenizer.train(
        ["the quick brown fox jumps over the lazy dog " * 40,
         "aaaa bbbb abab abba " * 30,
         "你好，世界。你好，世界。" * 20],
        vocab_size=340,
    )
    cases = [
        b"",
        b"a",
        b"the quick brown fox",
        b"a" * 200,
        b"ab" * 100,
        b"abababab",
        b"aaaa bbbb abab abba " * 10,
        "café — naïve ✓ 日本語 🧬".encode("utf-8"),
    ]
    rng = random.Random(20261025)
    alphabet = b"abc .,\n"
    for _ in range(24):
        cases.append(bytes(rng.choice(alphabet) for _ in range(rng.randint(1, 300))))

    for data in cases:
        assert tokenizer._encode_bytes(data) == _naive_encode_bytes(tokenizer, data)
        assert tokenizer.decode(tokenizer._encode_bytes(data), skip_special=True) == data.decode(
            "utf-8", errors="replace"
        )
