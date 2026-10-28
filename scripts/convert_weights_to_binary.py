"""Convert Genesis checkpoint .pt -> weights.bin for C++ runtime.

Binary format:
  [uint32_t num_tensors]
  [uint32_t name_len][name_bytes]
  [uint32_t dtype] [uint32_t ndim] [uint32_t dim0] ... [float32 data...]
  ... repeat for each tensor ...
"""
from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

try:
    import torch
    import numpy as np
except ImportError:
    print("torch and numpy required")
    raise SystemExit(1)


def convert(pt_path: Path, bin_path: Path) -> None:
    ckpt = torch.load(pt_path, map_location="cpu")
    state = ckpt["state_dict"]
    config = ckpt.get("config", {})

    names = sorted(state.keys())
    with bin_path.open("wb") as out:
        out.write(struct.pack("<I", len(names)))
        for name in names:
            tensor = state[name].detach().cpu()
            arr = tensor.numpy().astype("<f4")
            name_bytes = name.encode("utf-8")
            out.write(struct.pack("<I", len(name_bytes)))
            out.write(name_bytes)
            out.write(struct.pack("<I", 0))  # dtype: 0 = float32
            out.write(struct.pack("<I", arr.ndim))
            for dim in arr.shape:
                out.write(struct.pack("<I", dim))
            out.write(arr.tobytes())
    print(f"Converted {len(names)} tensors to {bin_path}")

    # Write a JSON manifest for the C++ runtime
    manifest = {
        "format": "genesis.binary.weights.v1",
        "vocab_size": config.get("vocab_size", 4096),
        "d_model": config.get("d_model", 128),
        "n_layers": config.get("n_layers", 4),
        "n_heads": config.get("n_heads", 4),
        "n_kv_heads": config.get("n_kv_heads", 1),
        "d_ff": config.get("d_ff", 344),
        "max_seq_len": config.get("max_seq_len", 256),
        "rms_norm_eps": config.get("norm_eps", 1e-6),
        "rope_theta": config.get("rope_theta", 10000.0),
        "tie_embeddings": config.get("tie_embeddings", True),
        "weights_file": "weights.bin",
        "parameter_count": int(sum(v.numel() for v in state.values())),
        "tensor_count": len(names),
    }
    manifest_path = bin_path.parent / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Manifest written to {manifest_path}")


def main() -> int:
    ap = argparse.ArgumentParser(description="Convert Genesis checkpoint .pt -> weights.bin")
    ap.add_argument("--input", required=True, help="Path to genesis.pt")
    ap.add_argument("--output", required=True, help="Path to weights.bin")
    args = ap.parse_args()
    convert(Path(args.input), Path(args.output))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
