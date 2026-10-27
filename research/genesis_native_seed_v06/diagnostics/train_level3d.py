import sys
sys.path.insert(0, r'C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed')
import os
import time
import torch
import torch.nn.functional as F
from genesis_seed.model import GenesisSeed, GenesisConfig, SeedConfig, BOS, EOS, PAD, encode, decode
import json

SEED = 42
TOTAL_STEPS = 400
LR = 0.0015
WEIGHT_DECAY = 0.01
DEVICE = 'cpu'
CHECKPOINT_DIR = r'C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed\diagnostics\level3d_controlled\checkpoints'
os.makedirs(CHECKPOINT_DIR, exist_ok=True)

manifest_path = r'C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed\diagnostics\level3d_controlled\dataset_manifest.json'
with open(manifest_path, 'r') as f:
    manifest = json.load(f)

TRAIN_SENTENCES = manifest['train']
DEV_SENTENCES = manifest['dev']

print('=== Level 3D Controlled Training ===')
print(f'Training sequences: {len(TRAIN_SENTENCES)}')
print(f'Development sequences: {len(DEV_SENTENCES)}')
print()

def build_sequences(sentences):
    sequences = []
    for sentence in sentences:
        seq = [BOS] + [b for b in sentence.encode("utf-8")] + [EOS]
        # Pad to context length (128)
        seq = seq + [PAD] * (128 - len(seq))
        sequences.append(seq)
    return sequences

train_sequences = build_sequences(TRAIN_SENTENCES)
dev_sequences = build_sequences(DEV_SENTENCES)

def train_model(seed, sampling_mode, train_sequences):
    torch.manual_seed(seed)
    init_rng = torch.Generator()
    init_rng.manual_seed(seed)
    sample_rng = torch.Generator()
    sample_rng.manual_seed(seed + 999)
    
    config = GenesisConfig(seed_config=SeedConfig(width=128, heads=4, layers=2, context=128, dropout=0.0))
    model = GenesisSeed(config).to(DEVICE)
    model.train()
    
    param_count = sum(p.numel() for p in model.parameters())
    print(f'  Seed {seed}, Sampling: {sampling_mode}, Params: {param_count:,}')
    
    optimizer = torch.optim.AdamW(model.parameters(), lr=LR, weight_decay=WEIGHT_DECAY)
    
    history = []
    start_time = time.time()
    
    if sampling_mode == 'balanced':
        indices = list(range(len(train_sequences)))
        indices = torch.tensor(indices, dtype=torch.long)
        # Shuffle with sample_rng
        shuffled = torch.randperm(len(indices), generator=sample_rng).tolist()
        epoch_sequence = [indices[i] for i in shuffled]
        epoch_pos = 0
    
    for step in range(TOTAL_STEPS):
        batch_size = len(train_sequences)
        
        if sampling_mode == 'balanced':
            batch = []
            for i in range(batch_size):
                if epoch_pos >= len(epoch_sequence):
                    shuffled = torch.randperm(len(indices), generator=sample_rng).tolist()
                    epoch_sequence = [indices[i] for i in shuffled]
                    epoch_pos = 0
                idx = epoch_sequence[epoch_pos % len(epoch_sequence)]
                batch.append(train_sequences[idx])
                epoch_pos += 1
        else:
            batch_indices = torch.randint(0, len(train_sequences), (batch_size,), generator=sample_rng)
            batch = [train_sequences[i] for i in batch_indices]
        
        x = torch.tensor([s[:-1] for s in batch], dtype=torch.long, device=DEVICE)
        y = torch.tensor([s[1:] for s in batch], dtype=torch.long, device=DEVICE)
        
        logits = model(x)
        # logits: [batch, seq_len, vocab_size]
        loss = F.cross_entropy(
            logits.reshape(-1, logits.size(-1)),
            y.reshape(-1),
            ignore_index=PAD
        )
        
        optimizer.zero_grad(set_to_none=True)
        loss.backward()
        torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
        optimizer.step()
        
        avg_loss = loss.item()
        history.append({'step': step+1, 'loss': avg_loss})
        
        if step == 0 or (step+1) % 100 == 0 or step == TOTAL_STEPS - 1:
            elapsed = time.time() - start_time
            print(f'   Step {step+1:4d} | Loss: {avg_loss:.6f} | Time: {elapsed:.1f}s')
    
    total_time = time.time() - start_time
    return model, history, total_time

def evaluate_teachers_forced(model, sentences):
    total_correct = 0
    total_targets = 0
    total_loss = 0.0
    for sentence in sentences:
        enc = [b for b in sentence.encode("utf-8")]
        seq = [BOS] + enc + [EOS]
        x = torch.tensor([seq[:-1]], dtype=torch.long)
        y = torch.tensor([seq[1:]], dtype=torch.long)
        with torch.inference_mode():
            logits = model(x)
            loss = F.cross_entropy(logits.squeeze(0), y.squeeze(0), ignore_index=-100)
            preds = logits.argmax(dim=-1)
            correct = (preds.squeeze(0) == y.squeeze(0)).float().sum().item()
            total = y.squeeze(0).numel()
            acc = correct / total
            total_correct += correct
            total_targets += total
            total_loss += loss.item() * total
    return {'total_loss': total_loss, 'total_targets': total_targets, 'overall_accuracy': total_correct / total_targets}

def main():
    seeds = [42, 123]
    modes = ['baseline', 'balanced']
    
    for seed in seeds:
        for mode in modes:
            model, history, train_time = train_model(seed, mode, train_sequences)
            tf_res = evaluate_teachers_forced(model, TRAIN_SENTENCES + DEV_SENTENCES)
            print(f'Seed {seed}, {mode}: TF accuracy={tf_res["overall_accuracy"]:.1%}')
            print(f'Train time: {train_time:.1f}s')
            
            checkpoint_name = f'level3d_seed{seed}_{mode}.pt'
            checkpoint_path = os.path.join(CHECKPOINT_DIR, checkpoint_name)
            torch.save({
                'config': model.state_config(),
                'state_dict': model.state_dict(),
                'sentences': TRAIN_SENTENCES,
                'seed': seed,
                'sampling_mode': mode,
                'steps': TOTAL_STEPS,
            }, checkpoint_path)
            print(f'Checkpoint: {checkpoint_path}')
            print()

if __name__ == '__main__':
    main()
