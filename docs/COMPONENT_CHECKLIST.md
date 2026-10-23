# Genesis component checklist

Status legend: **DONE** (implemented and tested here), **PARTIAL**, **SPEC** (specified, not built), **REUSE** (exists in another estate repo).

| ID | Component | Status | Where | Next action |
|---|---|---|---|---|
| C01 | Corpus/provenance registry | PARTIAL | `docs/DATASET_REGISTRY.md`, `corpus_digest` | Add first real dataset records |
| C02 | Byte-level BPE tokenizer | DONE | `genesis_model/tokenizer.py` | Train 8k on the local corpus; 32k for larger runs |
| C03 | Data pipeline | DONE (packed + memmap) | `genesis_model/data.py`, `scripts/tokenize_corpus.py` | Dedup, language tags, streaming web ingest |
| C04 | Decoder backbone | DONE | `genesis_model/model.py` | Scale preset + throughput tuning |
| C05 | Vision tower | DONE (synthetic) | `genesis_model/model.py` | Real licensed image-text data |
| C06 | Native tool protocol | DONE | `genesis_model/tools.py` | Constrained JSON decoding |
| C07 | Native tool registry | DONE | `genesis_model/tools.py` | More tools; OS sandbox |
| C08 | Agent loop | DONE | `genesis_model/agent.py` | Wire to C++ authority |
| C09 | Training engine | DONE (single device; local presets verified on GTX 1050 Ti; resume) | `genesis_model/train.py`, `scripts/local_train.py` | Distributed launch, sharding |
| C10 | Checkpoint/registry | DONE | `genesis_model/checkpoint.py` | Replace `MODEL-NONE` after eval |
| C11 | Inference runtime | PARTIAL | `genesis_model/model.py`, `server.py`, CLI `chat` | Preallocated cache, streaming, batching |
| C12 | Real multimodal data | SPEC | — | Licence review + dataset build |
| C13 | Long context | SPEC | — | RoPE scaling + needle tests |
| C14 | Alignment/safety | SPEC | — | SFT/DPO/GRPO + red team |
| C15 | Evaluation harness | PARTIAL | `genesis_model/eval.py` | Task suites, contamination checks |
| C16 | Memory/retrieval | REUSE | Genesis `memory/`, Agent-Bridge `memory.py` | Unify contract |
| C17 | Learning/consolidation | REUSE | Genesis `learning/` | Promotion gate + rollback |
| C18 | Speech | SPEC | — | Corpus, acoustic model, vocoder |
| C19 | Native C++/CUDA engine | SPEC | — | Format, kernels, quantisation |
| C20 | Local API/SDK | PARTIAL | `genesis_model/server.py` | Auth, streaming, TS/Python SDK |
| C21 | Observability/audit | PARTIAL | agent events | Trace IDs, redaction, replay |
| C22 | Security | REUSE | Agent-Bridge `security/`, Genesis `security/` | Integrate scopes and keychain |
| C23 | Organism integration | SPEC | — | `IModelRuntime` seam |
| C24 | Agent-Bridge adapter | SPEC | — | Contract mapping |
| C25 | Brand/distribution | PARTIAL | README | Model card, signed releases |
| C26 | Model/system card | PARTIAL | `docs/MODEL_CARD_TEMPLATE.md` | Fill per release |
| C27 | Evaluation independence | SPEC | — | Withheld sets + external reviewer |
| C28 | Continuous improvement | SPEC | — | Candidate → eval → approval → promote |

## Definition of done for Genesis-1

A release is Genesis-1 only when all of the following are true:

1. A checkpoint exists with a recorded corpus hash and training manifest.
2. `genesis-model eval` runs on a held-out set and the numbers are published.
3. Tool-call format validity is measured on a held-out tool set.
4. The agent completes a fixed task suite with native tools, with no bridge.
5. A model card states limitations and prohibited use.
6. No third-party weights, tokenizer or unlicensed data are present.
7. The C++ organism can load and use the checkpoint through the runtime seam.
8. A rollback to the previous checkpoint is tested.
