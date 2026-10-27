"""Export our own trained torch tensors into a verifiable, inference-only NumPy package.

This does not download or import any pretrained weights.  The output is a
portable data format: manifest.json + float32 weights.npz (no Python pickle).
"""
from __future__ import annotations
import argparse
import hashlib
import json
from dataclasses import asdict
from pathlib import Path
import numpy as np
from .model import VOCAB_SIZE
from .train import load_checkpoint


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1048576), b''):
            h.update(block)
    return h.hexdigest()


def export(checkpoint: str, output: str) -> dict:
    path = Path(checkpoint).resolve()
    folder = Path(output).resolve()
    folder.mkdir(parents=True, exist_ok=True)
    model = load_checkpoint(str(path), 'cpu')
    # Use the raw SeedConfig for the native inference manifest, keeping the
    # versioned GenesisConfig (with history/version) in the checkpoint.
    raw_cfg = model.seed_config
    weights = {key: tensor.detach().cpu().numpy().astype(np.float32)
               for key, tensor in model.state_dict().items()}
    archive = folder / 'weights.npz'
    np.savez_compressed(archive, **weights)
    manifest = {
        'format': 'genesis.numpy.inference.v1',
        'origin': 'Genesis original model weights, trained from random initialization',
        'architecture': 'GenesisSeed',
        'vocab_size': VOCAB_SIZE,
        'config': asdict(raw_cfg),
        'parameter_count': int(sum(x.size for x in weights.values())),
        'tensor_count': len(weights),
        'weights_file': 'weights.npz',
        'weights_sha256': sha256(archive),
        'source_checkpoint_sha256': sha256(path),
        'limitations': 'Small research prototype: not a general assistant',
    }
    with (folder / 'manifest.json').open('w', encoding='utf-8') as f:
        json.dump(manifest, f, indent=2, sort_keys=True)
    return manifest


def main() -> None:
    p=argparse.ArgumentParser(description='Export original Genesis weights to a Python-torch-free inference runtime')
    p.add_argument('--checkpoint', required=True)
    p.add_argument('--output', required=True)
    a=p.parse_args()
    print(json.dumps(export(a.checkpoint, a.output), indent=2))

if __name__ == '__main__':
    main()
