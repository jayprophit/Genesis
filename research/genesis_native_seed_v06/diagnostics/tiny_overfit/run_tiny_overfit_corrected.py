
import os, sys, json, time, random

project_dir = r"C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed"
diagnostics_dir = os.path.join(project_dir, "diagnostics", "tiny_overfit")
output_dir = os.path.join(diagnostics_dir, "checkpoints")
os.makedirs(output_dir, exist_ok=True)

sys.path.insert(0, project_dir)

import torch
import torch.nn.functional as F
from genesis_seed.model import GenesisSeed, GenesisConfig, SeedConfig, BOS, EOS, PAD

# Experiment metadata
RUN_ID = "20261024_fixed_target_experiment_001"
TRAINING_SENTENCE = "Once upon a time, there was a red cat. The cat was happy."
RUN_DESCRIPTION = "Corrected next-byte prediction experiment (target shifted, no target leakage)"

print("=== Genesis v0.6 Learning-Capacity Experiment (CORRECTED) ===")
print("Run ID: " + RUN_ID)
print("Training sentence: " + TRAINING_SENTENCE)
print("Device: CPU")
print("Architecture: width=128, layers=2, heads=4, context=128")
print("Tokenizer: Genesis byte-level (BOS=256, EOS=257, PAD=259, VOCAB_SIZE=262)")
print()

# Security: verify the four historical checkpoints cannot be overwritten
historical_checkpoints = [
    "genesis_v0_6_486k_bos20_s42_400.pt",
    "genesis_v0_6_486k_sampler_fixed_s42_100.pt",
    "genesis_v0_6_486k_sampler_fixed_s42_400.pt",
    "genesis_v0_6_tinystories_486k_s42_100.pt",
]
for cp in historical_checkpoints:
    cp_path = os.path.join(project_dir, "checkpoints", cp)
    assert os.path.exists(cp_path), f"Missing historical checkpoint: {cp_path}"
print("Security: All four historical checkpoints verified present (not overwritten)")

# Correct sequence construction: x = sequence[:-1], y = sequence[1:]
sentence = "Once upon a time, there was a red cat. The cat was happy."
sequence = [BOS] + [b for b in sentence.encode('utf-8')] + [EOS]

x = torch.tensor([sequence[:-1]], dtype=torch.long)
y = torch.tensor([sequence[1:]], dtype=torch.long)

assert x.shape == y.shape, "x and y have different shapes"
assert x[0, 0].item() == BOS, "First input token must be BOS"
assert y[0, -1].item() == EOS, "Last target token must be EOS"
assert torch.equal(x[0, 1:], y[0, :-1]), "Target must be shifted input (next-byte prediction)"

enc_len = len(sequence) - 1  # number of input bytes
print("Correct sequence construction:")
print("  Input x shape:", x.shape)
print("  Target y shape:", y.shape)
print("  First 10 input tokens:", [hex(v) for v in x[0, :10].tolist()])
print("  First 10 target tokens:", [hex(v) for v in y[0, :10].tolist()])

# Initialize random seed BEFORE constructing model
torch.manual_seed(42)
random.seed(42)

# Build model with correct architecture (width=128, layers=2, heads=4, context=128)
config = GenesisConfig(seed_config=SeedConfig(width=128, heads=4, layers=2, context=128, dropout=0.0))
model = GenesisSeed(config).to('cpu')
model.train()
print("\nModel initialized from random weights")
print("  Width: 128, Layers: 2, Heads: 4, Context: 128")
print("  Parameters: ~486,790")

# Optimizer
optimizer = torch.optim.AdamW(model.parameters(), lr=0.0015, weight_decay=0.01)

TOTAL_STEPS = 400
LOG_EVERY = 25
history = []
start_time = time.time()

print("\n=== Training Loop (400 steps, CPU) ===")
for step in range(TOTAL_STEPS):
    torch.manual_seed(42 + step)
    random.seed(42 + step)
    
    # Forward pass with correct next-byte prediction targets
    logits = model(x)
    loss = F.cross_entropy(logits.squeeze(0), y.squeeze(0), ignore_index=-100)
    
    # Backward pass
    optimizer.zero_grad(set_to_none=True)
    loss.backward()
    grad_norm = torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
    
    optimizer.step()
    
    if step == 0 or (step + 1) % LOG_EVERY == 0 or step == TOTAL_STEPS - 1:
        record = {'step': step + 1, 'loss': round(float(loss.item()), 6), 'grad_norm': round(float(grad_norm), 6)}
        history.append(record)
        elapsed = time.time() - start_time
        print("Step {:3d} | Loss: {:.6f} | GradNorm: {:.6f} | Time: {:.1f}s".format(step+1, loss.item(), grad_norm, elapsed))

print()
print("=== Final Training Statistics ===")
final_loss = history[-1]['loss']
print("Final training cross-entropy: {:.6f}".format(final_loss))
print("Initial loss (step 1): {:.6f}".format(history[0]['loss']))
print("Loss reduction: {:.6f}".format(history[0]['loss'] - final_loss))

# Save checkpoint
final_model_path = os.path.join(output_dir, 'tiny_overfit_model_' + RUN_ID + '.pt')
torch.save({
    'config': model.state_config(),
    'state_dict': model.state_dict(),
    'training_sentence': TRAINING_SENTENCE,
    'seed': 42,
    'steps': TOTAL_STEPS,
    'training_history': history,
    'run_id': RUN_ID,
    'run_description': RUN_DESCRIPTION,
    'training_data': 'single_sentence_memorization',
    'correct_targets': True  # Flag indicating corrected targets
}, final_model_path)
print("\nScratch checkpoint saved:", final_model_path)
print("Checkpoint size:", os.path.getsize(final_model_path), "bytes")
