# Authorized source abstractions

This document brings useful information from the user's authorized legacy data into Genesis without importing old programs wholesale. Exact evidence paths and digests are recorded in `provenance/SOURCE_MANIFEST.tsv`. These abstractions are inputs to original Genesis design and do not claim that the source implementations are production-ready.

The user expanded authorization on 2026-09-02 to useful first-party files and
data of any type within Aetherius OS, the AI conversation corpus and AI Digital
Twin. `docs/research/SOURCE_AUTHORIZATION.md` records the boundary. The source
trees remain read-only, and embedded third-party licenses still apply.

## Aetherius OS

### Retain as design input

- Keep domain behavior independent from presentation, storage and external services.
- Use stable interfaces and replaceable adapters at infrastructure boundaries.
- Separate system health, monitoring, security and recovery from feature marketing.
- Maintain layered tests: unit contracts, integration boundaries, end-to-end workflows and performance baselines.
- Treat scheduling, power/resource management, sandboxing, encryption and update/recovery as explicit subsystems rather than UI labels.

### Genesis adaptation

- Translate generic application/domain/infrastructure layering into organism layers: physics/runtime, identity/ledger, genome/expression, cells/tissues, organs and external adapters.
- Do not import the Aetherius trading, commerce, web, WordPress/vendor or large UI surface into the organism core.
- Treat its many web application panels as interface prototypes unless an underlying service and observed hardware/runtime path exist.
- Reject “quantum,” whole-brain, neuromorphic scale and performance claims until a concrete algorithm, hardware adapter, reproducible dataset and measured result exist.

### Source-health warning

Enumeration encountered a cyclic-redundancy-check read error under a deeply nested vendored WooCommerce email-template path. That vendor/unified-source area is excluded from candidates. The error may indicate an unreadable file or external-drive/filesystem damage; Genesis must never assume the source inventory is complete until the drive is checked or backed up.

## AI Digital Twin

### Capability truth model

The source distinguishes `real`, `partial`, `simulated` and `planned`, and carries summary, evidence, next step, dependency and category fields. Genesis already has stricter statuses (`operational`, `contract`, `planned`, `gated`) and should add:

- evidence kind and exact evidence identifier;
- verifier and verification timestamp;
- measured environment and reproducibility seed where relevant;
- explicit distinction between deterministic control logic, adapter-backed behavior and model-generated output;
- automatic downgrade when evidence or a required adapter becomes unavailable.

The source registry's explicit status entries are human assertions, while its default scanner relies on weak text heuristics such as finding `random`, `stub`, `placeholder` or `pass`. Genesis should retain the honesty goal but replace those heuristics with build/test/benchmark evidence.

### Memory concepts

Useful abstractions include sensory, working, episodic, semantic, procedural and affect-associated stores; bounded working capacity; association links; persistence; consolidation; and query/recency/importance scoring.

Genesis must add requirements absent from that implementation:

- injected logical time rather than direct wall-clock calls;
- organism and memory namespace identity;
- immutable origin (`direct`, `inherited_parent_a`, `inherited_parent_b`, `taught`, `observed`, `inferred`, `simulated`, `generated`, `external`);
- source memory, parent, birth-boundary and transformation provenance;
- separation of inherited knowledge from autobiographical experience;
- integrity/version/migration checks for persistence;
- deterministic retrieval tests and measured quality benchmarks;
- no hard-coded claim that a small fixed capacity faithfully models human cognition.

The legacy code is therefore a useful behavioral reference, not a direct Genesis memory implementation.

### Benchmark and workflow lessons

Focused legacy tests encode several sound truth boundaries:

- benchmark methods should refuse to invent a score when model responses or dimension measurements are absent;
- an unavailable external connector should return `not_configured`, not simulated success;
- structured detections supplied by an upstream source prove ingestion/processing, not that native perception exists;
- deterministic heuristic reward scoring is control logic, not optimizer-backed learning;
- persistence and sync behavior need restart tests.

Genesis adopts these as verification principles, not as proof that the entire Digital Twin is complete.

### Defer or reject for Genesis core

- modules named with “god level,” “infinite intelligence,” consciousness or reality manipulation are not capabilities by name;
- broad quantum, AGI and whole-brain claims require separate evidence;
- web routes, UI panels and metadata catalogues do not establish engine behavior;
- online automation and model-provider adapters come after identity, security, resource and supervision gates.

## AI conversation corpus

### Recursive improvement

The conversations repeatedly propose a loop resembling idea → generate → run → inspect errors → repair → improve. Genesis converts this into a governed development loop:

1. propose a bounded change against a registered requirement;
2. record source, rationale, predicted benefit and risk;
3. build in an isolated workspace;
4. run deterministic unit and integration tests;
5. run relevant safety and resource checks;
6. benchmark against the unchanged baseline;
7. reject, revise or request review;
8. commit only accepted changes with evidence;
9. retain rollback and regression records.

An organism must never rewrite its operational core merely because generated code compiled. Self-modification remains gated by identity continuity, signed provenance, sandboxing, recovery and demonstrated improvement.

### Other retained research themes

- specialist/multi-expert routing can inform later tissues or organs, but the number of specialists should emerge from measured need rather than an arbitrary diagram;
- feedback and control-system material can inform resource-backed homeostasis;
- biosemiotic material may inform a signal/meaning distinction in optional digital chemistry, provided its computational value is benchmarked;
- self-correction material supports explicit error detection, verifier independence and regression memory;
- benchmark discussions support task-specific, adversarial, integration, performance, recovery and longitudinal tests;
- autonomous-reasoning claims should be traced to original papers and reproduced before adoption.

### Rejected conversation patterns

- claiming placeholders are implemented security or intelligence;
- assuming access to every layer of the web or complete knowledge;
- adding cloud, blockchain, quantum, agents or databases without a requirement and measured benefit;
- using arbitrary percentages as evidence of completion;
- directly embedding credentials or service-specific deployment instructions in the organism core.

## Downloads

Downloads is an authorized discovery pool, not a dependency directory. Installers, archives, books, model sources, media, CAD and application assets are isolated from Genesis by default. A candidate is considered only when a registered requirement needs it, then it receives malware/security checks, embedded-license review, a digest, an isolated extraction, tests and a comparison with a simpler original implementation.

## Deferred legacy-project work

`F:\ai chat conversations` and `F:\AI Digital Twin` remain intact for future dedicated work. Finishing or modernizing those programs is a separate phase from Genesis abstraction and is not represented as completed here.
