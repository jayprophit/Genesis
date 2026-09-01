# Genesis

Genesis is a clean, C++-first research architecture for a digital organism. It is not a chatbot wrapper and does not claim consciousness, biological equivalence, or unmeasured capabilities.

This repository is authoritative. Legacy folders are read-only research sources and are never linked or copied without licensing, provenance, quality, test, and benchmark decisions.

The initial executable slice contains a permanent requirement registry, deterministic provenance ledger, identity/lineage contracts, three-strand genome, RNA-like expression lifecycle, competency-based development, tests, and a benchmark. The ledger checksum is not cryptographic authentication; cryptographic identity remains gated.

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/genesis_bench.exe
```

Read `docs/BUILD_ORDER.md`, `docs/audit/SOURCE_AUDIT.md`, and `registry/requirements.tsv` before extending Genesis.

Authorized legacy research is curated in `docs/research/SOURCE_ABSTRACTIONS.md`; source files and hashes are tracked in `provenance/SOURCE_MANIFEST.tsv`.

The detailed Aetherius disposition is in `docs/audit/AETHERIUS_COMPONENT_MAP.md`. Deferred work on the original programs is tracked separately in `docs/LEGACY_PROJECTS.md`.
