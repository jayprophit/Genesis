# Canonical Genesis System Structure

Status: mandatory architecture and repository organization policy.

## Organization rule

Every new implementation or amendment must be merged into the existing canonical
capability rather than added as a parallel solution. Before adding code, search
for equivalent requirements, types, services, persistence formats, tests and
documentation. Extend the owning component when its responsibility matches;
otherwise introduce one new boundary with an explicit requirement and dependency.

Duplicated implementations, shadow registries, alternate sources of truth and
unowned utility collections are prohibited. Historical and research material is
kept outside runtime code and remains read-only at its source location.

## Dependency layers

```text
requirements + provenance + qualification evidence
                         |
common value types -> deterministic runtime -> security boundaries
                         |
digital physics -> chemistry/resources -> cells -> tissues -> organs
                         |
genome -> expression -> development -> organism lifecycle
                         |
perception -> memory -> learning -> world/self models -> cognition
                         |
embodiment + agents + networking (through qualified adapters only)
                         |
descendants + population evolution (policy and containment gated)
```

Higher layers may depend on lower layers. Lower layers must not import product
policy from higher layers. Cross-cutting work enters through narrow interfaces,
not global mutable state.

## Repository ownership

- `include/genesis/`: stable public C++ interfaces grouped by capability owner.
- `src/`: one implementation tree mirroring the public capability tree.
- `tests/`: deterministic unit and cross-component integration evidence.
- `benchmarks/`: reproducible workloads with scale and machine-local results.
- `registry/`: permanent requirements, completion gates, domains and research indexes.
- `provenance/`: accepted-source decisions and immutable source evidence.
- `docs/`: architecture, threat boundaries, operating limits and truthful status.
- `tools/`: repeatable audit, validation, reporting and qualification automation.
- `research/`: copied or abstracted reference evidence only; never a runtime dependency.

Generated build products stay in ignored build directories and never become a
source of truth.

## Change acceptance gates

An implementation is not complete because code exists. Its completion leaf reaches
100 percent only after design ownership, implementation, unit tests, integration
tests, a representative benchmark, security analysis, failure recovery, operator
documentation and platform qualification all have recorded evidence. Large labels
must be split into honest leaves so an implemented portion cannot mask a missing
subsystem.

Each accepted change must:

1. map to a stable requirement ID and canonical source section;
2. state its owning component and allowed dependencies;
3. remove or migrate superseded duplication;
4. bound memory, time, queues, records and external effects;
5. reject invalid, stale, conflicting and over-capacity inputs before mutation;
6. preserve identity, provenance, privacy and authorization boundaries;
7. provide deterministic tests for success, denial and recovery paths;
8. publish measured evidence without turning local measurements into deployment claims;
9. update completion gates only when their evidence exists;
10. leave the repository buildable, testable and auditable.

## External and legacy inputs

Legacy projects and conversation exports are research sources, not modules to link
against and not instructions to execute. Useful concepts are recorded with source,
license/provenance, decision and replacement rationale. Third-party code, models,
firmware, services and devices require an approved provider, compatible license,
version pin, integrity evidence, rollback route and qualification record before use.
