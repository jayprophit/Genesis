# GENESIS LEVEL 3B REPRODUCIBILITY PROTOCOL

## Pre-Registered Configuration

### Dataset
- 12 training sentences: "The {color} {animal} {action}." (colors: red, blue, green, white; animals: cat->sleeps, dog->runs, bird->sings, fish->swims)
- 4 held-out sentences: "The red dog runs.", "The blue bird sings.", "The green fish swims.", "The white cat sleeps."

### Model
- GenesisSeed architecture (486,790 parameters)
- Width=128, Layers=2, Heads=4, Context=128
- Byte-level tokenizer
- BOS=256, EOS=257, PAD=259

### Training
- Fresh random-initialized model for each seed
- Seed values: 42, 123, 2026
- Optimizer: AdamW (lr=0.0015, weight_decay=0.01)
- Max steps: 800
- CPU-only execution
- Balanced batches (all 12 sentences each step)

### Evaluation
- Greedy decoding (temperature=0)
- Max new tokens: 64
- Distinguishing prefixes: "The {color} {animal}"
- Expected suffix: " {action}."

### Environment
- Python: from .venv-torch-probe/Scripts/python.exe
- PyTorch: 2.14.1+cpu
- Hardware: Intel i7-870 (CPU-only)

## Checkpoints
- Written to: diagnostics/level3b_reproducibility/checkpoints/
- Naming: level3b_seed{seed}_model.pt
- SHA256 hashes recorded for each
- No checkpoint overwrites existing artifacts

## Evaluation Metrics
- Teacher-forced next-byte accuracy
- Full-dataset cross-entropy
- Exact greedy generation from distinguishing prefixes
- Held-out exact match (byte-for-byte)
