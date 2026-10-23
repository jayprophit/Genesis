import json
from pathlib import Path

from genesis_model.checkpoint import load_checkpoint
from genesis_model.config import ModelConfig, VisionConfig
from genesis_model.eval import run_evaluation
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
