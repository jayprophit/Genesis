import torch

from genesis_model.config import ModelConfig, VisionConfig
from genesis_model.model import GenesisModel, KVCache


def tiny_config(vision=False):
    return ModelConfig(
        vocab_size=300, d_model=64, n_layers=2, n_heads=4, n_kv_heads=2,
        d_ff=172, max_seq_len=64,
        vision=VisionConfig(image_size=32, patch_size=8, width=32, layers=1, heads=4) if vision else None,
    )


def test_forward_shapes_and_loss():
    model = GenesisModel(tiny_config())
    x = torch.randint(0, 300, (2, 16))
    y = torch.randint(0, 300, (2, 16))
    logits, loss = model(x, targets=y)
    assert logits.shape == (2, 16, 300)
    assert loss.ndim == 0 and loss.item() > 0


def test_vision_prefix():
    model = GenesisModel(tiny_config(vision=True))
    x = torch.randint(0, 300, (2, 8))
    images = torch.rand(2, 3, 32, 32)
    logits, loss = model(x, images=images, targets=x)
    assert logits.shape == (2, 8, 300)
    assert torch.isfinite(loss)


def test_gradients_reach_vision_patch_embedding():
    model = GenesisModel(tiny_config(vision=True))
    x = torch.randint(0, 300, (1, 4))
    images = torch.rand(1, 3, 32, 32)
    _, loss = model(x, images=images, targets=x)
    loss.backward()
    grad = model.vision.patch_embed.weight.grad
    assert grad is not None and grad.abs().sum() > 0


def test_kv_cache_matches_full_forward():
    torch.manual_seed(0)
    model = GenesisModel(tiny_config())
    model.eval()
    ids = torch.randint(0, 300, (1, 10))
    with torch.inference_mode():
        full = model(ids)
        cache = KVCache(model.config.n_layers)
        prefix = model(ids[:, :9], cache=cache)
        last = model(ids[:, 9:], cache=cache, position_offset=9)
    assert torch.allclose(prefix[:, -1], full[:, 8], atol=1e-4)
    assert torch.allclose(last[:, -1], full[:, 9], atol=1e-4)


def test_generate_extends_sequence():
    model = GenesisModel(tiny_config())
    out = model.generate_ids([1, 5, 6], max_new_tokens=5, temperature=0.0)
    assert isinstance(out, list)
    assert len(out) <= 5
