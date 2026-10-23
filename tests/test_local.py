import json
from dataclasses import replace
from pathlib import Path

from genesis_model.config import PRESETS, ModelConfig, estimate_parameters
from genesis_model.corpus import document_sample, expand_paths, iter_documents
from genesis_model.model import GenesisModel
from genesis_model.train import TrainConfig, train


def test_local_presets_fit_a_4gb_card():
    expected = {
        "local_1m": (1_000_000, 1_500_000),
        "local_3m": (2_800_000, 3_500_000),
        "local_6m": (5_800_000, 6_800_000),
        "local_15m": (14_500_000, 16_500_000),
        "local_30m": (25_000_000, 28_000_000),
    }
    for name, (low, high) in expected.items():
        config = PRESETS[name]
        assert config.vision is None
        assert config.max_seq_len >= 256
        model = GenesisModel(config)
        params = model.num_parameters()
        assert low <= params <= high, (name, params)
        assert estimate_parameters(config) == params, name


def test_corpus_discovery_and_sample(tmp_path):
    (tmp_path / "a.txt").write_text("alpha beta gamma", encoding="utf-8")
    nested = tmp_path / "nested"
    nested.mkdir()
    (nested / "b.md").write_text("# heading\nbody text", encoding="utf-8")
    (nested / "c.jsonl").write_text(
        json.dumps({"text": "jsonl document"}) + "\n" + json.dumps({"skip": True}) + "\n",
        encoding="utf-8",
    )
    (nested / "ignore.bin").write_bytes(b"\x00\x01")

    files = expand_paths([tmp_path])
    assert {path.name for path in files} == {"a.txt", "b.md", "c.jsonl"}
    documents = list(iter_documents([tmp_path]))
    assert "alpha beta gamma" in documents
    assert "jsonl document" in documents
    sample = document_sample([tmp_path], max_chars=1)
    assert len(sample) == 1


def test_resume_continues_step_and_token_counts(tmp_path):
    model = ModelConfig(
        vocab_size=300, d_model=32, n_layers=1, n_heads=4, n_kv_heads=1,
        d_ff=64, max_seq_len=32, vision=None,
    )
    base = TrainConfig(
        steps=2, batch_size=2, seq_len=16, lr=1e-3, warmup_steps=0,
        log_every=1, out_dir=str(tmp_path / "run"), device="cpu", seed=3,
        sft_weight=0.0, vision_weight=0.0, model=model,
    )
    first = train(base)
    assert first["steps"] == 2
    assert first["tokens_seen"] == 64

    second = train(replace(base, steps=4), resume_from=first["checkpoint"])
    assert second["start_step"] == 2
    assert second["steps_this_run"] == 2
    assert second["steps"] == 4
    assert second["tokens_seen"] == 128
    assert len(second["history"]) == 4


def test_time_limit_keeps_a_checkpoint(tmp_path):
    model = ModelConfig(
        vocab_size=300, d_model=32, n_layers=1, n_heads=4, n_kv_heads=1,
        d_ff=64, max_seq_len=32, vision=None,
    )
    config = TrainConfig(
        steps=10, batch_size=2, seq_len=16, lr=1e-3, warmup_steps=0,
        log_every=1, out_dir=str(tmp_path / "run"), device="cpu", seed=4,
        sft_weight=0.0, vision_weight=0.0, model=model, max_seconds=1e-9,
    )
    manifest = train(config)
    assert manifest["timed_out"] is True
    assert manifest["steps"] == 1
    assert Path(manifest["checkpoint"]).exists()
