"""Convert exported Genesis weights.npz -> weights.bin for C++ runtime.

Binary format:
  [uint32_t num_tensors]
  [uint32_t name_len][name_bytes]
  [uint32_t dtype] [uint32_t ndim] [uint32_t dim0] ... [float32 data...]
  ... repeat for each tensor ...
"""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

try:
    import numpy as np
except ImportError:
    print("numpy required: pip install numpy")
    raise SystemExit(1)


def convert(npz_path: Path, bin_path: Path) -> None:
    data = np.load(npz_path, allow_pickle=False)
    names = sorted(data.files)
    with bin_path.open("wb") as out:
        out.write(struct.pack("<I", len(names)))
        for name in names:
            arr = data[name]
            name_bytes = name.encode("utf-8")
            out.write(struct.pack("<I", len(name_bytes)))
            out.write(name_bytes)
            out.write(struct.pack("<I", 0))  # dtype: 0 = float32
            out.write(struct.pack("<I", arr.ndim))
            for dim in arr.shape:
                out.write(struct.pack("<I", dim))
            out.write(arr.astype("<f4").tobytes())
    print(f"Converted {len(names)} tensors to {bin_path}")


def main() -> int:
    ap = argparse.ArgumentParser(description="Convert Genesis weights.npz -> weights.bin")
    ap.add_argument("--input", required=True, help="Path to weights.npz")
    ap.add_argument("--output", required=True, help="Path to weights.bin")
    args = ap.parse_args()
    convert(Path(args.input), Path(args.output))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
