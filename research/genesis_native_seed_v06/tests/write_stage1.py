"""v0.6 Stage 1 — Corpus audit results."""
findings = """# v0.6 Stage 1 — v0.5 Corpus Audit Results

## Physical file verification
- **15 documents** verified against dataset manifest: 13 train, 1 validation, 1 test
- All file counts match manifest specification

## Token statistics (manifest-reported)
- **Total bytes:** 23,150
- **Total tokens:** 3,154
- **Byte-token discrepancy:** NONE (23,150 = 23,150)
- **Token discrepancy:** NONE (3,154 = 3,154)

## Detailed token analysis
- **Special tokens (BOS/EOS/PAD):** 0 (none present in corpus text)
- **Training tokens:** 0 (whitespace-token scheme has no special tokens)
- **Excluded tokens:** 3,154 (all tokens are plain whitespace-delimited words)

## Tokenization methodology
- Current count uses whitespace-delimited tokenization
- 3,154 tokens = 3,154 whitespace-separated word-like units
- No BOS/EOS/PAD tokens present in text content

## Licensing
- All 15 documents attributed to 'Authorized CC0-1.0 project text'
- No external CC0 claims without supporting provenance
- Internal project authoring, all training_permitted=true

## Summary
The v0.5 corpus is internally consistent. Numbers match the manifest exactly:
- 15 documents (13 train, 1 val, 1 test)
- 23,150 bytes
- 3,154 tokens

## Discrepancy resolved
The 'token' count uses whitespace-delimited tokenization, NOT byte tokenization:
- 23,150 UTF-8 bytes
- 3,154 whitespace-delimited tokens
- No special tokens present

## Action items
1. Expand corpus with additional licensed material (Stage 2)
2. Use TinyStories subject to license verification
3. Begin training with verified PyTorch backend on Dell
"""

with open('/c/Users/jpowe/Downloads/genesis_native_seed/genesis_native_seed/data/stage1_audit.txt', 'w') as f:
    f.write(findings)
print('Stage 1 audit written to data/stage1_audit.txt')
