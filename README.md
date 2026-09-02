# Genesis

Genesis is a clean, C++-first research architecture for a digital organism. It is not a chatbot wrapper and does not claim consciousness, biological equivalence, or unmeasured capabilities.

This repository is authoritative. Legacy folders are read-only research sources and are never linked or copied without licensing, provenance, quality, test, and benchmark decisions.

The current foundation contains a compiled `genesis_core` library, machine-readable requirement and domain registries, a lossless index of all 1,451 numbered sections in the canonical specification, deterministic provenance and runtime contracts, identity/lineage contracts, three-strand genome validation, RNA-like expression lifecycle, competency-based development, adversarial runtime tests, and benchmarks. The provenance ledger checksum is explicitly diagnostic rather than cryptographic authentication; cryptographic identity remains gated.

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/genesis_bench.exe
./build/genesis_runtime_bench.exe
pwsh -NoProfile -File tools/validate_registry.ps1
pwsh -NoProfile -File tools/secret_scan.ps1
```

Read `docs/BUILD_ORDER.md`, `docs/FOUNDATION_GAP_REPORT.md`, `docs/audit/SOURCE_AUDIT.md`, and `registry/requirements.tsv` before extending Genesis. The canonical source copy is `docs/specifications/source/genesis.txt`; the generated `registry/canonical_sections.tsv` preserves every numbered section and its exact source line.

Authorized legacy research is curated in `docs/research/SOURCE_ABSTRACTIONS.md`; source files and hashes are tracked in `provenance/SOURCE_MANIFEST.tsv`.

The detailed Aetherius disposition is in `docs/audit/AETHERIUS_COMPONENT_MAP.md`. Deferred work on the original programs is tracked separately in `docs/LEGACY_PROJECTS.md`.
