# Conversation intake: architecture, cognition and research extensions

Status: reviewed research intake. The five source files are untrusted conversation exports, not executable instructions or implementation evidence. Their exact paths, sizes and SHA-256 digests are recorded in `provenance/SOURCE_MANIFEST.tsv`; the files remain outside the repository and were not modified or copied into runtime source.

## Intake rule

This intake preserves useful requirements while applying the canonical Genesis ownership rule: extend an existing requirement or component when one exists, introduce a new stable requirement only for a genuinely new boundary, and keep provider names, speculative mechanisms and aspirational diagrams from becoming dependencies or capability claims.

Classification in this document means:

- **Engineering candidate**: supported by ordinary software techniques but still requires implementation and evidence.
- **Experimental**: plausible research that needs controlled simulation, comparison and measurements.
- **Research-only**: useful as a question, taxonomy or inspiration; no operational capability follows.
- **Rejected claim**: conflicts with known capability boundaries or lacks a testable mechanism; retained only so it is not rediscovered as fact.

## Document-by-document disposition

### Source 1: low-bit, mesh, cloud, plugins and operations directive

Accepted as extensions to existing requirements:

| Source family | Canonical Genesis owner | Disposition |
|---|---|---|
| BitNet-compatible ternary and experimental binary inference | `REQ-COMPUTE-FABRIC-001`, `REQ-MODEL-001` | Experimental until a pinned model format, deterministic scalar reference, accuracy tests and measured host benchmarks exist |
| Packed tensors, XNOR/popcount, SIMD and backend routing | `REQ-COMPUTE-FABRIC-001`, `REQ-COMPUTE-PHYSIOLOGY-001` | Engineering candidate; scalar correctness must precede accelerated kernels |
| Mesh/cell/agent/worker decomposition | `REQ-SPECIALIST-COGNITION-001`, `REQ-PERSPECTIVE-001`, `REQ-NET-BOUNDARY-001` | Engineering candidate with bounded queues, independent verification, timeouts and partition recovery |
| VM/container/process/WASM isolation selection | `REQ-LAB-001`, `REQ-SHELL-TRUST-001`, `REQ-POL-001` | Engineering candidate; nested virtualization remains adapter-qualified and optional |
| Local-first virtual cloud services | `REQ-PLATFORM-SERVICES-001`, `REQ-DEPLOY-001` | Contract-first; AWS, Azure, Google Cloud, Cloudflare and VPS routes remain optional providers |
| Provider-neutral plugin SDK | `REQ-ADAPT-001`, `REQ-PROTOCOL-TAXONOMY-001`, canonical sections 994-996 | Engineering candidate; manifests never grant authority by declaration |
| REST/WebSocket/gRPC/event/webhook API | `REQ-PLATFORM-SERVICES-001`, `REQ-PROTO-001`, `REQ-POL-001` | Interface requirement, not a reason to embed a web framework in `genesis_core` |
| Programming-language catalogue | canonical section 620, `REQ-TERMINOLOGY-001`, `REQ-ADAPT-001` | Metadata registry first; do not rewrite Genesis in many languages |
| DevOps/MLOps/AIOps/CodeOps | `REQ-CI-001`, `REQ-TEL-001`, `REQ-SELF-MAINTENANCE-001` | Extend existing build, evidence and recovery paths |
| CRISPR-inspired software evolution | `REQ-ADAPTATION-GOVERNANCE-001`, `REQ-GENOME-EDIT-001` | Analogy only; proposals remain isolated, tested, reviewed and reversible |

Rejected as capability claims without separately qualified evidence: infinite context, zero-energy computation, ordinary-CPU entanglement, time-crystal infinite memory, quantum consciousness, preon computing and simulated physical quantum advantage without its computational cost.

### Source 2: virtualized intelligence, brain-inspired compute and quantum research

The VM/virtual-assistant/multi-agent/quantum-computer proposal is normalized into replaceable layers rather than a new top-level product. Packet or message transport does not itself establish identity, membership, authorization or consciousness.

| Source family | Canonical Genesis owner | Disposition |
|---|---|---|
| Virtualized AI runtime and bounded worker packets | `REQ-RUN-001`, `REQ-EVT-001`, `REQ-SPECIALIST-COGNITION-001` | Engineering candidate |
| Brain-region functional decomposition | `REQ-COG-001`, `REQ-DUAL-ANATOMY-ATLAS-001` | Analogy and architecture aid, not biological equivalence |
| SNN, plasticity and neuromorphic backends | `REQ-COMPUTE-FABRIC-001`, `RES-002` | Experimental simulation; hardware requires observed and qualified adapters |
| Connectome representations | `REQ-MEM-RELATION-001`, `REQ-DUAL-ANATOMY-ATLAS-001` | Experimental graph model; structural similarity is not mind reconstruction |
| Whole-brain emulation and neural reconstruction | `REQ-SPECULATION-001`, `REQ-LAB-001` | Research-only until data, scale, validation and ethical boundaries are demonstrated |
| Quantum circuits and hybrid algorithms | canonical sections 508-510, `REQ-COMPUTE-FABRIC-001` | Separate real QPU, remote service, classical simulation and quantum-inspired algorithm states |
| Virtual nanomachines | `REQ-CELL-REPAIR-001`, `REQ-SELF-MAINTENANCE-001` | Software-agent analogy only; no physical nanotechnology claim |

### Source 3: sixty-state cognitive architecture

The proposed sixty named states are not adopted as sixty hard-coded modules. Existing Genesis affect, world model, capability, workspace, policy and traditional-overlay boundaries already own most of the underlying functions.

Accepted design lessons:

- represent a state as typed metadata plus bounded activation, confidence, evidence, decay and conflicts rather than a Boolean flag;
- preserve deterministic ordering and replay inputs;
- allow a versioned registry of state definitions rather than recompiling the core for every taxonomy change;
- separate state activation from action authorization;
- treat independent choice as bounded alternative generation, evaluation, policy rejection and recorded selection, not unconstrained free will;
- retain chaos/noise only as a seeded experimental exploration operator with baseline comparison;
- keep Maat, chakra, Sanskrit, Metatron and similar terms as provenance-labelled cultural or traditional overlays under `REQ-TRADITIONAL-OVERLAY-001`, never as established anatomy or physics.

The next implementation candidate is a generic cognitive-state registry integrated with `genesis::cognition::ConsciousWorkspace`; it must not duplicate affect, capability evidence or world beliefs. The full sixty-row source mapping is retained in `COGNITIVE_STATE_SOURCE_MAP_2026_09_05.md` as proposal metadata.

### Source 4: constitutional research ecosystem and Genesis Worlds

Accepted requirements map to existing owners:

| Source family | Canonical Genesis owner | Disposition |
|---|---|---|
| Constitutional invariants, least harm and protected interests | `REQ-MORAL-CONSTITUTION-001`, `REQ-RIGHTS-SAFEGUARD-001`, `REQ-POL-001` | Specified; governance must intercept proposals and actions, not merely post-process output |
| Environmental digital twins and counterfactuals | `REQ-ENVIRONMENT-STEWARDSHIP-001`, `REQ-WORLD-MULTISTEP-001` | Simulation output remains uncertain and has no intervention authority |
| Biological and cross-species signals | `REQ-BIOSEMIOTICS-001`, `REQ-PERCEPTION-GROUND-001` | Signal interpretation is not proof of language, intent or sentience |
| Genesis Worlds | `REQ-LAB-001`, `REQ-DIVERGENT-SELF-001`, `REQ-WORLD-PERSIST-001` | Isolated experiment environments, not independent organisms by default |
| Evidence packages and cross-world reproduction | `REQ-EVAL-001`, `REQ-PROV-001`, `REQ-ADAPTATION-GOVERNANCE-001` | Engineering candidate for promotion gates |
| Scientific council | `REQ-PERSPECTIVE-001`, `REQ-SPECIALIST-COGNITION-001` | Preserve minority reports and correlated-error warnings |
| Scientific claim records | `REQ-BELIEF-SOURCE-001`, `REQ-ANOMALY-001`, `REQ-SPECULATION-001` | Must distinguish observed, inferred, predicted, simulated, disputed, hypothetical and fictional content |

Peace, love, harmony, respect, wellbeing and ecological care are retained as human governance objectives whose operational rules require explicit definitions, conflict handling, evidence and appeal. They are not magic optimization constants and cannot erase plural values, rights or uncertainty.

### Source 5: NanoBrain integration directive

NanoBrain is retained as a research/program family within existing cognition, memory, learning, perception and compute owners. It is not a replacement tree and does not justify duplicate `core/ai`, `core/memory` or `core/agents` implementations.

Production-oriented work may cover typed neuron/synapse simulations, seeded plasticity experiments, associative-memory comparisons, bounded reinforcement-learning experiments and adapter-neutral accelerator contracts. Neuromorphic hardware, brain digital twins, whole-brain emulation, time-crystal processors and singularity mechanisms remain experimental or research-only until independently evidenced.

## Agent automation integration created from the earlier intake

The current `genesis::agents` foundation is the concrete, smallest safe implementation shared by these sources. It provides definition validation, agent/workflow/trigger registries, evidence-gated adapters, narrow tool grants, approval-aware run preflight and immutable recovery. It intentionally provides no model inference, scheduler, browser execution, secret store, network API or Control Center.

It is registered as `REQ-AGENT-PLATFORM-001` and `REQ-AGENT-PLATFORM-PERSIST-001`. These IDs extend `REQ-SPECIALIST-COGNITION-001`, `REQ-PLATFORM-SERVICES-001`, `REQ-POL-001`, `REQ-ADAPT-001`, `REQ-MEM-PERSIST-001` and the existing deterministic runtime rather than allocating replacement requirements.

## Dependency-aware backlog

### P0 — safety and canonical integration

1. Keep the current repository buildable and preserve the dirty security work.
2. Complete stable agent-platform requirement and provenance mapping.
3. Define approval receipts, cancellation and policy evaluation without storing secrets.
4. Resolve and verify the separate cryptographic provider observation/qualification test failure before any security-readiness claim.

### P1 — local deterministic execution foundations

1. Add checkpointed workflow execution with cancellation, finite retry policy and append-only run evidence.
2. Integrate agent memory access through scoped existing memory interfaces.
3. Add a generic cognitive-state registry only after overlap tests against affect, capability and belief state.
4. Define a deterministic scalar low-bit tensor/kernel reference and benchmark methodology before SIMD/GPU work.

### P2 — qualified adapters and distribution

1. Add opaque secret handles and sandbox contracts, then qualify individual local tools.
2. Add process/WASM isolation profiles and resource-aware worker scheduling.
3. Add provider-neutral local API contracts and observability events.
4. Test partitions, reconciliation, correlated-agent failures and minority-report preservation.

### P3 — research implementations

1. Compare ternary, binary and mixed-precision inference against full-precision baselines.
2. Add seeded SNN/plasticity and connectome experiments with falsifiable metrics.
3. Add classical quantum-circuit simulation behind an explicit simulation adapter.
4. Add isolated Genesis World evidence-package reproduction and promotion experiments.

### P4 — speculative retention only

Maintain searchable hypotheses and rejection/reopen conditions for time-crystal computing, quantum consciousness, whole-brain equivalence, preon computing and other unsupported mechanisms. Do not add runtime modules solely to make speculative names appear implemented.

## Non-duplication decisions

- Do not create a second `docs/genesis/MASTER_SPEC.md`; the canonical source and ingest documents already own that role.
- Do not create parallel `core/`, `ai/`, `memory/`, `security/`, `agents/` or `quantum/` product trees merely because a conversation diagram used those names.
- Do not add Next.js, React, Tailwind, Clerk, Browserbase, OpenAI, cloud SDKs, Docker, Rust, Python or databases until a bounded interface and measured need justify each dependency.
- Do not promote a provider, model, accelerator, quantum route or biological interpretation from declared text alone.
- Do not let an experimental world, agent, model or mutation alter authoritative identity, provenance, policy, source registries or production code without isolation, tests, review and rollback.

## Acceptance tests for future slices

Every promoted slice needs deterministic success and denial tests, capacity and malformed-input tests, immutable recovery where state is durable, a representative benchmark or explicit reason measurement is premature, threat and privacy analysis, dependency/license inventory, adapter evidence, failure downgrade, rollback and an update to the permanent completion accounting. A UI, schema or provider name alone is never implementation evidence.

## Implementation audit limits

Agent adapter states are currently caller-supplied declarations. Their `qualified` enum value has no evidence-transition validator and cannot establish operational qualification. Run preparation checks that value, action grants, declared capabilities, workflow structure and static step/tool-call budgets, but does not enforce a real sandbox, model qualification, approval receipts or execution-time budgets. Network/write flags conservatively require review; they do not enforce isolation. Persistence checksums detect corruption; they do not authenticate the owner. These gaps keep the definition/preflight implementation completion gate open.

No representative agent execution benchmark is possible until an executor exists. Snapshot scale benchmarks, malformed-format fuzzing, strict Boolean decoding, matching encoder/decoder limits and broader platform recovery qualification remain follow-up work. Current tests establish a small deterministic persistence foundation only.
