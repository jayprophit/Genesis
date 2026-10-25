"""Extend a Genesis-Model checkpoint's context by rebuilding the RoPE buffers at a
longer max_seq_len and copying the position-independent weights across.

Why this exists: train() resumes the checkpoint's baked-in context (256) and never
grows it, so the agent's ~1,074-token prompt cannot fit. RoPE here is a *computed*
function of position (build_rope: inv_freq from theta, cos/sin buffers), NOT a learned
position-embedding table. So extending context needs no new learned parameters: build
the model at max_seq_len=1024 and every weight (embeddings, transformer blocks, lm_head)
copies across unchanged; only the cos/sin buffers are recomputed for the longer range.
Positions 256-1023 are an extrapolation of the same formula, later adapted by continued
training (SFT-b).

Identity rule: this transforms the existing from-scratch checkpoint; it borrows no
external weights.

Usage: python scripts/extend_context.py --resume runs/sft_a_tool_seq256/genesis.pt
                                         --out runs/sft_ext_1024/genesis.pt
                                         --max-seq-len 1024
"""
from __future__ import annotations
import argparse
import copy
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from genesis_model.checkpoint import load_checkpoint, save_checkpoint, CheckpointMeta  # noqa: E402
from genesis_model.config import ModelConfig  # noqa: E402
from genesis_model.model import GenesisModel  # noqa: E402


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--resume", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--max-seq-len", type=int, default=1024)
    args = ap.parse_args()

    src_model, tokenizer, meta = load_checkpoint(args.resume, map_location="cpu")
    src_cfg = src_model.config
    print(f"source: max_seq_len={src_cfg.max_seq_len} d_model={src_cfg.d_model} "
          f"n_layers={src_cfg.n_layers} n_heads={src_cfg.n_heads} "
          f"n_kv_heads={getattr(src_cfg, 'n_kv_heads', None)} vocab={src_cfg.vocab_size} "
          f"rope_theta={getattr(src_cfg, 'rope_theta', None)}", flush=True)

    dst_cfg = ModelConfig.from_dict({**src_cfg.to_dict(), "max_seq_len": args.max_seq_len})
    dst_model = GenesisModel(dst_cfg)

    src_sd = src_model.state_dict()
    dst_sd = dst_model.state_dict()
    # Rope buffers are non-persistent and recomputed; skip them in the copy.
    skip = {"rope_cos", "rope_sin"}
    copied, missing, unexpected = 0, [], []
    for key, value in src_sd.items():
        if key in skip:
            continue
        if key not in dst_sd:
            unexpected.append(key); continue
        if dst_sd[key].shape != value.shape:
            unexpected.append(f"{key} shape {tuple(dst_sd[key].shape)} != {tuple(value.shape)}"); continue
        dst_sd[key] = value
        copied += 1
    for key in dst_sd:
        if key not in skip and key not in src_sd:
            missing.append(key)

    print(f"copied params: {copied} | missing(in dst, absent src): {missing} | "
          f"unexpected(in src, absent dst): {unexpected}", flush=True)
    if missing or unexpected:
        raise SystemExit("weight copy mismatch; aborting rather than save a broken checkpoint")

    dst_model.load_state_dict(dst_sd)
    # Sanity: a forward pass at the new context must not raise.
    import torch
    x = torch.randint(0, tokenizer.vocab_size, (1, args.max_seq_len))
    with torch.no_grad():
        out = dst_model(x)
    print(f"forward at seq={args.max_seq_len}: ok, output shape {tuple(out.shape)}", flush=True)

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    new_meta = CheckpointMeta(
        steps=meta.steps,
        tokens_seen=meta.tokens_seen,
        corpus_sha256=meta.corpus_sha256,
        training_history=list(meta.training_history),
        notes=f"context-extended to {args.max_seq_len} from {src_cfg.max_seq_len} (RoPE buffer rebuild + weight copy); base={args.resume}",
    )
    save_checkpoint(out_path, dst_model, tokenizer, new_meta, optimizer=None)
    print(f"saved extended checkpoint: {out_path}", flush=True)


if __name__ == "__main__":
    main()
