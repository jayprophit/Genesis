
import os, sys
sys.path.insert(0, r"C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed")
import torch
from genesis_seed.model import GenesisSeed, GenesisConfig, SeedConfig, BOS, EOS, PAD, encode, decode

checkpoint_path = r"C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed\diagnostics\multi_sentence\checkpoints\multi_sentence_model_20261024_multi_sentence_experiment_001.pt"
saved = torch.load(checkpoint_path, map_location='cpu', weights_only=False)
config = GenesisConfig.from_dict(saved['config'])
model = GenesisSeed(config).to('cpu')
model.load_state_dict(saved['state_dict'], strict=True)
model.eval()
print("Model loaded from checkpoint:", checkpoint_path)

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

print("\n=== Phase 3: Generation Testing ===")

# Helper: greedy generation
def generate_from(prompt_str, max_tokens=64):
    prompt_tokens = [BOS] + encode(prompt_str)
    x_gen = torch.tensor([prompt_tokens], dtype=torch.long)
    generated = []
    for j in range(max_tokens):
        with torch.inference_mode():
            logits = model(x_gen)
            next_token = logits[0, -1].argmax().item()
        generated.append(next_token)
        x_gen = torch.cat([x_gen, torch.tensor([[next_token]], dtype=torch.long)], dim=1)
        if next_token == EOS:
            break
    return generated, decode(generated)

print("\n--- Per-sentence generation tests ---")
for i, sentence in enumerate(SENTENCES):
    seq = [BOS] + [b for b in sentence.encode('utf-8')] + [EOS]
    enc = [b for b in sentence.encode('utf-8')]
    x_target = torch.tensor([seq[:-1]], dtype=torch.long)
    y_target = torch.tensor([seq[1:]], dtype=torch.long)
    
    # Teacher-forced accuracy
    model.eval()
    with torch.inference_mode():
        logits = model(x_target)
        preds = logits.argmax(dim=-1)
        correct = (preds.squeeze(0) == y_target.squeeze(0)).float().mean().item()
        total = y_target.numel()
    
    # Generate from second half of sentence
    split_idx = len(enc) // 2
    prompt = sentence[split_idx:] if split_idx > 0 else sentence
    generated_tokens, generated_text = generate_from(prompt)
    
    remaining = sentence[split_idx:]
    exact_match = (generated_text == sentence)
    prefix_match = remaining in generated_text
    
    print(f"Sentence {i+1}: {sentence}")
    print(f"  Teacher-forced accuracy: {correct*100:.1f}% ({int(correct*total)}/{total})")
    print(f"  Prompt (2nd half): {prompt!r}")
    print(f"  Generated tokens: {generated_tokens}")
    print(f"  Generated text: {generated_text!r}")
    print(f"  Exact match: {exact_match}")
    print(f"  Remaining in output: {prefix_match}")

# BOS-only generation
print("\n--- BOS-only generation (diagnostic) ---")
prompt = ""
generated_tokens, generated_text = generate_from(prompt)
print("BOS-only greedy generation:", repr(generated_text))

# Test generation from the beginning of each sentence
print("\n--- Generation from sentence start (first 3 chars) ---")
for i, sentence in enumerate(SENTENCES):
    prompt = sentence[:3]
    generated_tokens, generated_text = generate_from(prompt)
    match = generated_text.startswith(sentence)
    print(f"Sentence {i+1}: {sentence}")
    print(f"  Prompt: {prompt!r} -> Generated: {generated_text!r}")
    print(f"  Starts with sentence: {match}")

print("\n=== Phase 3 Complete ===")
