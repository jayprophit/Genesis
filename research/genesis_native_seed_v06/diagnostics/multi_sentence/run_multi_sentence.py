
import os, sys, time, random
sys.path.insert(0, r"C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed")
project_dir = r"C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed"
diagnostics_dir = os.path.join(project_dir, "diagnostics", "multi_sentence")
output_dir = os.path.join(diagnostics_dir, "checkpoints")
os.makedirs(output_dir, exist_ok=True)
import torch
import torch.nn.functional as F
from genesis_seed.model import GenesisSeed, GenesisConfig, SeedConfig, BOS, EOS, PAD, encode

# Experiment metadata
RUN_ID = "20261024_multi_sentence_experiment_001"
RUN_DESCRIPTION = "Multi-sentence learning: 8 distinct sentences, 400 training steps, CPU only"

# Training sentences (8 distinct, overlapping vocabulary, different subjects/colors/actions)
SENTENCES = [
    "The red cat is happy.",
    "The blue dog is sleepy.",
    "The green bird can fly.",
    "The little boy has a ball.",
    "A yellow tree grows tall.",
    "The dark cat sleeps.",
    "The white dog runs fast.",
    "A small bird sings.",
]

# Training configuration
TOTAL_STEPS = 400
LOG_EVERY = 25
SEED = 42
LR = 0.0015
WEIGHT_DECAY = 0.01
DEVICE = 'cpu'

# Build training sequences
print("=== Genesis v0.6 Multi-Sentence Learning Experiment ===")
print("Run ID: " + RUN_ID)
print("Number of training sentences: " + str(len(SENTENCES)))
print("Architecture: width=128, layers=2, heads=4, context=128")
print("Device: CPU")
print()

# Verify all sentences
print("=== Training Sentences ===")
sequences = []
for i, s in enumerate(SENTENCES):
    seq = [BOS] + [b for b in s.encode('utf-8')] + [EOS]
    sequences.append(seq)
    print("Sentence {}: {} ({} bytes)".format(i+1, s, len(seq)-1))

# Compute statistical summary
total_tokens = sum(len(seq) - 1 for seq in sequences)
exposures_per_step = 8  # 8 examples per step
total_exposures = TOTAL_STEPS * exposures_per_step
total_tokens_processed = total_exposures * max(len(seq) - 1 for seq in sequences)
# More accurate: total_tokens_processed = sum over all steps of sum over examples of (len(seq)-1)
total_train_tokens = sum(len(seq) - 1 for seq in sequences) * TOTAL_STEPS
print("\nTotal training examples: " + str(len(SENTENCES)))
print("Number of exposures per example (400 steps / 8 examples): 400")
print("Training tokens processed: " + str(total_train_tokens))
print("Average tokens per sentence: " + str(sum(len(seq)-1 for seq in sequences) // len(SENTENCES)))

# Create model
torch.manual_seed(SEED)
random.seed(SEED)
config = GenesisConfig(seed_config=SeedConfig(width=128, heads=4, layers=2, context=128, dropout=0.0))
model = GenesisSeed(config).to(DEVICE)
model.train()
print("\nModel initialized from random weights (seed={})".format(SEED))
print("Parameters: ~486,790")

optimizer = torch.optim.AdamW(model.parameters(), lr=LR, weight_decay=WEIGHT_DECAY)
history = []
start_time = time.time()

print("\n=== Training Loop (400 steps, CPU) ===")
for step in range(TOTAL_STEPS):
    torch.manual_seed(SEED + step)
    random.seed(SEED + step)
    
    # Shuffle examples reproducibly
    indices = list(range(len(SENTENCES)))
    random.shuffle(indices)
    
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
    
    avg_step_loss = total_step_loss / len(SENTENCES)
    
    if step == 0 or (step + 1) % LOG_EVERY == 0 or step == TOTAL_STEPS - 1:
        record = {'step': step + 1, 'loss': round(avg_step_loss, 6)}
        history.append(record)
        elapsed = time.time() - start_time
        print("Step {:3d} | Loss: {:.6f} | Time: {:.1f}s".format(step+1, avg_step_loss, elapsed))

print("\n=== Final Training Statistics ===")
final_loss = history[-1]['loss']
print("Final training cross-entropy (avg per example): {:.6f}".format(final_loss))
print("Initial loss (step 1): {:.6f}".format(history[0]['loss']))
print("Loss reduction: {:.6f}".format(history[0]['loss'] - final_loss))
print("Runtime: {:.1f}s".format(time.time() - start_time))

# Save checkpoint
final_model_path = os.path.join(output_dir, 'multi_sentence_model_' + RUN_ID + '.pt')
torch.save({
    'config': model.state_config(),
    'state_dict': model.state_dict(),
    'sentences': SENTENCES,
    'seed': SEED,
    'steps': TOTAL_STEPS,
    'training_history': history,
    'run_id': RUN_ID,
    'run_description': RUN_DESCRIPTION,
    'training_data': 'multi_sentence_data=' + str(len(SENTENCES))
}, final_model_path)
print("\nCheckpoint saved:", final_model_path)
print("Checkpoint size:", os.path.getsize(final_model_path), "bytes")
