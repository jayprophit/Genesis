# Genesis

Genesis is a clean, C++-first research architecture for a digital organism. It is not a chatbot wrapper and does not claim consciousness, biological equivalence, or unmeasured capabilities.

This repository is authoritative. Legacy folders are read-only research sources and are never linked or copied without licensing, provenance, quality, test, and benchmark decisions.

The current foundation contains a compiled `genesis_core` library, machine-readable requirement and domain registries, a lossless index of all 1,451 numbered sections in the canonical specification, deterministic provenance and runtime contracts, identity/lineage contracts, three-strand genome validation, immutable versioned genome storage, deterministic two-parent reproduction with mutation and birth-snapshot audits, origin-labelled inherited memory, RNA-like expression lifecycle, competency-based development, computational cells and differentiation, tissues and organs, audited repair and recycling, bounded signalling, atomic digital metabolism, homeostasis and modulation, immune classification, quarantine, adversarial tests, and benchmarks. The technology-mining addendum is preserved as research data with independent evidence/utility classes and a dead-end cache. The provenance ledger checksum and pending lineage markers are explicitly diagnostic rather than cryptographic authentication; cryptographic identity remains gated. The complete specification remains the permanent program target and is not yet fully implemented.

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/genesis_bench.exe
./build/genesis_runtime_bench.exe
./build/genesis_genetics_bench.exe
./build/genesis_organism_bench.exe
pwsh -NoProfile -File tools/validate_registry.ps1
pwsh -NoProfile -File tools/secret_scan.ps1
pwsh -NoProfile -File tools/report_completion.ps1
pwsh -NoProfile -File tools/report_program_completion.ps1
```

Read `docs/BUILD_ORDER.md`, `docs/IMPLEMENTATION_STAGES.md`, `docs/STAGE_2_LIFE_INTEGRITY.md`, `docs/STAGE_3_4_ORGANISM_SUPPORT.md`, `docs/SPECIFICATION_COMPLETION_POLICY.md`, `docs/FOUNDATION_GAP_REPORT.md`, `docs/audit/SOURCE_AUDIT.md`, and `registry/requirements.tsv` before extending Genesis. The canonical source copy is `docs/specifications/source/genesis.txt`; the generated `registry/canonical_sections.tsv` preserves every numbered section and its exact source line. The technology-mining copy is `docs/specifications/source/technology_mining_addendum.txt`; its registry is research-only and does not trigger unrestricted crawling or experimental hardware implementation.

Program percentages are derived from leaf-level evidence gates in
`registry/completion_components.tsv`; run the program-completion report rather
than estimating progress from file count or passing unit tests.

Authorized legacy research is curated in `docs/research/SOURCE_ABSTRACTIONS.md`; source files and hashes are tracked in `provenance/SOURCE_MANIFEST.tsv`.

The detailed Aetherius disposition is in `docs/audit/AETHERIUS_COMPONENT_MAP.md`. Deferred work on the original programs is tracked separately in `docs/LEGACY_PROJECTS.md`.
