# Genesis Native AI v0.6 — Source/Mechanism Audit

**Basis:** User-supplied `GENESIS_V06_SOURCE_AUDIT_20261024-161303-472.txt` (2026-10-24 16:13 UK), comprising `genesis_seed/model.py`, `corpus.py`, `train.py`, and `train_corpus.py`. Conclusions describe the **supplied source**, not independently observed execution on the user's Windows workstation. Historical checkpoint and generation figures come from the user's preceding PowerShell logs. No source code or checkpoint was changed in this audit.

## Executive verdict

The next-byte objective, time-causal attention mask, model forward-pass alignment, and standard teacher-forced validation appear structurally correct. There is **no demonstrated core attention or off-by-one target defect** in the four supplied source files. Text generation quality remains poor, but the available training exposure is very small. Several real engineering limitations exist, most notably incomplete special-token masking, no context-window continuation, and insufficient diagnostic/benchmark controls. Do not infer language understanding from next-byte cross-entropy improvements.

## Verified architecture

| Mechanism | Source evidence | Finding |
|---|---|---|
| UTF-8 bytes | `model.py` lines 16–30 | Original 0–255 byte vocabulary; BOS=256, EOS=257, IMG=258, PAD=259, INSTRUCTION=260, RESPONSE=261 (262 symbols total). |
| Decoder model | `model.py` lines 75–108 | 2-layer, width-128 TransformerEncoder configured as decoder by causal mask; 4 heads; learned positional embedding; separate image patch projection and output head. |
| Time-causal attention | `model.py` lines 118–144 | Upper triangular boolean mask masks all future positions. All image patches precede text when image supplied. |
| Next-byte labels | `corpus.py` lines 253–258 | `seq[:-1]` as inputs, `seq[1:]` as labels; includes BOS and EOS. Correct shift. |
| Training samples | `corpus.py` lines 260–277 | Randomly chosen train documents, random substring start, now 20% BOS-biased starts; explicitly seeded or global RNG. |
| Validation | `corpus.py` lines 279–292; `train_corpus.py` lines 393–405 | Separately assigned validation documents, sequential nonoverlapping target windows, sum-of-losses weighted by target count; teacher forcing. |
| Optimization | `train_corpus.py` lines 422–466 | Scratch initialization, AdamW, gradient clipping, batch size 4, context 128, CE loss on valid targets. |
| Loading | `train.py` lines 352–359 | `weights_only=True` deserialization and `strict=True` weights. |
| Save/resume | `train_corpus.py` lines 428–481 | Corpus fingerprint on resume; optimizer and RNG restoration; `.tmp` followed by replace. |

### Crucial clarification: right-padding and attention

Input windows are right-padded with `PAD`, with label `-100` for padding. Although there is no separate attention padding mask, **valid positions cannot attend to future right-padding** because the time-causal mask already blocks all future positions. The omission is not, by itself, an established defect under the present padding layout. If training data is ever left-padded, interleaved, or packed differently, attention-masking semantics must be revisited.

## Empirical comparison (previously provided PowerShell results)

| Experiment | Validation CE | Byte-level perplexity | Qualitative greedy generation |
|---|---:|---:|---|
| Corrected sampler, 100 steps | 2.46364 | 11.748 | Repeated `the` |
| Corrected sampler, 400 steps | 2.25367 | 9.523 | Repeated `the` |
| 20% BOS sampler, 400 steps | 2.24617 | 9.451 | Repeated `was`/`wand`/`and` |

BOS20 reduced validation CE by 0.00750 (about 0.33%). This difference is too small to establish robust superiority from one seed, and generated language remained incoherent at greedy and temperature 0.7. Do **not** promote BOS20 to an unqualified default or claim it solves the problem.

## Priority findings

### P0 — Data exposure is insufficient for judging language-model capacity (confirmed experimental limitation)

400 steps × 4 windows × 128 positions = **at most 204,800 target-token presentations**, not unique bytes. The corpus contains ~1,001,798 total bytes and ~900k training bytes; sampled tokens repeat and shorter windows contain padding. Thus exposure is roughly 0.23 corpus-equivalent passes at most. At most 1,600 documents are sampled with replacement from 961 training documents; some documents likely have not been presented at all. A weak 400-step byte-level model is unsurprising.

**Action:** Before complex redesign, establish a batch-size/context-controlled baseline that tracks cumulative non-padding token presentations, distinct documents sampled, elapsed CPU time, training loss, held-out CE, and generated-text examples at multiple intervals. Preserve immutable checkpoint names and do not touch final test documents during tuning.

### P1 — Untrained control tokens can be sampled then silently hidden (confirmed code issue)

`model.py` lines 161–170 mask `BOS`, `IMG`, and `PAD` but not `INSTRUCTION` and `RESPONSE`. The plain corpus trainer does not assign these instruction tokens as training targets; `decode` (lines 25–26) filters out all token IDs >255. If a generated control token is selected, it silently disappears from displayed text while still entering the generated context. **Action:** In plain text mode, mask INSTRUCTION and RESPONSE as well; retain EOS as a stopping token. Add coverage ensuring all special-token behavior is explicit and PyTorch and NumPy implementations agree. This is a real fix but is unlikely by itself to turn 400-step output into coherent stories.

### P1 — Generation ends at context capacity, without rolling context (confirmed design limitation)

`model.py` lines 154–170 cap new bytes at `context - prompt_length - prefix_length`, giving at most 127 new bytes with an empty prompt and fewer as prompts grow. There is no sliding context, position shifting, or KV caching. A prompt longer than context can return an empty string without a clear diagnostic. **Action:** Clarify current generation limits and validate prompt length; add a dedicated long-generation design only after baseline correctness checks. Caching is a performance optimization, not a correctness fix.

### P1 — BOS/EOS exposure is not tracked as an optimization metric (confirmed observability gap)

`corpus.py` lines 255–277 insert BOS/EOS but random sampling and 20% BOS weighting do not ensure adequate EOS appearances. A previously observed BOS rate was 0.25%, compared with 20.5% after patch (400 sampled windows). **Action:** Collect frequencies of BOS as input, EOS as target, PAD fraction, per-document coverage, and lengths across 1,600 or more samples. Avoid assuming the EOS problem exists until counted. For story generation, measure start-of-story and ending behavior separately.

### P1 — Evaluation detects local byte prediction, not free-running coherence (confirmed evaluation gap)

`train_corpus.py` lines 393–405 compute deterministic document-held-out teacher-forced next-byte cross entropy. This measures a real task, but a model may predict common bytes well and still generate repetitive gibberish. **Action:** Add generation benchmark prompts, held-out beginning-of-story CE, teacher-forced conditional accuracy, repetition metrics, validity of UTF-8, invalid special-token rate, and an n-gram/frequency baseline. Report byte-level perplexity, not word perplexity.

### P2 — Training source has fragile code/overwrite and reproducibility assumptions (confirmed)

`train_corpus.py` lines 479–481 call `temp.replace(out)`, replacing an existing checkpoint if a caller reuses a path. The external PowerShell guard previously prevented this, but the trainer itself does not. `train_corpus.py` lines 428–448 load optimizer hyperparameters from a resumed checkpoint, so a passed `--lr` may be superseded by the stored optimizer LR. The metadata labels `shuffle_seed` while Corpus sorts manifests rather than shuffling documents (lines 216–217). **Action:** Make overwrite protection an intrinsic trainer option, record model/source hash and sampler policy in checkpoint metadata, and explicitly report effective LR and resume settings.

### P2 — Overfitting-status names overpromise what the heuristic demonstrates (confirmed)

`train_corpus.py` lines 407–420 classify 'overfitting', 'underfitting', or 'good_fit' by a single stochastic last-batch train CE versus full document-heldout validation CE. That cannot reliably establish underfitting or good fit. **Action:** Rename as a descriptive train/validation-gap warning, or compute both CE scores over fixed comparable datasets and track them longitudinally.

### P2 — Learned absolute positions, attention initialization, and tokenizer are research constraints (review / test)

`model.py` line 101 uses learned absolute positions of length 128; text prompt length plus any image-prefix patches must fit. Byte-level tokenization is exact and independently implemented, but each word costs multiple prediction steps and UTF-8 can end mid-codepoint, causing replacement characters. PyTorch TransformerEncoder clones a prototype TransformerEncoderLayer; initialization via `self.apply` reinitializes Linear/Embedding/Conv modules, but MultiheadAttention `in_proj_weight` is not a Linear child and may initially be identical across cloned layers. Check this with an initialization-seed test before labeling it a performance defect. Consider fresh manual init of QKV for future runs **only if benchmarked**, never modify existing trained checkpoint weights.

### P2 — Vision path is present, not trained by the v0.6 corpus experiment (confirmed scope)

`model.py` lines 97–99 and 124–143 provide RGB patch embeddings. However, `train_corpus.py` lines 455–460 call `model(x)` **without images**. Thus the v0.6 TinyStories corpus run provides no vision-gradient training to `patch_embedding`; saved vision-related parameters remain at random initialization for this experiment. `train.py` has a *separate* toy visual-shape training procedure (lines 308–349), which must not be confused with this text checkpoint. Do not describe this checkpoint as a functional vision model.

### P3 — Provenance is checked within manifest conventions but not externally established

`corpus.py` lines 205–251 requires an explicitly permissioned manifest, UTF-8 text, independent named train/validation/test splits, a nonempty license string, and checks duplicate byte-identical documents by SHA-256. This is useful engineering. It **does not independently establish** that the asserted license grants permission or detect near-duplicate documents with different bytes. The final test split is correctly held aside in the present code; preserve it.

## Prioritized controlled test plan

**Gate A — Read-only model invariants (no training):**
1. Causality: changing final token must not change logits at earlier positions beyond floating-point noise.
2. Target shift: check sampled non-padding `y[t] == x[t+1]` wherever both valid; start-of-document `x[0] == BOS` and EOS target count.
3. Prompt sensitivity: alter meaningful earlier words while keeping length and suffix fixed; measure next-token total variation, not just sampled text.
4. Special-token audit: inspect logit probability mass for EOS, INSTRUCTION, RESPONSE and IMG, plus sampled frequency over generation.
5. Native parity: load the same checkpoint in PyTorch and exported NumPy runtime and compare multiple logits, not only one greedy example, before architectural changes.

**Gate B — Isolated small-batch capability test (requires a fresh scratch checkpoint):** Train on a tiny consented corpus/window until training CE falls sharply and text continuation reproduces a memorized pattern. If it cannot, inspect architecture and optimization; if it can, the current model may primarily be undertrained. Never reuse or overwrite baseline checkpoint paths.

**Gate C — Quantified original-model scaling (requires approval):** After passing A and B, use a fresh run or verified resume with periodic validation checkpoints, document/byte coverage logs, multiple comparable seeds where possible, and predeclared stopping/quality thresholds. Keep width/layers/context constant initially; compare sampling rates on the same dataset and validation procedure. Do not access final test split during iterative development.

**Gate D — Optional architectural research:** Study tokenizer improvements, context extension, caching, and NCM-inspired coarse-to-fine mechanisms independently. Do not merge unverified third-party code, weights, or dependencies into Genesis production.

## Audit confidence classification

- **Confirmed in source:** causal triangular mask; next-byte alignment; plain-training unused control symbols; static context termination; checkpoint overwrite behavior; source model's separate toy vision training; trainer uses no image input.
- **Confirmed in prior logs:** corrected sampler passes; 48 tests passed; BOS20 produced 20.5% BOS in a 400-window audit; held-out byte CE improved slightly; output remained incoherent.
- **Hypothesis requiring experiment:** precise source of repetition, attention's semantic use of context, EOS underexposure severity, degree of benefit from more training, relative value of 20% BOS bias, QKV initialization effect.

**Recommendation:** Preserve both 400-step checkpoints, make no broad architectural modifications, run invariant diagnostics, add safe decoding behavior, then conduct a controlled longer scratch-training / resumable training experiment to determine whether sample exposure explains most of the quality gap. This is a small research model, not yet a conversational agent.
