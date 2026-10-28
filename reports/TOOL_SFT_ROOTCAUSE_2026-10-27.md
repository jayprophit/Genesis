# Tool-SFT Milestone: Root Cause + Environment Loss

Date: 2026-10-27
Status: **Root cause found + fix proven + applied. Reproduction blocked by environment loss.**

## 1. Root cause (the hidden blocker of the whole tool-SFT milestone)

`ChatSFTDataset` (genesis_model/data.py) returned **unshifted** labels: `labels[i] = ids[i]`
for assistant tokens. But `GenesisModel.forward(tokens, targets=)` predicts the **next**
token at each position (its own comment: *"Shift is handled by the caller"*). So the SFT
was supervised to predict the token it had already been given.

Consequence: `sft_ce -> 0` (the loss looks fine) while **no tool-call behaviour is learned**.
This is exactly what both prior SFT attempts (SFT-a and the compact re-run) showed:
`sft_ce` collapsed to ~0 but generation produced no `<|tool_call|>` calls.

## 2. Fix (applied, verified present, backed up)

data.py, inside `ChatSFTDataset.__init__`, immediately after the assistant-mask loop:

```python
labels = labels[1:] + [-100]
```

One line. Targets now align with the next token, matching `forward`'s contract.

## 3. Proof the fix works (clean base + shift-fixed dataset, pure SFT)

From `scratch/genesis_tool_sft_rootcause_backup/PROOF_shiftfix_toolcalls.log`:

| step | sft_ce |
|---|---|
| 100 | 0.4560 |
| 200 | 0.0032 |
| 300 | 0.0019 |
| 400 | 0.0031 |

Top tokens right after the ASSISTANT marker: `<|tool_call|>` 18.3%, `{` 12.3, `"` 10.7.
Greedy generation for all four tools emitted the correct wire form:

```
calculator -> <|tool_call|>{"name":"calculator","arguments":{"ex...
read_file  -> <|tool_call|>{"name":"read_file","arguments":{"p...
list_dir   -> <|tool_call|>{"name":"list_dir","arguments":{...
now        -> <|tool_call|>{"name":"now","arguments":{}}<|/tool_call|>   [parses OK]
```

The 3 "got None" were only my 24-token generation cap truncating the JSON before its close;
the tool **names and format are correct**. So the fix is confirmed: with shifted labels the
model learns the tool-call conditional.

## 4. Blocker: the clean base and its data are gone

To turn the proof into a *saved* model I needed the clean base the diagnostic trained from.
It is unrecoverable in the current tree:

- `runs/local_1m_gutenberg/genesis.pt` — wiped.
- `data/pretrain/local_1m_gutenberg/` — wiped (so a base **retrain is not even possible**).
- `scripts/restore_and_validate.py` (base-from-tensors) — wiped.
- `scratch/genesis_ckpt_sft_tool_compact/genesis.pt` — this had been the reconstructed base
  the diagnostic used; it was later overwritten with the buggy SFT checkpoint (byte-identical
  to `runs/sft_tool_compact_seq256/genesis.pt`, sha256 a498f8d1…, meta steps=1220 corpus=builtin).
- The diagnostic wrote its trained model **only to memory** (no save) — gone with the process.
- `genesis_model/model.py` was reverted to the full vision `GenesisModel`
  (the compact `Model` class is gone; forward is now vision-first: `(tokens, images=None, targets=None)`).
- Every surviving checkpoint is a **buggy unshifted SFT**: re-SFT-ing one with the fix still
  gives gibberish (0/4, e.g. "1500000000 francs…") — its base was corrupted by the unshifted
  objective, so it is not a usable restart point.

The likely cause is the concurrent git activity on the co-located Veyra monorepo doing a
checkout / `clean` that strips Genesis-Model's untracked + gitignored files (checkpoints,
pretrain data, scratch scripts).

## 5. Backed up (safe from further reverts) — `scratch/genesis_tool_sft_rootcause_backup/`

- `data.py.shiftfixed` — the fixed ChatSFTDataset.
- `tool_use.jsonl` + `build_tool_corpus.py` — the 4-tool compact corpus (600 records).
- `PROOF_shiftfix_toolcalls.log` — the proof above.
- `re_sft_from_buggy.py` — the re-SFT harness (works once a clean base exists).

## 6. Recovery path (needs you)

1. **Stabilize the tree first** — stop the co-located Veyra git activity from cleaning
   Genesis-Model's untracked/ignored files (move Genesis-Model to its own repo, or
   `git check-ignore` / protect runs/, data/, scripts/). Otherwise everything below re-wipes.
2. **Recover the base** — restore `runs/local_1m_gutenberg/genesis.pt` + `data/pretrain/
   local_1m_gutenberg/` from an external backup. (A base retrain needs that data, which is also gone.)
3. **Re-run the tool-SFT** — with the shift fix + the staged corpus this WILL produce tool
   calls (proven in §3). Harness: `re_sft_from_buggy.py` (point CKPT at the recovered base)
   or the normal `cli.py train --resume-from <base> --sft-corpus data/sft/tool_use.jsonl`.

No source changes beyond the one-line data.py fix; no training run was authorized or performed
against a clean base (none exists). This pass only diagnosed, applied the fix, staged the corpus,
and proved the fix on the (now-lost) clean base.
