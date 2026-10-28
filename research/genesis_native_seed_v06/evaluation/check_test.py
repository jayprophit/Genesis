#!/usr/bin/env python3
with open('test_canonical_evaluator.py', 'r') as f:
    content = f.read()

# Find the test method
idx = content.find('def test_production_output_matches_direct_greedy_inference(self):')
# Find where the test method ends (next 'class ' or 'def setUp')
end_idx = content.find('class TestEvaluatorImmutability', idx)
if end_idx == -1:
    end_idx = len(content)

# Extract the section
section = content[idx:end_idx]

# Check what we have
print("Section contains f\":", 'f"' in section[:1000])
print("Section contains expected_suffix:", 'expected_suffix' in section)
print("Section contains action_word:", 'action_word' in section)
