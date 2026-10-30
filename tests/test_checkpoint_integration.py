"""Regression suite for the hardened Genesis v0.6 checkpoint loader.

Exercises the INTEGRATED genesis_model.checkpoint module via synthetic,
arch-identical fixtures. No real checkpoints, weight files, or absolute machine
paths are committed: every checkpoint/tokenizer is built in-memory on an
isolated temp dir, and the trusted registry is injected per-test via the
GENESIS_TOKENIZER_REGISTRY env knob (the production in-package registry is never
mutated by these tests).

Canonical tokenizer digest mirrors the loader exactly:
    sha256(json.dumps(json.loads(file), sort_keys=True, ensure_ascii=False))
"""
from __future__ import annotations

import json
import os
import sys
from pathlib import Path

import pytest
import torch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
os.environ.pop("GENESIS_TOKENIZER_REGISTRY", None)

from genesis_model.checkpoint import (  # noqa: E402
    TokenizerCompatibilityError,
    _canonical_tokenizer_sha,
    _file_sha256,
    load_checkpoint,
)
from genesis_model.config import ModelConfig  # noqa: E402
from genesis_model.model import GenesisModel  # noqa: E402
from genesis_model.tokenizer import (  # noqa: E402
    Tokenizer,
    SPECIAL_TOKENS,
    SPECIAL_BASE,
    BOS,
    USER,
)

# --- synthetic, arch-identical fixtures (no real artifacts) ---
# 4 merges -> vocab 272 + 4 = 276. Arch mirrors the verified legacy Item-A
# (d_model=128, n_layers=4, n_heads=4, d_ff=344) at a CPU-friendly scale.
MERGES = [(0, 1), (2, 3), (4, 5), (6, 7)]
VOCAB = SPECIAL_BASE + len(SPECIAL_TOKENS) + len(MERGES)
ARCH = {"d_model": 128, "n_layers": 4, "n_heads": 4, "d_ff": 344}
RAW_CFG = {"n_layer": 4, "n_head": 4, "n_embd": 128, "block_size": 256,
           "vocab_size": VOCAB, "d_ff": 344}
META_BASE = "buggy_sft_tool_compact_seq256"


def build_model():
    cfg = ModelConfig(
        vocab_size=VOCAB, d_model=128, n_layers=4, n_heads=4, n_kv_heads=1,
        d_ff=344, max_seq_len=256, rope_theta=10000.0, tie_embeddings=True,
        norm_eps=1e-6,
    )
    return GenesisModel(cfg), cfg


def tok(merges=None):
    return Tokenizer(merges=list(MERGES if merges is None else merges))


def write_tok(path, tokenizer):
    path.write_text(json.dumps(tokenizer.to_dict(), ensure_ascii=False), encoding="utf-8")


def cansha(tokenizer, tmp_path, name="_t.json"):
    p = tmp_path / name
    write_tok(p, tokenizer)
    return _canonical_tokenizer_sha(p)


def save_legacy(path, sd, tokenizer=None, meta_base=META_BASE, extra_meta=None,
                steps=1200, fmt="genesis-ckpt-v1"):
    meta = {"base": meta_base, "trained": "re_sft_from_buggy_shiftfix",
            "steps": steps, "shift_fixed": True, "tool_call_acc": 0.0}
    if extra_meta:
        meta.update(extra_meta)
    payload = {"format_version": fmt, "config": dict(RAW_CFG), "state_dict": sd, "meta": meta}
    if tokenizer is not None:
        write_tok(path.parent / "tokenizer.json", tokenizer)
    torch.save(payload, path)
    return path


def make_legacy(tmp_path, tokenizer=None, **kw):
    model, _ = build_model()
    sd = {k: v.clone() for k, v in model.state_dict().items()}
    return save_legacy(tmp_path / "genesis.pt", sd, tokenizer=tokenizer, **kw)


def modern_payload(model, tokenizer, meta_base="modern", steps=1200):
    return {
        "format_version": 1,
        "config": model.config.to_dict(),
        "tokenizer": tokenizer.to_dict(),
        "state_dict": {k: v.clone() for k, v in model.state_dict().items()},
        "meta": {"base": meta_base, "trained": "native", "steps": steps},
    }


def entry(canonical, vocab=VOCAB, meta_base=META_BASE, fmt="genesis-ckpt-v1",
          arch=None, include_tok=True):
    e = {
        "format": fmt, "vocab_size": vocab,
        "architecture": dict(arch) if arch else dict(ARCH),
        "parent_checkpoint_sha256": "a498f8d161202e2b" + "0" * 56,
        "special_token_layout": {"count": 16, "id_range": [256, 271], "version": 1},
        "meta_base": meta_base,
    }
    if include_tok:
        e["tokenizer_canonical_sha256"] = canonical
    return e


def register(reg, ckpt_path, **entry_kwargs):
    data = json.loads(reg.read_text())
    data["by_checkpoint_sha256"][_file_sha256(ckpt_path)] = entry(**entry_kwargs)
    reg.write_text(json.dumps(data, sort_keys=True))


@pytest.fixture
def env(tmp_path, monkeypatch):
    """Inject an empty trusted-registry file; tests opt in per case.

    GENESIS_TOKENIZER_REGISTRY selects the registry LOCATION (a deployment
    config knob). Trust still comes from the registry CONTENTS.
    """
    reg = tmp_path / "tokenizer_provenance.json"
    monkeypatch.setenv("GENESIS_TOKENIZER_REGISTRY", str(reg))
    reg.write_text(json.dumps({"registry_version": 1, "by_checkpoint_sha256": {}}))
    return reg


# --- success cases ---

def test_modern_embedded_tokenizer(tmp_path):
    model, _ = build_model()
    t = tok()
    torch.save(modern_payload(model, t), tmp_path / "genesis.pt")
    m, tok_loaded, _meta = load_checkpoint(tmp_path / "genesis.pt")
    assert tok_loaded.vocab_size == VOCAB
    assert (tmp_path / "genesis.pt").is_file()


def test_legacy_with_verified_tokenizer(tmp_path, env):
    t = tok()
    path = make_legacy(tmp_path, tokenizer=t)
    register(env, path, canonical=cansha(t, tmp_path))
    model, tok_loaded, _meta = load_checkpoint(path)
    assert model.config.vocab_size == VOCAB


def test_no_fallback_training_invoked(tmp_path, env, monkeypatch):
    import genesis_model.checkpoint as C

    def boom(_v):
        raise AssertionError("_train_fallback_tokenizer must never be called")
    monkeypatch.setattr(C, "_train_fallback_tokenizer", boom)
    path = make_legacy(tmp_path, tokenizer=None)
    register(env, path, canonical="0" * 64)
    with pytest.raises(TokenizerCompatibilityError, match="co-located tokenizer"):
        load_checkpoint(path)


def test_weights_byte_stable(tmp_path, env):
    t = tok()
    path = make_legacy(tmp_path, tokenizer=t)
    register(env, path, canonical=cansha(t, tmp_path))
    before = _file_sha256(path)
    load_checkpoint(path)
    after = _file_sha256(path)
    assert before == after


def test_short_deterministic_inference(tmp_path, env):
    t = tok()
    path = make_legacy(tmp_path, tokenizer=t)
    register(env, path, canonical=cansha(t, tmp_path))
    model, _tok, _meta = load_checkpoint(path)
    with torch.no_grad():
        out1 = model.generate_ids([BOS, USER], max_new_tokens=4, temperature=0.0,
                                  top_k=None, top_p=1.0)
        out2 = model.generate_ids([BOS, USER], max_new_tokens=4, temperature=0.0,
                                  top_k=None, top_p=1.0)
    # deterministic greedy decoding: bounded length, two runs identical
    assert 1 <= len(out1) <= 4
    assert out1 == out2


# --- negative cases (directive §4) ---

def test_missing_co_located_tokenizer(tmp_path, env):
    path = make_legacy(tmp_path, tokenizer=None)
    register(env, path, canonical="0" * 64)
    with pytest.raises(TokenizerCompatibilityError, match="co-located tokenizer"):
        load_checkpoint(path)


def test_wrong_vocab_272_no_merges(tmp_path, env):
    # checkpoint config expects vocab 276 (4 merges) but a 0-merge (vocab 272)
    # tokenizer is co-located -> vocab gate rejects.
    path = make_legacy(tmp_path, tokenizer=tok(merges=[]))
    register(env, path, canonical=cansha(tok(merges=[]), tmp_path, "_b.json"))
    with pytest.raises(TokenizerCompatibilityError, match="tokenizer vocab"):
        load_checkpoint(path)


def test_replaced_companion_tokenizer(tmp_path, env):
    a, b = tok(), tok(merges=[(1, 0), (3, 2), (5, 4), (7, 6)])  # same vocab, different merges
    path = make_legacy(tmp_path, tokenizer=b)
    register(env, path, canonical=cansha(a, tmp_path))  # registered for tokenizer A
    with pytest.raises(TokenizerCompatibilityError, match="canonical sha does not match"):
        load_checkpoint(path)


def test_tampered_merges_vocab_preserved(tmp_path, env):
    base = tok()
    swapped = tok(merges=[(0, 1), (4, 5), (2, 3), (6, 7)])  # swap order, same vocab
    path = make_legacy(tmp_path, tokenizer=swapped)
    register(env, path, canonical=cansha(base, tmp_path))
    with pytest.raises(TokenizerCompatibilityError, match="canonical sha does not match"):
        load_checkpoint(path)


def test_checkpoint_digest_mismatch(tmp_path, env):
    t = tok()
    path = make_legacy(tmp_path, tokenizer=t)
    register(env, path, canonical=cansha(t, tmp_path))
    # tamper a weight -> file sha no longer matches the registered key
    model, _ = build_model()
    sd = {k: v.clone() for k, v in model.state_dict().items()}
    k0 = list(sd.keys())[0]
    sd[k0] = sd[k0] + 1
    save_legacy(path, sd, tokenizer=t)
    with pytest.raises(TokenizerCompatibilityError, match="no trusted provenance record"):
        load_checkpoint(path)


def test_modified_meta_base_rejected(tmp_path, env):
    t = tok()
    path = make_legacy(tmp_path, tokenizer=t, meta_base="sneaky_forged_base")
    register(env, path, canonical=cansha(t, tmp_path), meta_base=META_BASE)
    with pytest.raises(TokenizerCompatibilityError, match="meta.base"):
        load_checkpoint(path)


def test_different_checkpoint_same_meta_base(tmp_path, env):
    a = tok()
    path_a = make_legacy(tmp_path, tokenizer=a)
    register(env, path_a, canonical=cansha(a, tmp_path))
    # distinct checkpoint (different weights) but SAME meta.base, unregistered
    model, _ = build_model()
    sd = {k: v.clone() for k, v in model.state_dict().items()}
    sd[list(sd.keys())[0]] = sd[list(sd.keys())[0]] + 1
    path_b = tmp_path / "other.pt"
    save_legacy(path_b, sd, tokenizer=tok(merges=MERGES), meta_base=META_BASE)
    with pytest.raises(TokenizerCompatibilityError, match="no trusted provenance record"):
        load_checkpoint(path_b)


def test_missing_registry_entry(tmp_path, env):
    # env fixture leaves the registry empty -> fail-closed reject even with a
    # valid, matching tokenizer co-located.
    path = make_legacy(tmp_path, tokenizer=tok())
    with pytest.raises(TokenizerCompatibilityError, match="no trusted provenance record"):
        load_checkpoint(path)


def test_wrong_registry_fingerprint(tmp_path, env):
    t = tok()
    path = make_legacy(tmp_path, tokenizer=t)
    register(env, path, canonical="f" * 64)
    with pytest.raises(TokenizerCompatibilityError, match="canonical sha does not match"):
        load_checkpoint(path)


def test_wrong_architecture(tmp_path, env):
    t = tok()
    path = make_legacy(tmp_path, tokenizer=t)
    register(env, path, canonical=cansha(t, tmp_path),
             arch={"d_model": 999, "n_layers": 4, "n_heads": 4, "d_ff": 344})
    with pytest.raises(TokenizerCompatibilityError, match="registry architecture"):
        load_checkpoint(path)


def test_wrong_format_in_registry(tmp_path, env):
    t = tok()
    path = make_legacy(tmp_path, tokenizer=t)
    register(env, path, canonical=cansha(t, tmp_path), fmt="genesis-ckpt-v2")
    with pytest.raises(TokenizerCompatibilityError, match="registry format"):
        load_checkpoint(path)


def test_missing_trusted_fingerprint_field(tmp_path, env):
    t = tok()
    path = make_legacy(tmp_path, tokenizer=t)
    register(env, path, canonical=cansha(t, tmp_path), include_tok=False)
    with pytest.raises(TokenizerCompatibilityError, match="no tokenizer_canonical_sha256"):
        load_checkpoint(path)


def test_malformed_registry(tmp_path, env):
    t = tok()
    path = make_legacy(tmp_path, tokenizer=t)
    register(env, path, canonical=cansha(t, tmp_path))
    env.write_text("{ this is not valid json")
    with pytest.raises(TokenizerCompatibilityError, match="malformed"):
        load_checkpoint(path)


def test_unsupported_262_native_lineage(tmp_path, env):
    # The 262-token native GenesisSeed lineage has no tokenizer.json and must
    # NOT be routed into the 4096-token BPE loader (no fake compatibility layer).
    payload = {"format_version": "genesis-native-seed",
               "config": {"vocab_size": 262}, "state_dict": {}, "meta": {"base": "native_seed"}}
    torch.save(payload, tmp_path / "native.pt")
    with pytest.raises(ValueError, match="unsupported checkpoint format"):
        load_checkpoint(tmp_path / "native.pt")


def test_corrupt_tokenizer_json(tmp_path, env):
    path = make_legacy(tmp_path, tokenizer=None)
    register(env, path, canonical="0" * 64)
    (path.parent / "tokenizer.json").write_text("{ not valid json {{{", encoding="utf-8")
    with pytest.raises(TokenizerCompatibilityError, match="unrecognized special-token layout"):
        load_checkpoint(path)


def test_bad_specials_rejected(tmp_path, env):
    path = make_legacy(tmp_path, tokenizer=None)
    register(env, path, canonical="0" * 64)
    bad = tok().to_dict()
    bad["special_tokens"] = list(reversed(bad["special_tokens"]))
    (path.parent / "tokenizer.json").write_text(
        json.dumps(bad, ensure_ascii=False), encoding="utf-8")
    with pytest.raises(TokenizerCompatibilityError, match="unrecognized special-token layout"):
        load_checkpoint(path)


def test_symlink_escape_rejected(tmp_path, env):
    model, _ = build_model()
    sd = {k: v.clone() for k, v in model.state_dict().items()}
    path = save_legacy(tmp_path / "genesis.pt", sd, tokenizer=None)
    register(env, path, canonical="0" * 64)
    real = tmp_path / "real_tokenizer.json"
    write_tok(real, tok())
    link = path.parent / "tokenizer.json"
    try:
        link.symlink_to(real)
    except (OSError, NotImplementedError):
        pytest.skip("symlink creation not supported on this platform")
    with pytest.raises(TokenizerCompatibilityError, match="symlink"):
        load_checkpoint(path)


def test_meta_tokenizer_path_not_honored(tmp_path, env):
    # A malicious meta.tokenizer_path must NOT redirect the loader; it always
    # reads the co-located tokenizer.json. Success here proves the meta path is
    # ignored (no path-escape / traversal).
    t = tok()
    path = make_legacy(tmp_path, tokenizer=t,
                       extra_meta={"tokenizer_path": "/c/Windows/System32/evil_tokenizer.json"})
    register(env, path, canonical=cansha(t, tmp_path))
    _m, tok_loaded, _meta = load_checkpoint(path)
    assert tok_loaded.vocab_size == VOCAB
