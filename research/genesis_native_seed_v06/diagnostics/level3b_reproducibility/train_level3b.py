import sys
sys.path.insert(0, r'C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed')
import os
import time
import torch
import torch.nn.functional as F
from genesis_seed.model import GenesisSeed, GenesisConfig, SeedConfig, BOS, EOS, PAD, encode, decode

# Configuration
SEEDS = [42, 123, 2026]
TOTAL_STEPS = 800
LOG_EVERY = 100
LR = 0.0015
WEIGHT_DECAY = 0.01
DEVICE = 'cpu'
CHECKPOINT_DIR = r'C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed\diagnostics\level3b_reproducibility\checkpoints'
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

HELD_OUT_PROMPTS = [
    ("The red dog", " runs."),
    ("The blue bird", " sings."),
    ("The green fish", " swims."),
    ("The white cat", " sleeps."),
]

def train_model(seed, train_sentences, total_steps, lr, weight_decay):
    """Train a fresh model from random initialization."""
    torch.manual_seed(seed)
    
    config = GenesisConfig(seed_config=SeedConfig(width=128, heads=4, layers=2, context=128, dropout=0.0))
    model = GenesisSeed(config).to(DEVICE)
    model.train()
    
    param_count = sum(p.numel() for p in model.parameters())
    print(f'  Model: {param_count:,} parameters')
    
    optimizer = torch.optim.AdamW(model.parameters(), lr=lr, weight_decay=weight_decay)
    
    # Build sequences
    sequences = []
    for sentence in train_sentences:
        seq = [BOS] + [b for b in sentence.encode('utf-8')] + [EOS]
        sequences.append(seq)
    
    history = []
    start_time = time.time()
    
    for step in range(total_steps):
        torch.manual_seed(seed + step)
        
        # Deterministic shuffle of indices
        indices = list(range(len(sequences)))
        for i in range(len(indices)-1, 0, -1):
            j = (step + i) % len(indices)
            indices[i], indices[j] = indices[j], indices[i]
        
        total_loss = 0.0
        for idx in indices:
            seq = sequences[idx]
            x = torch.tensor([seq[:-1]], dtype=torch.long, device=DEVICE)
            y = torch.tensor([seq[1:]], dtype=torch.long, device=DEVICE)
            
            logits = model(x)
            loss = F.cross_entropy(logits.squeeze(0), y.squeeze(0), ignore_index=-100)
            total_loss += loss.item()
            
            optimizer.zero_grad(set_to_none=True)
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
            optimizer.step()
        
        avg_loss = total_loss / len(sequences)
        history.append({'step': step+1, 'loss': avg_loss})
        
        if step == 0 or (step + 1) % LOG_EVERY == 0 or step == total_steps - 1:
            elapsed = time.time() - start_time
            print(f'   Step {step+1:4d} | Loss: {avg_loss:.6f} | Time: {elapsed:.1f}s')
    
    total_time = time.time() - start_time
    
    return model, history, total_time

def evaluate_teachers_forced(model, sentences):
    """Compute teacher-forced accuracy and loss."""
    total_correct = 0
    total_targets = 0
    total_loss = 0.0
    per_sentence = []
    
    for sentence in sentences:
        enc = [b for b in sentence.encode('utf-8')]
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
            per_sentence.append({
                'sentence': sentence,
                'ce': loss.item(),
                'accuracy': acc,
                'correct': correct,
                'total': total
            })
    
    return {
        'total_loss': total_loss,
        'total_targets': total_targets,
        'overall_accuracy': total_correct / total_targets,
        'per_sentence': per_sentence
    }

def generate_greedy(model, prefix, max_new_tokens=64):
    """Generate using greedy decoding from a prefix."""
    prompt_tokens = [BOS] + encode(prefix)
    x_gen = torch.tensor([prompt_tokens], dtype=torch.long)
    generated_tokens = []
    
    with torch.inference_mode():
        for j in range(max_new_tokens):
            logits = model(x_gen)
            next_token = logits[0, -1].argmax().item()
            generated_tokens.append(next_token)
            x_gen = torch.cat([x_gen, torch.tensor([[next_token]], dtype=torch.long)], dim=1)
            if next_token == EOS:
                break
    
    return decode(generated_tokens), generated_tokens

def evaluate_generation(model, distinguishing_prefix, expected_suffix, sentence):
    """Evaluate greedy generation from a distinguishing prefix."""
    generated, tokens = generate_greedy(model, distinguishing_prefix)
    full = distinguishing_prefix + generated
    exact_match = full == sentence
    suffix_exact = generated == expected_suffix
    eos = tokens[-1] == EOS if tokens else False
    
    return {
        'distinguishing_prefix': distinguishing_prefix,
        'expected_suffix': expected_suffix,
        'generated': generated,
        'generated_tokens': tokens,
        'full': full,
        'sentence': sentence,
        'exact_match': exact_match,
        'suffix_exact': suffix_exact,
        'eos': eos
    }

def evaluate_held_out(model, held_out):
    """Evaluate held-out generation tests."""
    results = []
    for prompt, expected_suffix in held_out:
        # Construct distinguishing prefix from the held-out sentence
        parts = prompt.split()
        color = parts[1]
        animal = parts[2]
        distinguishing_prefix = f'The {color} {animal}'
        
        result = evaluate_generation(model, distinguishing_prefix, expected_suffix, prompt + expected_suffix)
        results.append(result)
    return results

def main():
    print('=== Level 3B: Generalization Reproducibility ===')
    print(f'Python: {sys.version}')
    print(f'PyTorch: {torch.__version__}')
    print(f'Hardware: CPU-only')
    print()
    
    all_results = {}
    
    for seed in SEEDS:
        print(f'=== Seed {seed} ===')
        
        # Train the model
        print(f'  Training...')
        model, history, train_time = train_model(seed, TRAIN_SENTENCES, TOTAL_STEPS, LR, WEIGHT_DECAY)
        
        # Teacher-forced evaluation
        print(f'  Teacher-forced evaluation...')
        tf_results = evaluate_teachers_forced(model, TRAIN_SENTENCES)
        
        # Generation evaluation from distinguishing prefixes
        print(f'  Generation evaluation...')
        train_gen_results = []
        for sentence in TRAIN_SENTENCES:
            parts = sentence.split()
            color = parts[1]
            animal = parts[2]
            action = sentence.split('.')[0].split()[-1].rstrip('.')
            distinguishing_prefix = f'The {color} {animal}'
            expected_suffix = f' {action}.'
            
            gen_result = evaluate_generation(model, distinguishing_prefix, expected_suffix, sentence)
            train_gen_results.append(gen_result)
        
        train_exact_matches = sum(1 for r in train_gen_results if r['exact_match'])
        
        # Held-out evaluation
        held_out_results = evaluate_held_out(model, HELD_OUT_PROMPTS)
        held_out_exact_matches = sum(1 for r in held_out_results if r['exact_match'])
        
        # Compute full-dataset losses for held-out
        total_held_loss = 0.0
        total_held_targets = 0
        for sentence, expected_suffix in HELD_OUT_PROMPTS:
            enc = [b for b in (sentence + expected_suffix).encode('utf-8')]
            seq = [BOS] + enc + [EOS]
            x = torch.tensor([seq[:-1]], dtype=torch.long)
            y = torch.tensor([seq[1:]], dtype=torch.long)
            
            with torch.inference_mode():
                logits = model(x)
                loss = F.cross_entropy(logits.squeeze(0), y.squeeze(0), ignore_index=-100)
                total = y.squeeze(0).numel()
                total_held_loss += loss.item() * total
                total_held_targets += total
        
        result = {
            'seed': seed,
            'train_gen_results': train_gen_results,
            'held_out_results': held_out_results,
            'train_exact_matches': train_exact_matches,
            'held_out_exact_matches': held_out_exact_matches,
            'train_tf': tf_results,
            'held_out_tf': {
                'ce': total_held_loss / total_held_targets,
                'targets': total_held_targets
            },
            'training_history': history,
            'train_time': train_time,
        }
        all_results[seed] = result
        
        # Save checkpoint
        checkpoint_path = os.path.join(CHECKPOINT_DIR, f'level3b_seed{seed}_model.pt')
        torch.save({
            'config': model.state_config(),
            'state_dict': model.state_dict(),
            'sentences': TRAIN_SENTENCES,
            'seed': seed,
            'steps': TOTAL_STEPS,
        }, checkpoint_path)
        
        # Print summary
        print(f'  Train exact matches: {train_exact_matches}/12')
        print(f'  Held-out exact matches: {held_out_exact_matches}/4')
        print(f'  Train TF accuracy: {result["train_tf"]["overall_accuracy"]:.1%}')
        print(f'  Train TF CE: {result["train_tf"]["total_loss"]/result["train_tf"]["total_targets"]:.6f}')
        print(f'  Held-out TF CE: {total_held_loss/total_held_targets:.6f}')
        print(f'  Train time: {train_time:.1f}s')
        print(f'  Checkpoint: {checkpoint_path}')
        print()
    
    # Compare with rule baseline
    print('=== Rule Baseline Comparison ===')
    rule_baseline = {
        'cat': 'sleeps',
        'dog': 'runs',
        'bird': 'sings',
        'fish': 'swims'
    }
    train_sentences = TRAIN_SENTENCES
    held_out_sentences = [
        'The red dog runs.',
        'The blue bird sings.',
        'The green fish swims.',
        'The white cat sleeps.',
    ]
    all_sentences = train_sentences + held_out_sentences
    
    rule_correct = 0
    rule_total = len(all_sentences)
    rule_details = []
    for sentence in all_sentences:
        parts = sentence.split()
        color = parts[1]
        animal = parts[2]
        expected_action = rule_baseline[animal]
        expected_full = f'The {color} {animal} {expected_action}.'
        rule_correct += (expected_full == sentence)
        rule_details.append({
            'sentence': sentence,
            'expected': expected_full,
            'match': expected_full == sentence
        })
    
    print(f'Rule baseline: {rule_correct}/{rule_total} exact matches')
    for d in rule_details:
        print(f'  {d["sentence"]!r} -> {d["expected"]!r} [{"PASS" if d["match"] else "FAIL"}]')
    
    print()
    print('=== LEVEL 3B REPRODUCIBILITY REPORT ===')
    print()
    for seed, result in all_results.items():
        print(f'Seed {seed}:')
        print(f'  Train generation exact: {result["train_exact_matches"]}/12')
        print(f'  Held-out exact: {result["held_out_exact_matches"]}/4')
        print(f'  Train TF CE: {result["train_tf"]["total_loss"]/result["train_tf"]["total_targets"]:.6f}')
        print(f'  Train TF accuracy: {result["train_tf"]["overall_accuracy"]:.1%}')
        print(f'  Held-out TF CE: {result["held_out_tf"]["ce"]:.6f}')
        print(f'  Time: {result["train_time"]:.1f}s')
        print()
    
    # Write results summary file
    results_path = r'C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed\diagnostics\level3b_reproducibility\results_summary.txt'
    with open(results_path, 'w') as f:
        f.write('Level 3B Reproducibility Results Summary\n')
        f.write('=' * 50 + '\n\n')
        for seed, result in all_results.items():
            f.write(f'Seed {seed}:\n')
            f.write(f'  Train exact matches: {result["train_exact_matches"]}/12\n')
            f.write(f'  Held-out exact matches: {result["held_out_exact_matches"]}/4\n')
            f.write(f'  Train TF CE: {result["train_tf"]["total_loss"]/result["train_tf"]["total_targets"]:.6f}\n')
            f.write(f'  Train TF accuracy: {result["train_tf"]["overall_accuracy"]:.1%}\n')
            f.write(f'  Held-out TF CE: {result["held_out_tf"]["ce"]:.6f}\n')
            f.write(f'  Train time: {result["train_time"]:.1f}s\n')
            f.write(f'  Checkpoint: {checkpoint_path}\n\n')
    
    print(f'Results summary written to: {results_path}')

if __name__ == '__main__':
    main()
