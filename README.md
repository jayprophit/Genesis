# Genesis

Genesis is a clean, C++-first research architecture for a digital organism. It is not a chatbot wrapper and does not claim consciousness, biological equivalence, or unmeasured capabilities.

This repository is authoritative. Legacy folders are read-only research sources and are never linked or copied without licensing, provenance, quality, test, and benchmark decisions.

The current foundation contains a compiled `genesis_core` library, machine-readable requirement and domain registries, a lossless index of all 1,451 numbered sections in the canonical specification, deterministic provenance and runtime contracts, identity/lineage contracts, deterministic typed entity addresses, append-only relationship histories, immutable entity-registry recovery, identity-bound append-only digital life records with immutable recovery, three-strand genome validation, immutable versioned genome storage, deterministic two-parent reproduction with mutation and birth-snapshot audits, origin-labelled inherited memory, RNA-like expression lifecycle, competency-based development, computational cells and differentiation, tissues and organs, audited repair and recycling, bounded signalling, atomic digital metabolism, homeostasis and modulation, immune classification, quarantine, an owner-bound cryptographic provider qualification registry, a secret-free key-custody metadata registry with cryptoperiods, typed uses, lifecycle transitions, immutable recovery and rotation/recovery lineage, and a read-only Windows CNG registered-provider inventory with an explicit unsupported fallback on other platforms. Entity, life-record, provider-evidence and key-custody views explicitly do not confer authority or replace organism identity. Genesis can enumerate registered Windows KSP names, but it does not open a provider, access a key store, execute cryptography, authenticate identity/provenance or authorize an action. Qualification and key-use preflight still produce evidence-gated candidates only. The technology-mining addendum is preserved as research data with independent evidence/utility classes and a dead-end cache. The provenance ledger checksum, lineage anchors and pending lineage markers are explicitly diagnostic rather than cryptographic authentication. The complete specification remains the permanent program target and is not yet fully implemented.

```powershell
cmake --preset local-release
cmake --build --preset local-release
ctest --preset local-release
./build/genesis_bench.exe
./build/genesis_runtime_bench.exe
./build/genesis_genetics_bench.exe
./build/genesis_organism_bench.exe
./build/genesis_memory_bench.exe
./build/genesis_learning_bench.exe
./build/genesis_identity_bench.exe
./build/genesis_life_record_bench.exe
./build/genesis_crypto_provider_bench.exe
./build/genesis_key_custody_bench.exe
./build/genesis_crypto_platform_probe.exe
pwsh -NoProfile -File tools/validate_registry.ps1
pwsh -NoProfile -File tools/validate_genesis3_ingest.ps1
pwsh -NoProfile -File tools/validate_genesis4_ingest.ps1
pwsh -NoProfile -File tools/secret_scan.ps1
pwsh -NoProfile -File tools/report_completion.ps1
pwsh -NoProfile -File tools/report_program_completion.ps1
```

Local development uses one regenerable `build/` directory. Feature stages are
separated by requirements, modules, tests and Git commits, not by accumulating
`build-next`, `build-final` or stage-named output directories. A genuinely
different compiler or platform may use a named subdirectory beneath `build/`
when qualification requires simultaneous artifacts, but generated output never
becomes project source or completion evidence by itself.

Read `docs/BUILD_ORDER.md`, `docs/IMPLEMENTATION_STAGES.md`, `docs/STAGE_2_LIFE_INTEGRITY.md`, `docs/IDENTITY_ENTITY_ADDRESSING.md`, `docs/IDENTITY_LIFE_RECORD.md`, `docs/STAGE_3_4_ORGANISM_SUPPORT.md`, `docs/security/CRYPTOGRAPHIC_THREAT_MODEL.md`, `docs/security/CRYPTOGRAPHIC_PROVIDER_BOUNDARY.md`, `docs/security/KEY_CUSTODY_BOUNDARY.md`, `docs/security/CRYPTO_PLATFORM_INVENTORY.md`, `docs/PROGRAM_COMPLETION_HISTORY.md`, `docs/SPECIFICATION_COMPLETION_POLICY.md`, `docs/FOUNDATION_GAP_REPORT.md`, `docs/specifications/GENESIS3_INGEST.md`, `docs/specifications/GENESIS4_INGEST.md`, `docs/audit/SOURCE_AUDIT.md`, and `registry/requirements.tsv` before extending Genesis. The canonical source copy is `docs/specifications/source/genesis.txt`; the generated `registry/canonical_sections.tsv` preserves every numbered section and its exact source line. The separate Genesis3 and Genesis4 continuations are preserved at `docs/specifications/source/`, with every Markdown heading occurrence in their corresponding section registries and family-level semantic decisions in their deduplication maps. The technology-mining copy is `docs/specifications/source/technology_mining_addendum.txt`; its registry is research-only and does not trigger unrestricted crawling or experimental hardware implementation. Cryptographic standards retrieval metadata is isolated in `registry/crypto_reference_baseline.tsv` and has no automatic approval authority.

Program percentages are derived from leaf-level evidence gates in
`registry/completion_components.tsv`; run the program-completion report rather
than estimating progress from file count or passing unit tests.

Authorized legacy research is curated in `docs/research/SOURCE_ABSTRACTIONS.md`; source files and hashes are tracked in `provenance/SOURCE_MANIFEST.tsv`.

The detailed Aetherius disposition is in `docs/audit/AETHERIUS_COMPONENT_MAP.md`. Deferred work on the original programs is tracked separately in `docs/LEGACY_PROJECTS.md`.
