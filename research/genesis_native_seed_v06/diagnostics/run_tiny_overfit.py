
import os, sys, json, time, random

project_dir = "/c/Users/jpowe/Desktop/Projects/Genesis/experiments/genesis_native_seed"
diagnostics_dir = "/c/Users/jpowe/Desktop/Projects/Genesis/experiments/genesis_native_seed/diagnostics/tiny_overfit"
output_dir = os.path.join(diagnostics_dir, "checkpoints")
os.makedirs(output_dir, exist_ok=True)

print("=== Genesis v0.6 Learning-Capacity Experiment ===")
print("Training sentence: Once upon a time, there was a red cat. The cat was happy.")
print("Device: CPU")
print("Architecture: width=128, layers=2, heads=4, context=128")
print("Vocabulary: Genesis byte-level (262 tokens)")
print()

TRAINING_SENTENCE = "Once upon a time, there was a red cat. The cat was happy."
print("=== Phase 2: Controlled Memorization Experiment ===")
print(f"Training sentence ({len(TRAINING_SENTENCE.encode('utf-8'))} UTF-8 bytes):")
print(f"  {TRAINING_SENTENCE!r}")
print()

enc = [b for b in TRAINING_SENTENCE.encode('utf-8')]
print(f"Training sentence encoded ({len(enc)} bytes):")
print(f"  {[hex(b) for b in enc]}")
print(f"  Decoded: {bytes(enc).decode('utf-8')}")
print()

BOS, EOS, PAD = 256, 257, 259

def next_token_batch(texts, device):
    sequences = [[BOS] + encode(t) + [EOS] for t in texts]
    width = max(len(row) for row in sequences) - 1
    x = torch.full((len(sequences), width), PAD, dtype=torch.long, device=device)
    y = torch.full((len(sequences), width), -100, dtype=torch.long, device=device)
    for i, row in enumerate(sequences):
        x[i, :len(row)-1] = torch.tensor(row[:-1], device=device)
        y[i, :len(row)-1] = torch.tensor(row[1:], device=device)
    return x, y

import torch
import torch.nn.functional as F
from genesis_seed.model import GenesisSeed, GenesisConfig, SeedConfig

config = GenesisConfig(seed_config=SeedConfig(width=128, heads=4, layers=2, context=128, dropout=0.0))
model = GenesisSeed(config).to('cpu')
model.train()

optimizer = torch.optim.AdamW(model.parameters(), lr=0.0015, weight_decay=0.01)

TOTAL_STEPS = 400
LOG_EVERY = 25

print("=== Training Loop (400 steps, CPU) ===")
for step in range(TOTAL_STEPS):
    torch.manual_seed(42 + step)
    random.seed(42 + step)
    
    sequences = [[BOS] + enc + [EOS]]
    width = max(len(row) for row in sequences) - 1
    x = torch.full((1, width), PAD, dtype=torch.long, device='cpu')
    y = torch.full((1, width), -100, dtype=torch.long, device='cpu')
    x[0, :len(enc)] = torch.tensor(enc, dtype=torch.long, device='cpu')
    y[0, :len(enc)] = torch.tensor(enc, dtype=torch.long, device='cpu')
    
    logits = model(x)
    loss = F.cross_entropy(logits.squeeze(0), y.squeeze(0), ignore_index=-100)
    
    optimizer.zero_grad(set_to_none=True)
    loss.backward()
    torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
    optimizer.step()
    
    if step == 0 or (step + 1) % LOG_EVERY == 0 or step == TOTAL_STEPS - 1:
        record = {'step': step + 1, 'loss': round(float(loss.item()), 6)}
        history.append(record)
        print(f"Step {step+1:3d} | Loss: {loss.item():.6f}")

print()
print("=== Final Training Statistics ===")
final_loss = history[-1]['loss']
print(f"Final training cross-entropy: {final_loss:.6f}")
print(f"Initial loss (step 1): {history[0]['loss']:.6f}")
print(f"Loss reduction: {history[0]['loss'] - final_loss:.6f}")

# Save checkpoint
final_model_path = os.path.join(output_dir, 'tiny_overfit_model.pt')
torch.save({
    'config': model.state_config(),
    'state_dict': model.state_dict(),
    'training_sentence': TRAINING_SENTENCE,
    'seed': 42,
    'steps': TOTAL_STEPS,
    'training_history': history,
    'training_data': 'single_sentence_memorization'
}, final_model_path)
print(f"Scratch checkpoint saved: {final_model_path}")
print(f"Checkpoint size: {os.path.getsize(final_model_path):,} bytes")
