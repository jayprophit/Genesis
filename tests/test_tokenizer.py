import json
from pathlib import Path

import pytest
import torch

from genesis_model.tokenizer import BOS, EOS, Tokenizer


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
