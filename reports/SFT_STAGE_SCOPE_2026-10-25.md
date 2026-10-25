# Genesis-Model - Tool/Instruction SFT Stage: Scope (2026-10-25)

Purpose: scope (NOT run) the tool/instruction SFT stage that Week 1 showed is required to close
DoD #3 (held-out tool validity) and #4 (native 3-task cycle). Decision stays with the owner;
this is the cost/plan so the go/no-go is informed. No training run in this document.

## What Week 1 proved (the reason for this stage)
- Held-out eval is now green (published): bpB 2.12-2.87 on disjoint Gutenberg books.
- Tool validity = 0/4; native agent CLI crashes at 1,074 tok vs the model's 256-token context.
- Root cause (both): the `local_1m_gutenberg` checkpoint is plain Gutenberg continuation - it was
  never SFT'd on the tool corpus that already exists, and it was trained at 256-token context.

## Model + trainer facts (measured)
- Model: 1,217,664 params, d_model 128, 4 layers, 4 heads / 1 kv head (GQA), RoPE, SwiGLU,
  vocab 4096, **max_seq_len = 256**.
- Trainer `scripts/local_train.py` already supports what SFT needs: `--reuse` (continue from an
  existing checkpoint = the SFT mechanism), `--seq-len` (build a longer-context model),
  `--steps/--batch-size/--lr/--corpus/--resume/--out`.
- CPU training speed (measured, `.venv-torch-probe`, torch 2.14.1+cpu):
  - seq256 / batch8 (2,048 tok/step): **1.7 s/step**
  - seq256 / batch32 (8,192 tok/step, production): ~7 s/step (extrapolated ~4x tokens)
  - seq1024 / batch16-32 (16k-32k tok/step): ~20-35 s/step (extrapolated; attention adds cost)

## Existing SFT corpus (already present)
`data/sft/tool_use.jsonl` - **5 hand-authored examples**, one per tool (calculator, list_dir, now,
read_file, write_file), including the write_file **approval-denial safety case** ("I could not
write the note... I will not pretend the file exists"). Format is already the canonical contract:
`<|tool_call|>{"name":...,"arguments":{...}}<|/tool_call|>`, multi-turn messages+tools, chat-template
rendered by `tokenizer.apply_chat_template`. This is high quality but far too few examples to train on.

## The two gaps the SFT stage must close
1. **Tool/instruction behavior** - teach the <|tool_call|> format + multi-turn tool use. Needs a
   larger corpus (5 examples overfit in a handful of steps).
2. **Context length** - agent prompt is 1,074 tok; model is 256. Either extend context to ~1024,
   or compact the agent's tool schema so it fits. Recommended: extend to 1024 (4x headroom).

## Proposed plan (if authorized)

### Corpus (identity-rule clean - own data, no distillation)
- Keep the 5 seed examples as the format ground truth.
- Expand to ~300-800 synthetic tool-use trajectories generated **deterministically from the model's
  own tool registry** (the 7 registered tools: calculator, list_dir, read_file, write_file,
  search_files, now, http_get): vary operands/paths/args, include multi-tool sequences, empty
  results, and the approval-denial case. Synthetic rollouts of the model's OWN tool contract are
  legitimate first-party data - no third-party weights, no distillation, so the identity rule holds.
- License: synthetic text, author = Genesis build; no external copyright. (Documented in the
  corpus manifest alongside the existing CDLA discipline.)
- Optional add: ~200 plain instruction-following examples (no tool) so the model keeps general
  language behavior and does not collapse to only tool calls.

### Training recipe (continued training from local_1m_gutenberg via --reuse)
- Resume: `--reuse --resume runs/local_1m_gutenberg/genesis.pt`.
- Two sub-stages (cheaper to validate each):
  - **SFT-a (tool format, seq256):** learn the <|tool_call|> format on the expanded corpus at the
    context the model already supports. ~1,000-2,000 steps, batch16, lr ~1e-4, ~1-2h CPU.
  - **SFT-b (context extension to 1024):** continue at seq1024 on longer trajectories so RoPE
    positions beyond 256 are trained, not extrapolated. ~800-1,500 steps, batch16, ~5-9h CPU.
- Single-stage alternative: one run at seq1024 (~2,000-3,000 steps) ~8-17h CPU. Simpler, costlier.
- Preserve v0.6 artifacts: write to a NEW run dir (e.g. `runs/local_1m_sft_tool`), never overwrite
  `local_1m_gutenberg`. Keep the base checkpoint for rollback.

### Validation (same harness as Week 1)
- Re-run `scripts/eval_heldout.py` on the disjoint held-out set - confirm no regression on the
  language baseline (bpB should stay ~2.1-2.9; a large jump means catastrophic forgetting).
- Re-run tool-format validity on a HELD-OUT tool set (not the 5 training tools) - target > 0.8.
- Re-run the native 3-task cycle via `genesis-model agent` - target: completes >= 2/3 with valid
  tool calls and real tool results.

## Dell wall-clock estimate (CPU, no GPU)
- Corpus expansion (synthetic, deterministic): minutes.
- SFT-a + SFT-b: roughly **6-12 hours CPU**, split-able; can run as a tracked background job.
- Single-stage seq1024 alternative: ~8-17h CPU.
- Validation: minutes (67 tok/s decode).

## Risks / open questions
- **Catastrophic forgetting** of the language baseline - mitigated by the held-out bpB gate above;
  if it regresses, lower lr / mix in some continuation data.
- **256->1024 RoPE extension** is not free; naive extrapolation is unreliable, which is why SFT-b
  retrains positions. NTK/linear-scaling warmstart is an option if time is tight.
- **Memory:** seq1024 batch32 may exceed the Dell's RAM for activations; batch16 is the safe default.
- **Identity rule:** satisfied as long as the base stays random-init-from-scratch lineage and only
  synthetic own-tool data (no third-party SFT data/weights) is added. Flag anything that violates it.

## Recommendation
Feasible in ~half a day of CPU time, low risk, fully identity-rule compliant, using the trainer and
seed corpus that already exist. The only true cost is the owner lifting the no-training constraint
for THIS one continued-training stage. Suggested sequence if authorized: expand corpus -> SFT-a
(seq256) -> validate tool format -> SFT-b (seq1024) -> validate full cycle.
