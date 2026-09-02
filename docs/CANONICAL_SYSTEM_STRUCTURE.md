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

The supported local workflow has one disposable top-level `build/` directory,
configured by `CMakePresets.json`. Development stages are integrated continuously
in the canonical source tree and separated by requirements, interfaces, tests and
commits. They are not assembled later from parallel `build-final` or `build-next`
trees. When simultaneous compiler or platform qualification becomes necessary,
artifacts may be placed in explicit subdirectories such as `build/msvc-release`
and `build/clang-sanitized`; those names describe a reproducible toolchain profile,
not a feature branch or a claim of finality.

## Programming-language policy

Genesis is C++-first, not C++-only. C++20 owns the organism runtime, deterministic
state, memory, cognition, safety-critical boundaries, persistence formats and
hardware-facing contracts. Additional languages are admitted only when a defined
layer benefits materially and the cross-language boundary is smaller and safer
than implementing that layer in C++.

Current approved roles are:

- C++20: production runtime, libraries, simulations, adapters and performance tests.
- CMake: portable build graph, installation and test registration.
- PowerShell: Windows-local audits, registry generation and operator automation.
- YAML: continuous-integration declarations only.
- JSON/TSV/Markdown: configuration, registries, evidence and documentation; these
  are data formats rather than executable product layers.

Potential future languages require a registered decision before introduction:

- Python may serve offline research, dataset preparation, evaluation and model
  conversion, but must not become an implicit production runtime dependency.
- Rust may own a narrowly defined memory-safe parser, cryptographic boundary or
  network service only after ABI, supply-chain, build and recovery qualification.
- TypeScript may own a local operator interface or visualization layer through a
  versioned API; it must not duplicate organism state or business rules.
- C or platform SDK languages may be used behind qualified device adapters when
  a vendor ABI requires them.
- Shader languages may be used for visualization or qualified accelerator kernels
  with a deterministic CPU reference path where correctness requires one.

Each added language must have an owner, pinned toolchain, license and dependency
inventory, reproducible build, formatting and static-analysis rules, tests,
security review, failure isolation, data/ABI contract, upgrade policy and removal
path. Polyglot code is justified by capability evidence, never by fashion.

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
