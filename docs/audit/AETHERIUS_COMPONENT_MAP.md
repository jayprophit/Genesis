# Aetherius OS component map

## Scope and conclusion

`F:\Aetherius OS` is an owner-authorized, read-only research source. It is a large, partially duplicated collection rather than one reproducibly buildable operating system. Ownership permits use of first-party material for Genesis, but embedded WooCommerce, LearnDash, payment, framework, dependency, asset and other vendor material retains separate provenance.

No Aetherius implementation is accepted directly into the Genesis C++ core in this phase. The high-value findings below are captured as behavior, contracts, failure cases and test ideas for original Genesis slices.

## Preservation warning

A whole-tree read encountered a Windows CRC/data error below:

`F:\Aetherius OS\src\backend-admin\unified-source\aetherius-commerce-core\woocommerce\packages\woocommerce-blocks\build\inner-blocks\filled-mini-cart-contents-block`

Eleven `.terabox.uploading.cfg` artifacts were also observed outside the excluded vendor tree. Because Aetherius is reportedly the only copy, make a verified backup of readable material before further intensive mining. The corrupt/vendor path must not be repeatedly scanned or imported.

Potential private material also exists:

- `assets\aetherius.pfx` and `config\aetherius.pfx` are identical 2,654-byte PKCS#12/PFX containers;
- `config\.env.local` contains API-key-shaped values;
- conversation/training payloads and media exist in nested AI directories.

These are excluded from Genesis and must never be pushed to GitHub.

## Evidence quality

- No root Git history, CMake build or coherent Make build was found.
- Root and `src` npm test scripts deliberately fail with “no test specified.”
- The legacy C test Makefile references absent sources and stale include roots.
- A self-contained C simulation reports 13/13 only in compiler-default mode; strict C17 fails due a missing standard header, and the tests do not exercise real subsystems.
- The substantive C suites fail before parsing because their relative include paths no longer match the tree.
- One Python service-runtime lifecycle behavior passed an isolated, bytecode-disabled smoke equivalent to its small test. It remains an in-memory registry rather than real process supervision.
- A single web test checks a marketing heading, not operating-system behavior.
- Documentation simultaneously claims full completion and reports a roughly 19% build state. Neither percentage is evidence.
- No credible Aetherius performance benchmark is accepted.

## Candidate map

| Priority | Aetherius candidate | Genesis target | Decision | Retained abstraction |
|---|---|---|---|---|
| P0 | `src\infrastructure\os_core\kernel.py` | runtime/resources/homeostasis | Original C++ rewrite | capacity, committed/reserved/peak values; atomic multi-resource reservation; rollback; release on success/failure; stable task ordering |
| P0 | `src\application\services\service_runtime.py` | service/organ lifecycle | Original C++ rewrite | stopped/starting/running/stopping/error transitions; restart and failure metadata; lifecycle hooks |
| P0 | `src\core\events\EventBus.ts` | deterministic event runtime | Original C++ rewrite | typed envelopes, subscriptions, wildcard/once concepts and bounded observation history |
| P0 | weak ledger/audit implementations | provenance regression tests | Reject implementation | adversarial tests for mutation, deletion, wrapping, predecessor tampering and digest recomputation |
| P1 | `src\infrastructure\os_core\memory_graph.py` | memory and inherited knowledge | Original C++ contract/store | records, typed relationships, importance, access metadata, time queries, retention and statistics |
| P1 | `src\infrastructure\monitoring\metrics_runtime.py` | bounded telemetry | Rewrite | counters, gauges, alerts, severity, acknowledgement and health summaries; explicitly avoid its nested-lock deadlock |
| P1 | `src\core\security\ZeroTrustFramework.ts` | security policy boundary | Vocabulary only | subject/resource/action/context request, default denial, reason codes and audit evidence |
| P1 | `src\application\inference\adapter_backend.py` | later model adapter | Deferred quarantined sidecar | local-first order, bounded timeout, explicit backend result, latency, health and availability |
| P1 | `src\core\genealogy\GenealogySystem.ts` | Genesis lineage | Vocabulary only | ancestry traversal, parent links and relationship evidence; do not import human genealogy assumptions |
| P2 | WBE, consciousness and AGI surfaces | cognition research | Taxonomy only | possible memory/goal/affect/self-model vocabulary; reject numerical consciousness claims |
| P3 | resource/log/task web panels | later UI | Wireframes only | evidence badges, filtered logs, resource views and health warnings backed by real Genesis APIs |

## Required Genesis corrections to candidates

### Runtime and resources

- inject logical time and deterministic IDs;
- bind every transition to a provenance event;
- make allocation all-or-nothing and replayable;
- add thread-safe state mutation, idempotence and failure causality;
- base capacities and utilization on measured adapters rather than invented constants;
- make homeostasis policies propose bounded actions through the event path instead of directly mutating runtime state.

### Memory

- add organism ID, birth event and memory namespace;
- add mandatory origin, source parent/memory, confidence and transformation history;
- separate inherited knowledge from autobiographical experience;
- replace `INSERT OR REPLACE` with explicit version/supersession events;
- use deterministic retrieval inputs and validated vector dimensions;
- ledger-back retention, consolidation and deletion decisions.

### Identity, lineage and genome

Aetherius DNA combines identity, memory, capability and current state. Genesis must preserve its existing separation of genome, identity, lineage, memory, temporary expression and current state. Aetherius has no valid equivalent of Genesis RNA, birth cut-off or restore/clone/fork/child semantics. Genetic-algorithm material uses ambient randomness and is deferred until recorded-seed reproduction gates exist.

### Security

Do not reuse Aetherius authentication, encryption, signature, blockchain, backup or “consciousness transfer” implementations. Audit findings include unsigned base64 tokens, `Math.random()` secrets, XOR labeled as AES/AES-GCM/ChaCha20-Poly1305, unverified MFA/email codes, mutable audit records and simulated backup success. Genesis security remains gated on a threat model, approved cryptographic provider and key-custody design.

### Adapters and devices

Capabilities need truth states such as declared, probed, operational, degraded, unavailable and permission-denied. User-agent inference, fixed sensor values, log-only rendering, unconditional biometric success and an `encrypted: true` flag without encryption are not adapter evidence.

## Hard exclusions

- `node_modules`, `.next`, `build`, `obj`, `__pycache__`;
- `.env*`, `*.pfx`, `*.terabox.uploading.cfg`;
- duplicate nested trees until one canonical source is selected;
- `src\backend-admin\unified-source` and other vendor/download mirrors;
- generated binaries and package artifacts unless deliberately retained as historical evidence;
- raw conversations, training payloads, logs, models, audio and images unless separately consented, scrubbed and registered.

## First implementation sequence derived from Aetherius

1. Deterministic event envelope and replay-safe dispatcher.
2. Atomic resource account/reservation transaction tied to provenance.
3. Explicit service lifecycle state machine with failure and concurrency tests.
4. Bounded telemetry types with health-state derivation.
5. Memory graph interfaces with Genesis origin and lineage fields.

Each slice must be independently implemented in C++, tested and benchmarked against the unchanged Genesis baseline. Aetherius remains evidence and inspiration, not a silent dependency.
