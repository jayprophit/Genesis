
import os
import sys
sys.path.insert(0, r"C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed")
import torch
import torch.nn.functional as F
import time
from genesis_seed.model import GenesisSeed, GenesisConfig, SeedConfig, BOS, EOS, PAD, encode

# Configuration
SEED = 42
TOTAL_STEPS = 800
LOG_EVERY = 100
LR = 0.0015
WEIGHT_DECAY = 0.01
DEVICE = 'cpu'
CHECKPOINT_DIR = r"C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed\diagnostics\level3_composition\checkpoints"

os.makedirs(CHECKPOINT_DIR, exist_ok=True)

# Training sentences (12 train, 4 held-out)
TRAIN_SENTENCES = [
    "The red cat sleeps.",
    "The red bird sings.",
    "The red fish swims.",
    "The blue cat sleeps.",
    "The blue dog runs.",
    "The blue fish swims.",
    "The green cat sleeps.",
    "The green dog runs.",
    "The green bird sings.",
    "The white dog runs.",
    "The white bird sings.",
    "The white fish swims.",
]

# Full vocabulary
ALL_COLORS = ['red', 'blue', 'green', 'white']
ANIMALS = ['cat', 'dog', 'bird', 'fish']
ACTIONS = {
    'cat': 'sleeps',
    'dog': 'runs',
    'bird': 'sings',
    'fish': 'swims'
}
TEMPLATE = "The {color} {animal} {action}."

# Build training sequences
print("=== Level 3 Compositional Generalization — Training ===")
print(f"Training sentences: {len(TRAIN_SENTENCES)}")
print(f"Seed: {SEED}")
print(f"Steps: {TOTAL_STEPS}")
print()

sequences = []
for sentence in TRAIN_SENTENCES:
    seq = [BOS] + [b for b in sentence.encode('utf-8')] + [EOS]
    sequences.append(seq)

print("Training sequences:")
for i, s in enumerate(TRAIN_SENTENCES):
    print(f"  {i+1}: {s!r}")

# Model config
config = GenesisConfig(seed_config=SeedConfig(width=128, heads=4, layers=2, context=128, dropout=0.0))
model = GenesisSeed(config).to(DEVICE)
model.train()
print(f"\nModel initialized (seed={SEED})")
print(f"Parameters: {sum(p.numel() for p in model.parameters()):,}")

optimizer = torch.optim.AdamW(model.parameters(), lr=LR, weight_decay=WEIGHT_DECAY)
history = []
start_time = time.time()

print("\n=== Training Loop ===")
for step in range(TOTAL_STEPS):
    torch.manual_seed(SEED + step)
    
    # Shuffle indices reproducibly
    indices = list(range(len(sequences)))
    # Simple deterministic shuffle (not random.shuffle, for reproducibility)
    for i in range(len(indices)-1, 0, -1):
        j = (step + i) % len(indices)
        indices[i], indices[j] = indices[j], indices[i]
    
    total_step_loss = 0.0
    for idx in indices:
        seq = sequences[idx]
        x = torch.tensor([seq[:-1]], dtype=torch.long, device=DEVICE)
        y = torch.tensor([seq[1:]], dtype=torch.long, device=DEVICE)
        
        logits = model(x)
        loss = F.cross_entropy(logits.squeeze(0), y.squeeze(0), ignore_index=-100)
        total_step_loss += loss.item()
        
        optimizer.zero_grad(set_to_none=True)
        loss.backward()
        torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
        optimizer.step()
    
    avg_loss = total_step_loss / len(sequences)
    history.append({'step': step+1, 'loss': avg_loss})
    
    if step == 0 or (step + 1) % LOG_EVERY == 0 or step == TOTAL_STEPS - 1:
        elapsed = time.time() - start_time
        print(f"Step {step+1:4d} | Loss: {avg_loss:.6f} | Time: {elapsed:.1f}s")

print(f"\nFinal loss: {history[-1]['loss']:.6f}")
print(f"Runtime: {time.time() - start_time:.1f}s")

# Save checkpoint with training history
final_checkpoint = os.path.join(CHECKPOINT_DIR, 'level3_composition_model.pt')
torch.save({
    'config': model.state_config(),
    'state_dict': model.state_dict(),
    'sentences': TRAIN_SENTENCES,
    'seed': SEED,
    'steps': TOTAL_STEPS,
    'training_history': history,
    'run_id': 'level3_composition_experiment_001',
    'run_description': 'Compositional generalization: 12 train, 4 held-out',
}, final_checkpoint)

print(f"\nCheckpoint saved: {final_checkpoint}")
print(f"Checkpoint size: {os.path.getsize(final_checkpoint):,} bytes")
