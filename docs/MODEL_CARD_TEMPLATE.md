# Genesis model card — template

**Model:** Genesis-1 seed  
**Checkpoint:** `runs/seed_30m/genesis.pt`  
**SHA-256:** `<fill>`  
**Date:** `<fill>`  
**Owner:** Genesis first-party

## 1. What this model is

A from-random dense decoder trained by this project. State the parameter count,
context length, vocabulary size and modalities actually trained.

## 2. Training

| Item | Value |
|---|---|
| Tokens seen | `<fill>` |
| Compute | `<GPU-hours, hardware>` |
| Data | `<dataset IDs from registry>` |
| Objectives | next token, image caption, tool SFT |
| Code version | `<git commit>` |

## 3. Evaluation

Only include numbers produced by `genesis-model eval` or a named harness on this
checkpoint. For each: dataset hash, command, hardware, date.

| Metric | Value | Dataset | Notes |
|---|---|---|---|
| Bits per byte | | | held-out |
| Tool format validity | | | synthetic/real split |
| Tool selection accuracy | | | |
| Tokens/sec | | | hardware |

## 4. Intended use

Local assistant, tool-using agent, estate automation, research.

## 5. Limitations

State honestly: knowledge cutoff, languages, context, hallucination, vision
scope, safety coverage, hardware requirements, what was not evaluated.

## 6. Prohibited use

Unauthorised surveillance, credential theft, autonomous high-impact actions
without human approval, medical/legal/financial advice without review.

## 7. Licence and contact

`<fill>`
