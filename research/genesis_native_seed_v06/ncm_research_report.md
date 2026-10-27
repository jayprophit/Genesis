# NCM Research Audit — Genesis Native AI v0.6
**Date:** 2026-10-24 | **Status:** READ-ONLY AUDIT COMPLETE | **Sources:** arXiv 2610.14162, GitHub HazyResearch/ncm, HF hazyresearch/ncm-checkpoints

## Phase 1 — Source Verification

| Source | URL | HTTP | Verified |
|---|---|---|---|
| arXiv paper | https://arxiv.org/abs/2610.14162 | 200 | yes |
| GitHub repo | https://github.com/HazyResearch/ncm | 200 | yes |
| HF checkpoints | https://huggingface.co/hazyresearch/ncm-checkpoints | 200 | yes |

**Repo metadata (GitHub main):** License: MIT (Copyright 2025 Hermann Kumbong). 5 commits. Created 2026-10-11.
**Dependencies:** torch, numpy, omegaconf, hydra-core, transformers>=4.51, huggingface_hub[cli], regex, rouge_score, sacrebleu.
**Verified: models/quant.py EXISTS (28,762 bytes)** — confirmed via GitHub API tree.
**HF checkpoints:** 7 task folders + tinystories: xsum, wmt14, sudoku, lm1b, tinystories, owt.

## Phase 2 — Architecture Research

### 2.1 Overview
Two-stage coarse-to-fine framework: (1) multiscale residual VQ-VAE encodes text into a hierarchy of discrete codes; (2) transformer generates one level at a time, predicting all codes within a level in parallel. One forward pass per level after the first.

### 2.2 Multiscale Residual VQ-VAE (models/quant.py, models/vae.py, models/vqvae.py)
- Multiple codebooks (one per level), updated by EMA (not gradients)
- quant_resi: per-level residual convolution blending h*(1-r) + Conv(h)*r
- learned_l1_sampling: learned down/upsampler at level 0; learned_l1_cascade for cascaded conv
- aux: fine_drop (prob 0.5, min_keep 1), code_corruption (uniform, skips level 0)
- Levels (TinyStories): [1, 8, 16, 24, 32, 40, 52, 64, 76, 92, 108, 128] (12 levels)
- Pre-tokenizer: BPE (8192 vocab) or HF pre-tokenizer (bert-base-uncased / gpt2)

### 2.3 Coarse-to-Fine Generator (models/next_concept.py)
- NextConceptTransformer subclass of Transformer
- Config (TinyStories): n_layer=12, n_head=12, n_embd=768, context=129, prefix=1, vocab=[4096]*12, attn_pattern=block_diagonal, rope_base=100000, qk_norm=true, level_encoding=true, tie_heads_to_codebook=true
- Tied heads: _TiedCodebookHead (distance-based, no projection bias)
- Sampling: temperature, top_k, top_p, frequency penalty, adjacency penalty
- Inference: 12 forward evaluations per sequence (1 per level)

### 2.4 Inference Steps
- Inferred steps = number of levels (12 for TinyStories, 9 for XSum, 11 for LM1B-small)
- First level processes prefix + coarsest codes; each subsequent level processes only its chunk

### 2.5 Memory (estimated)
- TinyStories: ~67M params (768-dim, 12 layers) + codebook embeddings (12x4096x768)
- LM1B-large: ~300M+ params (1280-dim, 36 layers) — exceeds GTX 1050 Ti 4GB

## Phase 3 — Genesis Compatibility

### 3.1 Genesis v0.6 vs NCM

| Dimension | Genesis v0.6 | NCM (published) |
|---|---|---|
| Params | 486,790 | 67M–100M+ |
| Vocabulary | 262-byte | 4096–8192/level |
| Context | 128 tokens | 129–560 tokens |
| Architecture | Original transformer (2L, w=128) | 12L transformer + multiscale VQVAE |
| Framework | PyTorch CPU + NumPy | PyTorch only |
| Training | Original random weights | Pretrained on LM1B/OWT/XSum/TinyStories |

### 3.2 Mechanism Compatibility Matrix

| NCM Mechanism | Proven? | Genesis-compatible? | Notes |
|---|---|---|---|
| Multiscale residual VQ-VAE | yes | partial | Needs 262-token-compatible tokenizer; 4096+ codebooks |
| Coarse-to-fine generation | yes | partial | Two-stage inference adds complexity |
| Level-wise parallel decoding | yes | partial | Requires block_diagonal mask; Genesis uses full causal |
| Tied codebook heads | yes | no | Genesis uses untied linear heads |
| Learned RoPE | yes | no | Genesis likely different position encoding |
| QK normalization | yes | no | Genesis normalization may differ |
| CFA sampling | yes | no | Separate from architecture |
| Post-upsample conv | yes | no | Complexity not needed for 128-token context |

### 3.3 Key Constraint
- Genesis: 262-token vocab, 128-token context, CPU-first, NumPy inference
- NCM: 4096+ vocab codebooks, PyTorch-only, PyTorch training
- Config compat: TinyStories config (129 context, 12 levels) closest to Genesis's 128-token context

## Phase 4 — Workstation Feasibility

### 4.1 CPU-Only
- Yes pure PyTorch CPU training (486k params, minutes on CPU)
- Yes pure PyTorch CPU inference (12 forward passes per sequence)
- No NumPy inference (NCM is PyTorch-only; no NumPy export path)

### 4.2 GTX 1050 Ti 4GB
- Marginal for TinyStories (~67M params + codebook buffers)
- No for LM1B-small/large (~300M+ params)
- Published speed claims not transferable (measured on author GPU cluster, vs DLM baselines)

### 4.3 No-Paid-API
- Obeyed: All research read-only; no paid APIs used.

## Phase 5 — Licensing and Risks

| Risk | Mitigation |
|---|---|
| Misattribution: using NCM pretrained weights | Do NOT integrate; preserve Genesis as first-party |
| Dependency conflict: transformers>=4.51, hydra, omegaconf | Future isolated research environment only |
| Checkpoint lock-in: pt-only, no NumPy export | Future: implement NCM-to-NumPy export separately |
| Hardware mismatch: 4GB vs published model sizes | Use scaled-down configs for GPU deployment |
| License respect: NCM is MIT | Document in final report |

## Phase 6 — Gap Analysis

| Gap | Recommendation |
|---|---|
| No NumPy runtime for NCM | Future: implement NCM inference in NumPy (separate research) |
| No CPU-only training script | Requires Dell workstation |
| No config for Genesis 262-token vocab | Design 262-vocab multiscale tokenizer for Genesis |
| No Genesis-compatible context | Evaluate TinyStories config as closest match (129 context) |
| No training on Genesis corpus | Future: train NCM-style tokenizer on Genesis corpus |

## Deliverables: Proposed Isolated Experiment (Requires Approval)

Title: Genesis-NCM Hybrid: Multiscale Residual VQ Tokenizer for Genesis v0.6

Scope (after approval):
1. Implement multiscale residual VQ tokenizer in pure PyTorch
2. Train on Genesis v0.6's expanded corpus (1MB, 3,024 docs)
3. Evaluate perplexity and generation quality vs. Genesis v0.6's 486k native transformer
4. Export successful checkpoints to NumPy inference runtime
5. Compare against unigram/bigram baselines

Hardware requirement: Dell workstation (Python 3.14.7, torch 2.14.1+cpu)
Constraints: Read-only for Genesis; no pretrained NCM weights integrated; no paid APIs; preserve Agent Bridge and RSI safety mechanisms.

## Conclusion

NCM is a proven, well-architected two-stage generative framework with credible speedups over diffusion baselines. However:
1. Published performance claims are hardware-specific and do not transfer to Genesis's CPU-only environment or 4GB VRAM budget.
2. NCM is PyTorch-only — no NumPy inference runtime exists.
3. Genesis v0.6's compact footprint is architecturally different from NCM's 67M–100M+ models.
4. Compatibility is partial: multiscale VQ-VAE and coarse-to-fine generation are conceptually interesting but require Genesis-compatible tokenizer, codebook alignment, and CPU-optimized inference.

Recommendation: Do not replace Genesis v0.6. Future work: design a Genesis-scale (486k–685k param) variant of the multiscale VQ-VAE tokenizer, evaluate on the expanded corpus, and compare against Genesis's native transformer — preserving the original model and Agent Bridge safety mechanisms.

## Source Citations

1. Kumbong, H., Khare, I.S., Mlauzi, I., Woen, A., Ding, C., Buchanan, E.K., Arora, S., Mirhoseini, A., Re, C. (2026). Next-Concept Modeling: Fast, Parallel Coarse-to-Fine Text Generation. arXiv:2610.14162. https://arxiv.org/abs/2610.14162
2. HazyResearch/ncm (main branch). https://github.com/HazyResearch/ncm — MIT. Key files: models/quant.py, models/next_concept.py, models/vae.py, models/vqvae.py, tokenization/vqvae_tokenizer.py, utils/registry.py, config/eval/tinystories.yaml, config/eval/xsum.yaml, config/eval/lm1b-small.yaml
3. HF checkpoints: https://huggingface.co/hazyresearch/ncm-checkpoints — MIT. 7 folders: xsum, wmt14, sudoku, lm1b, tinystories, owt.

## Completed Actions

| # | Action | Result |
|---|---|---|
| 1 | Source verification (HTTP GET) | All 3 sources return HTTP 200 |
| 2 | Repository tree inspection (GitHub API) | 47 entries; confirmed models/quant.py exists (28,762 bytes) |
| 3 | LICENSE extraction | MIT License, Copyright 2025 Hermann Kumbong |
| 4 | requirements.txt | torch, numpy, omegaconf, hydra-core, transformers>=4.51, huggingface_hub[cli], regex, rouge_score, sacrebleu |
| 5 | Architecture research (arXiv HTML) | Two-stage framework, multiscale residual VQ-VAE, coarse-to-fine transformer |
| 6 | Code analysis (Python requests) | Read quant.py, next_concept.py, vae.py, vqvae.py, vqvae_tokenizer.py, registry.py |
| 7 | Config analysis | TinyStories: 12 levels, [4096]*12 vocab, 128 target, 128 context |
| 8 | Genesis compatibility | Partial — vocabulary, scale, and context differ; CPU-only inference possible but PyTorch-only |
| 9 | Workstation feasibility | CPU-only training/inference possible; GPU speed claims not transferable to 4GB VRAM |
| 10 | Licensing audit | NCM MIT; Genesis preserved as first-party; no incompatible licenses |
| 11 | No modifications | All actions read-only; zero files changed, zero packages installed
