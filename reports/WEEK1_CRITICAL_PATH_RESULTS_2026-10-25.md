# Genesis-Model Week 1 Critical Path - Results (2026-10-25)

Read-only measurements on `runs/local_1m_gutenberg/genesis.pt` (1,217,664 params, vocab 4096,
trained 1,220 steps / 9,994,240 tokens, `max_seq_len=256`). Env: `.venv-torch-probe` (torch 2.14.1+cpu).
No training, no weight changes, no source edits.

## Step 1 - Held-out evaluation: MET (published)

**Finding 1 (blocker surfaced):** the repo's local "held-out" split
(`data/local/gutenberg_heldout/` = pg1342, pg1399) is **contaminated** - both books are present
in the 59-source training plan (content-digest match). The real eval harness
(`scripts/eval_heldout.py`) correctly **refused** to score it. DoD criterion #2 could not be
satisfied on the existing data.

**Fix:** built a genuinely disjoint held-out set from Project Gutenberg books NOT in the training
plan (pg5 Alice, pg753 Zarathustra, pg34566), verified contamination-free by digest
(`overlap.overlaps == []`, 59 training files).

**Results (64 blocks x 256 tokens scored per file, CPU):**

| Held-out book | loss | perplexity | bits/byte |
|---|---|---|---|
| pg34566 | 4.41 | 81.9 | 2.12 |
| pg5 (Alice) | 4.89 | 132.5 | 2.50 |
| pg753 (Zarathustra) | 5.57 | 261.6 | 2.87 |
| decode throughput | - | - | 67.4 tok/s |

Report: `reports/eval_heldout_local_1m_gutenberg_2026-10-25.json`.
Interpretation: high perplexity is expected for a 1.2M prototype; the number is an honest baseline,
not a product claim. **DoD #2 (held-out eval with published numbers) is now satisfied.**

## Step 2 - Held-out tool validity: NOT MET (measured; root cause found)

Built-in tool-format check (`--tool-cases`, 4 cases): **format_valid = 0/4, tool_accuracy = 0.0**.
The model emits degenerate Gutenberg-style prose instead of tool-call JSON for every case
("What is 12*9?" -> "111. on the 18. The Glacien...").

**Root cause:** the model was trained on plain Gutenberg continuation text with **no
instruction/tool SFT**, so it has no tool-call behavior at all. This is not a harness gap - it is
an absent training stage. **DoD #3 (held-out tool-validity) is NOT met.**

## Step 3 - Native 3-task cycle: BLOCKED (root cause found)

The agent CLI (`genesis-model agent --task ...`) crashes on the first generation:
`ValueError: sequence 1084 exceeds max_seq_len 256`. The tool-augmented agent prompt
(system + task + 7-tool schema) is **1,074 tokens**, ~4x the model's trained 256-token context.
A minimal no-tools prompt (38 tokens) fits, but the model still has 0/4 tool-format validity.

**Root cause:** two compounding gaps - (a) agent prompts exceed the 256-token trained context,
and (b) no tool/instruction SFT. Both trace to the same fact: the prototype is a plain
text-continuation model at 256-token context with no agent training. **DoD #4 (native 3-task
cycle) is NOT met.**

## Net effect on the Definition of Done

| Criterion | Before | After Week 1 |
|---|---|---|
| #2 held-out eval w/ published numbers | not met | **MET** (clean, published) |
| #3 held-out tool-validity | not met | not met (0/4; needs tool SFT) |
| #4 native 3-task cycle | not met | not met (context + tool SFT) |
| #1 checkpoint exists | met | met |
| #6 identity rule | met | met |

## Decision point for the owner

Week 1's honest measurements show the **pipeline and eval harness are solid**, but the model
itself is a plain-continuation prototype: no instruction/tool behavior and a 256-token context.
Closing DoD #3 and #4 requires an **instruction/tool SFT stage at longer context** - i.e. a
training run, which is outside the current no-training constraint. Options:

1. **Authorize a small tool/instruction SFT stage** (new corpus: tool-call + instruction pairs,
   longer context e.g. 1024) - the real unblock for #3/#4.
2. **Keep no-training**: record #3/#4 as known gaps with root cause, ship #1/#2/#6 as the honest
   current state, and treat Genesis-1 as not-yet-releasable until SFT is authorized.
3. **Reduce agent prompt** to fit 256 tokens as an experiment (won't fix tool SFT; limited value).

Recommendation: option 1 is the only path to a defensible Genesis-1; option 2 preserves the
constraint and documents the gap honestly.
