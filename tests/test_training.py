import json
from pathlib import Path

from genesis_model.checkpoint import load_checkpoint
from genesis_model.config import ModelConfig, VisionConfig
from genesis_model.eval import run_evaluation
from genesis_model.tokenizer import Tokenizer
from genesis_model.train import TrainConfig, train


def test_training_reduces_loss_and_reloads(tmp_path):
    config = TrainConfig(
        steps=6,
        batch_size=2,
        seq_len=32,
        lr=3e-3,
        warmup_steps=1,
        log_every=1,
        out_dir=str(tmp_path),
        device="cpu",
        seed=0,
        model=ModelConfig(
            vocab_size=512, d_model=64, n_layers=2, n_heads=4, n_kv_heads=2,
            d_ff=172, max_seq_len=64,
            vision=VisionConfig(image_size=32, patch_size=8, width=32, layers=1, heads=4),
        ),
    )
    manifest = train(config)
    assert manifest["steps"] == 6
    checkpoint = Path(manifest["checkpoint"])
    assert checkpoint.exists()

    model, tokenizer, meta = load_checkpoint(checkpoint)
    assert model.config.vocab_size == tokenizer.vocab_size
    assert meta.steps == 6

    report = run_evaluation(checkpoint, include_tool_cases=False)
    assert report.parameters > 0
    assert report.metrics["throughput"]["generated_tokens"] > 0


def test_training_from_memmap_shard(tmp_path):
    from genesis_model.data import tokenize_to_shard

    texts = ["Genesis learns text, images and tools. " * 8, "你好，世界。" * 8]
    tokenizer = Tokenizer.train(texts, vocab_size=300)
    tokenizer_path = tmp_path / "tokenizer.json"
    tokenizer.save(tokenizer_path)
    shard_path = tmp_path / "shard.bin"
    tokenize_to_shard(texts, tokenizer, shard_path)

    config = TrainConfig(
        steps=2,
        batch_size=1,
        seq_len=16,
        lr=1e-3,
        warmup_steps=0,
        log_every=1,
        out_dir=str(tmp_path / "run"),
        device="cpu",
        seed=1,
        sft_weight=0.0,
        vision_weight=0.0,
        shard_path=str(shard_path),
        tokenizer_path=str(tokenizer_path),
        model=ModelConfig(
            vocab_size=300, d_model=32, n_layers=1, n_heads=4, n_kv_heads=1,
            d_ff=96, max_seq_len=32, vision=None,
        ),
    )
    manifest = train(config)
    assert manifest["tokens_seen"] > 0
    assert Path(manifest["checkpoint"]).exists()

