# Genesis-1 architecture

This is the concrete architecture of the first-party model and runtime. The
estate-level analysis and the full C01–C28 component specification are in
[`docs/ECOSYSTEM_ANALYSIS_2026-10-23.md`](docs/ECOSYSTEM_ANALYSIS_2026-10-23.md).

## 1. Boundary

```text
Genesis Organism (C++ Genesis repo)
  identity · life record · memory · development · authority
        │  intention / action proposal
        ▼
Genesis Agent (this repo, native loop)
  plan → model → tool proposal → policy → execute → observe → answer
        │                         │
        ▼                         ▼
Genesis Model (this repo)     Native tools (default)
  weights · tokenizer           calculator, files, time, search
  vision · tool tokens          Agent-Bridge (optional external plane)
```

- **Genesis Model** answers "what is likely next / what does this image contain".
- **Genesis Agent** answers "what should be done next, with which tool".
- **Genesis Organism** answers "who is this across time, and who may authorise this".
- **Agent-Bridge** answers "how do we reach an external device, browser or service".

No layer may silently impersonate another. A model output is not an action; an
action proposal is not an authorisation; an authorisation is not an effect.

## 2. Model

| Property | Genesis-1 |
|---|---|
| Type | decoder-only transformer |
| Norm | RMSNorm, pre-norm |
| Position | rotary (RoPE), theta 10k, scaling later |
| Attention | grouped-query (GQA), causal, SDPA |
| FFN | SwiGLU |
| Embeddings | tied input/output |
| Tokenizer | byte-level BPE, 256 bytes + 16 control tokens + merges |
| Vision | conv patch embed + ViT + linear projector, trained jointly |
| Context | 2k seed, 4k small, 8k 1B |
| Precision | fp32 CPU, bf16 CUDA training, int8/int4 inference later |

### Why these choices

- **Byte-level BPE:** every script works, no UNK, no borrowed vocabulary. The
  cost is longer sequences for non-Latin text; a 32k–64k merge vocabulary and a
  multilingual corpus reduce it.
- **GQA:** cuts KV cache and decode memory with little quality loss. It is the
  right default for local inference on 4–24 GB devices.
- **RoPE:** clean extrapolation story and no learned position table to resize.
- **SwiGLU:** standard, strong, simple.
- **Early vision fusion:** image patches enter the same sequence as text, so
  tool use can reason over a screenshot and a sentence together. A separate
  cross-attention tower is a later experiment, not the v1 default.

## 3. Native tool protocol

The model emits:

```text
<|tool_call|>{"name":"calculator","arguments":{"expression":"17 * 24"}}<|/tool_call|>
```

The runtime:

1. parses only well-formed JSON;
2. validates the arguments against the tool's JSON Schema;
3. checks the tool's risk and approval policy;
4. confines path tools to the workspace;
5. executes and returns a structured result;
6. records the call, result and duration in the trace;
7. feeds the result back as a `tool` message.

The model never sees a fabricated result. A refusal is returned as a result with
an error code, and the model is expected to explain it.

## 4. Agent loop

```text
messages = [system, user]
for step in 1..max_steps:
    output = model(messages, tools)
    if no tool call:
        return answer
    for call in calls:
        result = registry.call(call)
        messages.append(assistant output)
        messages.append(tool result)
return stopped(max_steps)
```

Bounded steps, bounded tool calls, no shell in the default registry, and an
approval callback for writes and network access.

## 5. Training

```text
text corpus ── tokenizer ── packed blocks ──┐
image/caption pairs ────────────────────────┼── shared decoder ── CE losses
chat/tool traces ── prompt masking ─────────┘
```

Loss:

```text
L = L_text + λ_vision · L_caption + λ_sft · L_sft
```

The SFT loss masks system, user and tool-result tokens so the model learns to
answer and call tools, not to reproduce prompts.

## 6. Runtime

- Python research runtime now: `GenesisModel.generate_ids`, KV cache, local HTTP.
- C++/CUDA native runtime later: own model format, quantised kernels, scheduler,
  streaming, memory budget, hardware profiler.
- The wire contract is provider-independent and compatible with the existing
  Agent-Bridge inference contract.

## 7. What must not be built into the model

- identity or life record (belongs to the organism);
- authority or approvals (belongs to the policy/authority layer);
- a renderer or avatar (belongs to presentation);
- a device/browser/cloud connector (belongs to Agent-Bridge);
- a claim of consciousness, biology or human equivalence.

The model is one organ of Genesis, not the whole organism.
