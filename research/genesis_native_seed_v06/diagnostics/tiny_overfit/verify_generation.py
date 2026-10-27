
import os, sys
sys.path.insert(0, r"C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed")

import torch
from genesis_seed.model import GenesisSeed, GenesisConfig, SeedConfig, BOS, EOS, PAD, encode, decode

checkpoint_path = r"C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed\diagnostics\tiny_overfit\checkpoints\tiny_overfit_model_20261024_fixed_target_experiment_001.pt"
saved = torch.load(checkpoint_path, map_location='cpu', weights_only=False)
config = GenesisConfig.from_dict(saved['config'])
model = GenesisSeed(config).to('cpu')
model.load_state_dict(saved['state_dict'], strict=True)
model.eval()
print("Model loaded from checkpoint")

sentence = "Once upon a time, there was a red cat. The cat was happy."
enc = [b for b in sentence.encode('utf-8')]
sequence = [BOS] + enc + [EOS]
x = torch.tensor([sequence[:-1]], dtype=torch.long)
y = torch.tensor([sequence[1:]], dtype=torch.long)

# Test 1: Teacher-forced accuracy
print("\n=== Test 1: Teacher-forced next-byte prediction accuracy ===")
model.eval()
with torch.inference_mode():
    logits = model(x)
    preds = logits.argmax(dim=-1)
    correct = (preds.squeeze(0) == y.squeeze(0)).float().mean().item()
    total = y.numel()
print("Teacher-forced accuracy: {:.2f}% ({}/{})".format(correct*100, int(correct*total), total))

# Test 2: Generation from BOS-only
print("\n=== Test 2: Generation from BOS-only prompt ===")
prompt = ""
prompt_tokens = [BOS] + encode(prompt)
x_gen = torch.tensor([prompt_tokens], dtype=torch.long)
generated = []
for i in range(64):
    with torch.inference_mode():
        logits = model(x_gen)
        next_token = logits[0, -1].argmax().item()
    generated.append(next_token)
    x_gen = torch.cat([x_gen, torch.tensor([[next_token]], dtype=torch.long)], dim=1)
    if next_token == EOS:
        break
generated_text = decode(generated)
print("Greedy decoding from BOS:", generated_text)

# Test 3: Generation from prompt
print("\n=== Test 3: Generation from prompt ===")
prompt = "Once upon a time, there was a"
prompt_tokens = [BOS] + encode(prompt)
x_gen = torch.tensor([prompt_tokens], dtype=torch.long)
generated = []
for i in range(64):
    with torch.inference_mode():
        logits = model(x_gen)
        next_token = logits[0, -1].argmax().item()
    generated.append(next_token)
    x_gen = torch.cat([x_gen, torch.tensor([[next_token]], dtype=torch.long)], dim=1)
    if next_token == EOS:
        break
generated_text = decode(generated)
print("Greedy decoding:", generated_text)

# Test 4: Compare
print("\n=== Test 4: Comparison ===")
print("Training sentence:", repr(sentence))
print("BOS prompt generation:", repr(generated_text))
match = generated_text == sentence
print("Exact match with training sentence:", match)

# Test 5: EOS
print("\n=== Test 5: EOS behavior ===")
last_token = sentence.encode('utf-8')[-1]
print("Final training byte:", hex(last_token))
print("EOS token:", hex(EOS))

print("\n=== Phase 3 Complete ===")
