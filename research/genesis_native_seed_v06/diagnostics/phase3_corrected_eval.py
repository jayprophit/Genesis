import sys
sys.path.insert(0, r'C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed')
import torch
import torch.nn.functional as F
from genesis_seed.model import GenesisSeed, GenesisConfig, SeedConfig, BOS, EOS, PAD, encode, decode

checkpoint_path = r'C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed\diagnostics\level3_composition\checkpoints\level3_composition_model.pt'
saved = torch.load(checkpoint_path, map_location='cpu', weights_only=False)
config = GenesisConfig.from_dict(saved['config'])
model = GenesisSeed(config).to('cpu')
model.load_state_dict(saved['state_dict'], strict=True)
model.eval()

TRAIN_SENTENCES = [
    'The red cat sleeps.',
    'The red bird sings.',
    'The red fish swims.',
    'The blue cat sleeps.',
    'The blue dog runs.',
    'The blue fish swims.',
    'The green cat sleeps.',
    'The green dog runs.',
    'The green bird sings.',
    'The white dog runs.',
    'The white bird sings.',
    'The white fish swims.',
]

held_out = [
    ('The red dog', ' runs.'),
    ('The blue bird', ' sings.'),
    ('The green fish', ' swims.'),
    ('The white cat', ' sleeps.'),
]

# Phase 2: Training-set generation tests with distinguishing prefix
print('=== Phase 2: Training-Set Generation Tests (distinguishing prefix) ===')
print()
print('Prefix: "The {color} {animal}"')
print('Expected suffix: " {action}."')
print()

for sentence in TRAIN_SENTENCES:
    parts = sentence.split()
    color = parts[1]
    animal = parts[2]
    action = sentence.split('.')[0].split()[-1].rstrip('.')
    
    prefix = f'The {color} {animal}'
    expected_suffix = f' {action}.'
    
    prompt_tokens = [BOS] + encode(prefix)
    x_gen = torch.tensor([prompt_tokens], dtype=torch.long)
    generated = []
    with torch.inference_mode():
        for j in range(64):
            logits = model(x_gen)
            next_token = logits[0, -1].argmax().item()
            generated.append(next_token)
            x_gen = torch.cat([x_gen, torch.tensor([[next_token]], dtype=torch.long)], dim=1)
            if next_token == EOS:
                break
    
    gen_text = decode(generated)
    if len(gen_text) >= len(prefix):
        actual_suffix = gen_text[len(prefix):]
    else:
        actual_suffix = gen_text
    
    full = prefix + actual_suffix
    exact_match = 'PASS' if full == sentence else 'FAIL'
    
    print(f'Sentence: {sentence!r}')
    print(f'  Prefix: {prefix!r}')
    print(f'  Expected suffix: {expected_suffix!r}')
    print(f'  Actual suffix: {actual_suffix!r}')
    print(f'  Full: {full!r}')
    print(f'  Result: {exact_match}')
    print()

print('=== Phase 3: Held-Out Tests (Original Four) ===')
print()

print('=== Phase 4: Full-Dataset Metrics ===')
print()

print('--- Training Data (12 sentences) ---')
total_train_loss = 0.0
total_train_targets = 0
for sentence in TRAIN_SENTENCES:
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
    
    total_train_loss += loss.item()
    total_train_targets += total
    
    print(f'{sentence!r}: CE={loss.item():.6f}, accuracy={acc*100:.1f}% ({correct}/{total})')

print()
print(f'Full training CE: {total_train_loss/total_train_targets:.6f}')
print(f'Full training accuracy: {total_train_targets}')
print(f'Total target positions: {total_train_targets}')
print(f'Average per-sentence loss: {total_train_loss/len(TRAIN_SENTENCES):.6f}')
print()

print('--- Held-Out Data (4 sentences) ---')
total_held_loss = 0.0
total_held_targets = 0
for sentence, expected_suffix in held_out:
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
    
    total_held_loss += loss.item()
    total_held_targets += total
    
    print(f'{sentence!r}: CE={loss.item():.6f}, accuracy={acc*100:.1f}% ({correct}/{total})')

print()
print(f'Full held-out CE: {total_held_loss/total_held_targets:.6f}')
print(f'Total held-out target positions: {total_held_targets}')
