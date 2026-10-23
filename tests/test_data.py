from pathlib import Path

import pytest

from genesis_model.data import MemmapTokenDataset, PackedTextDataset, tokenize_to_shard
from genesis_model.tokenizer import EOS, Tokenizer


def test_packed_text_blocks_are_shifted():
    dataset = PackedTextDataset(list(range(20)), block_size=4)
    x, y = dataset[0]
    assert x.tolist() == [0, 1, 2, 3]
    assert y.tolist() == [1, 2, 3, EOS]
    assert len(dataset) == 5


def test_memmap_shard_roundtrip(tmp_path):
    tokenizer = Tokenizer.train(
        ["Genesis learns from text and images. " * 4, "你好，世界。" * 4],
        vocab_size=320,
    )
    path = tmp_path / "shard.bin"
    count = tokenize_to_shard(
        ["Genesis learns from text and images. " * 3, "你好，世界。" * 3],
        tokenizer,
        path,
        flush_tokens=16,
    )
    assert count > 0
    assert path.exists()

    dataset = MemmapTokenDataset(path, block_size=8)
    assert len(dataset) >= 1
    x, y = dataset[0]
    assert x.shape == (8,) and y.shape == (8,)
    assert x.tolist()[1:] == y.tolist()[:-1]
    # EOS is written between documents.
    assert EOS in dataset._array.tolist()


def test_memmap_rejects_out_of_range(tmp_path):
    tokenizer = Tokenizer.train(["abc " * 20], vocab_size=280)
    path = tmp_path / "shard.bin"
    tokenize_to_shard(["abc " * 20], tokenizer, path)
    dataset = MemmapTokenDataset(path, block_size=4)
    with pytest.raises(IndexError):
        _ = dataset[len(dataset) + 1]
