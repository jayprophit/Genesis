#!/usr/bin/env python3
with open('test_canonical_evaluator.py', 'r') as f:
    content = f.read()

# Find the test method start
idx = content.find('def test_production_output_matches_direct_greedy_inference(self):')

# Find what's at the prefix = line
prefix_line_start = content.find('prefix = ', idx)
prefix_line_end = content.find('\n', prefix_line_start)
prefix_line = content[prefix_line_start:prefix_line_end]
print("prefix line repr:", repr(prefix_line))
print("Has backslash:", '\\' in prefix_line)
print("Has f quote:", 'f"' in prefix_line)
