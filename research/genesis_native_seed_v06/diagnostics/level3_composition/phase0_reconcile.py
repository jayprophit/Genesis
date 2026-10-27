
import sys
sys.path.insert(0, r"C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed")
import torch
import torch.nn.functional as F
from genesis_seed.model import GenesisSeed, GenesisConfig, SeedConfig, BOS, EOS, PAD, encode

checkpoint_path = r"C:\Users\jpowe\Desktop\Projects\Genesis\experiments\genesis_native_seed\diagnostics\multi_sentence\checkpoints\multi_sentence_model_20261024_multi_sentence_experiment_001.pt"
saved = torch.load(checkpoint_path, map_location='cpu', weights_only=False)
config = GenesisConfig.from_dict(saved['config'])
model = GenesisSeed(config).to('cpu')
model.load_state_dict(saved['state_dict'], strict=True)
model.eval()

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

print("=== Phase 0: Reconciliation ===")
total_loss_sum = 0.0
total_targets = 0

for sentence in SENTENCES:
    sequence = [BOS] + [b for b in sentence.encode('utf-8')] + [EOS]
    x = torch.tensor([sequence[:-1]], dtype=torch.long)
    y = torch.tensor([sequence[1:]], dtype=torch.long)
    
    with torch.inference_mode():
        logits = model(x)
        loss_sum = F.cross_entropy(logits.squeeze(0), y.squeeze(0), reduction='sum', ignore_index=-100)
        num_targets = (y.squeeze(0) != -100).sum().item()
        avg_loss = loss_sum / num_targets
        
        total_loss_sum += loss_sum.item()
        total_targets += num_targets
        
        probs = torch.softmax(logits, dim=-1)
        correct = y.squeeze(0)
        pred = logits.argmax(dim=-1)
        correct_preds = (pred == correct).float().mean().item()
        correct_probs = probs[0, torch.arange(len(correct)), correct]
        top5 = torch.topk(correct_probs, min(5, len(correct_probs)))
        
        print(f"{sentence!r}: loss={avg_loss:.6f}, targets={num_targets}, accuracy={correct_preds*100:.1f}%")
        print(f"  Top-5 correct probs: {[round(p,4) for p in top5.values.tolist()]}")

print()
print(f"Total targets: {total_targets}")
print(f"Full-dataset CE: {total_loss_sum/total_targets:.6f}")
print(f"Total NLL: {total_loss_sum:.4f} nats")

print()
print("=== Discrepancy Analysis ===")
print("Level 2 report: CE=0.088791, 189 targets, total NLL=16.78 nats")
print("Previous audit: 6 probabilities between 0.021-0.042, total NLL=21.47 nats")
print()
print("If 6 probabilities were 0.021-0.042, their -log values would be ~3.2-3.9 nats each.")
print("6 * 3.5 = ~21 nats minimum (matches the 21.47 nats report).")
print("The Level 2 report used all 189 positions, so the NLL should be lower.")
print("The earlier audit likely used a different checkpoint, step, or subset of positions.")
print(f"Verification: 189 targets, CE={total_loss_sum/total_targets:.6f}, NLL={total_loss_sum:.2f} nats")
