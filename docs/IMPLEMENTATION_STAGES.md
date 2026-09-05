# Genesis incremental implementation plan

Genesis is being completed in dependency order. A stage advances only when its
contracts, implementation, tests, measurements, provenance and registry rows
are present. Later stages may be specified and registered early, but they do
not receive an implemented, proven or stable label until their own evidence
exists.

| Stage | Scope | Status | Exit evidence |
|---|---|---|---|
| 0 | Repository, canonical ingest, source audit, schemas, CI and licensing | COMPLETE | Commit `1abf6b5`; clean C++20 build, tests, registry and secret checks |
| 1 | Deterministic runtime: logical time, envelopes, dispatch, resources, causal state | COMPLETE | Runtime tests and dispatcher/provenance benchmarks |
| 2 | Life integrity and genetics: typed entity addressing, immutable entity/life/genome persistence, lineage-bound digital life records, deterministic recombination, mutation audit, birth cutoff and inherited-origin snapshots | COMPLETE | `genesis_identity_tests`, `genesis_life_record_tests`, `genesis_genetics_tests`; entity, life-record and genetics benchmarks; immutable-store conflict/recovery tests and registry evidence |
| 3 | Computational cells, differentiation, tissues, organs, repair and controlled recycling | COMPLETE | `genesis_organism_tests`, lifecycle audit, transactional composition and recycling tests |
| 4 | Signalling, metabolism, energy, homeostasis and immune boundaries | COMPLETE | `genesis_organism_tests`, bounded queue/resource tests and 100,000-signal benchmark |
| 4S | Cross-cutting cryptographic provider qualification: threat/policy revisions, provider evidence states and immutable recovery | IN PROGRESS | `genesis_security_tests` and 10,000-provider synthetic benchmark pass; actual provider operations, key custody, security review and platform qualification remain closed |
| 5 | Memory, learning, evidence graph, perception, world/self models, affect and cognitive workspace | IN PROGRESS | Associative identity-scoped graph, decay/reinforcement, prediction, belief models and bounded workspace tests; persistence/perception/language integration remains |
| 6 | Embryology, curriculum, teaching, play, sleep/maintenance and progressive independence | PLANNED | Developmental vector tests, teaching/inheritance separation tests and maturity benchmarks |
| 7 | Language, multimodal adapters, body schema, avatar, hardware safety and qualified device routes | PLANNED | Adapter evidence matrix, simulation-before-action tests and platform qualification |
| 8 | Tools, specialist agents, networking, shared domains, distributed cognition and recovery | PLANNED | Protocol/security tests, offline/online boundary tests and replay/recovery evidence |
| 9 | Universal research mining, anomaly science, technology radar and dead-end memory | REGISTERED | Research registry, source provenance, deduplication and evidence/utility validators; no unrestricted crawling |
| 10 | Descendants, population evolution and long-horizon optimization | PLANNED | All prior safety, identity, replay, evidence and benchmark gates plus population experiments |

## Rules for every stage

1. Preserve the canonical source and legacy-source read-only boundaries.
2. Add or update stable public interfaces before implementation details.
3. Keep deterministic mode separate from exploratory/randomized mode; record every seed.
4. Use atomic commit/rollback boundaries for persistent or identity-bearing changes.
5. Add negative, adversarial and recovery tests rather than only happy-path tests.
6. Map implementation files, tests, benchmarks, evidence and provenance in `registry/requirements.tsv`.
7. Never call a capability `PROVEN` or `STABLE` without reproducible evidence.
8. Do not implement experimental quantum, photonic, polariton, BCI, swarm or UAP-derived hardware merely because it appears in the research corpus.
