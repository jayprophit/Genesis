# Model Card — Genesis `local_1m_gutenberg`

**Model:** Genesis local_1m_gutenberg  
**Version:** 2026-10-25  
**License:** MIT (model weights and code)  
**Training corpus:** Public-domain Project Gutenberg texts (59 files, ~9.99M tokens)  
**Framework:** PyTorch (CPU-first, CUDA-compatible)  
**Architecture:** 12-layer decoder-only transformer, RMSNorm + RoPE + GQA + SwiGLU

## Intended Use

- Research and experimentation with small from-scratch language models
- Agent tool-use prototyping (JSON-structured tool calls)
- Byte-level next-token prediction at context 256

## Not Intended For

- Production conversational AI
- High-accuracy question answering
- Image or speech generation (vision/speech heads are separate research components)

## Training Details

| Parameter | Value |
|-----------|-------|
| Parameters | 1,217,664 |
| Vocabulary size | 4,096 (byte-level BPE) |
| Context length | 256 tokens |
| Training steps | 1,220 |
| Tokens seen | ~9,994,240 |
| Optimizer | AdamW (lr=1e-3, cosine decay) |
| Regularization | Gradient clip 1.0, dropout 0.1 |
| Checkpoint | `runs/local_1m_gutenberg/genesis.pt` |

## Evaluation

### Held-out Text (3 Project Gutenberg files, disjoint from training)

| File | Bytes | Tokens | CE loss | Perplexity | Bits/byte |
|------|-------|--------|---------|------------|-----------|
| pg34566.txt | 49,133 | 24,477 | 4.405 | 81.87 | 2.119 |
| pg5.txt | 46,278 | 23,370 | 4.650 | 104.77 | 2.239 |
| pg753.txt | 40,552 | 20,459 | 4.531 | 92.67 | 2.195 |

**Average perplexity: ~93.1**  
**Average bits-per-byte: ~2.18**

### Tool Validity (Scripted Backend)

- JSON-schema conformance: 100% (5/5 tests pass)
- Path escape rejection: 100% (1/1 test pass)
- Approval gate enforcement: 100% (1/1 test pass)

### Native 3-Task Cycle

**Status:** Not yet validated with real model output.  
**Planned tasks:**  
1. Calculator expression evaluation (deterministic math)  
2. File read with path confinement  
3. Current-time retrieval  

**Target:** ≥2/3 tasks completed correctly with native model + tools, no scripted backend.

## Identity & Provenance

- **No borrowed weights or tokenizer.** Random initialization, own byte-level BPE trained on the corpus.
- **Own data manifest:** every training file has a SHA-256 hash, license string, and source origin recorded in `local_plan.json`.
- **No distillation.** This model was trained from scratch with AdamW on next-token prediction.

## Limitations

- 1.2M parameters is a research prototype, not a production LM.
- Training corpus is narrow (English literature only).
- Perplexity >80 indicates the model has not learned strong long-range structure.
- Repetitive/garbled generation is expected at this scale.
- No safety fine-tuning, no RLHF, no guardrails beyond tool-call schema validation.

## Hardware

- Developed and verified on CPU (Intel/AMD x64)
- CUDA-compatible but not benchmarked on GPU
- Inference memory: ~5 MB (model weights + KV cache for 256 tokens)

## Citation

```
@misc{genesis-local-1m-gutenberg,
  title  = {Genesis local\_1m\_gutenberg},
  year   = {2026},
  url    = {https://github.com/jayprophit/Genesis}
}
```
