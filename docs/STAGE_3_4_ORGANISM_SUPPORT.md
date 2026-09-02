# Stage 3-4: Organism Support

Status: **implemented and locally verified** for the bounded interfaces listed below.

This increment implements the first executable organism hierarchy required by
`genesis.txt`: computational cells, differentiation, tissues, organs, controlled
recycling, audited repair, deterministic signalling, resource-backed metabolism,
operating-band homeostasis, endocrine-like modulation, immune classification,
quarantine, and immune incident memory.

The biological terms are architectural analogies. Energy is an explicitly
declared accounting unit; it is not a claim of biological energy. Authentication
only establishes trust when a provider is separately qualified. No cryptographic
provider is approved by this increment.

## Evidence

- Public APIs: `include/genesis/organism/anatomy.hpp` and `systems.hpp`
- Implementations: `src/organism/anatomy.cpp` and `systems.cpp`
- Invariant tests: `tests/organism_tests.cpp`
- Throughput probe: `benchmarks/organism_benchmark.cpp`
- Requirement evidence: `registry/requirements.tsv`

## Lifecycle and failure boundaries

Cells follow a validated state graph from creation through differentiation,
activity, specialization, damage, repair, senescence, and terminal recycling.
Tissues and organs enforce type compatibility, capacity, health, and resource
budgets. Refresh operations are transactional: rejected updates restore the
previous valid member snapshot.

Repair records form a SHA-256-linked audit chain. Repair and immune memory store
evidence and regression-test identifiers, but do not silently execute external
changes or promote diagnostic identity evidence to authenticated identity.

## Scope boundary

This closes the Stage 3-4 implementation slice. It does **not** mean every one
of the 1,451 canonical `genesis.txt` sections is implemented. Later stages still
cover cognition and memory graphs, development and progressive independence,
multimodal/model/device routes, networking and distributed cognition, complete
cryptographic-provider qualification, platform qualification, and population
evolution. Those remain governed by the permanent requirement registry.
