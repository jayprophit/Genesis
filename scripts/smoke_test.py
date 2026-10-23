"""End-to-end smoke test: tokenizer, forward, cache, tool call, agent.

Run this before claiming the pipeline works:
    python scripts/smoke_test.py
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

import torch  # noqa: E402

from genesis_model.agent import GenesisAgent, ScriptedBackend  # noqa: E402
from genesis_model.config import ModelConfig, VisionConfig  # noqa: E402
from genesis_model.model import GenesisModel  # noqa: E402
from genesis_model.tokenizer import Tokenizer  # noqa: E402
from genesis_model.tools import default_registry, parse_tool_calls  # noqa: E402


def main() -> int:
    torch.set_num_threads(1)
    text = "Genesis learns language, images and tools."
    tokenizer = Tokenizer.train([text, text, "你好，世界。"], vocab_size=320)
    ids = tokenizer.encode(text)
    assert tokenizer.decode(ids) == text, "tokenizer round-trip failed"

    config = ModelConfig(
        vocab_size=tokenizer.vocab_size, d_model=64, n_layers=2, n_heads=4,
        n_kv_heads=2, d_ff=172, max_seq_len=64,
        vision=VisionConfig(image_size=32, patch_size=8, width=32, layers=1, heads=4),
    )
    model = GenesisModel(config)
    x = torch.tensor([ids[:16]])
    logits, loss = model(x, images=torch.rand(1, 3, 32, 32), targets=x[:, :16])
    assert logits.shape[0] == 1 and torch.isfinite(loss)

    out = model.generate_ids(ids[:4], max_new_tokens=4, temperature=0.0)
    assert isinstance(out, list)

    calls = parse_tool_calls('<|tool_call|>{"name":"calculator","arguments":{"expression":"2+2"}}<|/tool_call|>')
    assert calls and calls[0].name == "calculator"

    backend = ScriptedBackend([
        '<|tool_call|>{"name":"calculator","arguments":{"expression":"2+2"}}<|/tool_call|>',
        "The answer is 4.",
    ])
    result = GenesisAgent(backend, default_registry(), workspace=ROOT).run("2+2")
    assert result.tool_results[0].output["value"] == 4
    assert result.answer == "The answer is 4."

    print("SMOKE OK")
    print(f"vocab={tokenizer.vocab_size} params={model.num_parameters()} loss={float(loss.detach()):.4f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
