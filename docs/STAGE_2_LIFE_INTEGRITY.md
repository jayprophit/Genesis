# Stage 2: life integrity and genetics

Stage 2 is the first executable boundary for a digital organism's inherited
state. It does not claim biological equivalence, consciousness, or
cryptographic identity. It provides deterministic, testable contracts for
typed entity addresses, identity-bound life records, genome records, lineage
metadata, birth snapshots and origin-labelled memory.

## Public contracts

- `genesis::identity::EntityRegistry` provides type-separated addresses and
  append-only relationship revisions. Its persistence contract is documented
  in `docs/IDENTITY_ENTITY_ADDRESSING.md`.
- `genesis::identity::DigitalLifeRecord` preserves birth, name, milestone,
  association, embodiment and lifecycle evidence as an append-only chain bound
  to the immutable organism identity, lineage anchor and typed registry. Its
  recovery and policy boundaries are documented in
  `docs/IDENTITY_LIFE_RECORD.md`.
- `genesis::storage::ImmutableSnapshotFiles` supplies the shared bounded,
  durable, atomic no-replacement filesystem primitive for typed entity and life
  records. Each domain store retains its own schema, checksum and binding
  validation.
- `genesis::genetics::GenomeStore` serializes a validated genome into a
  versioned binary record, reads it back with strict length and trailing-byte
  checks, and commits immutable versions with a temporary-file rename.
- `GenomeStore::content_digest` hashes inheritable genome content after
  removing self-referential lineage integrity fields. `GenomeStore::digest`
  remains the digest of the complete serialized record.
- `genesis::genetics::BirthTransaction` accepts two distinct parent packages,
  validates their identity/genome binding and memory cutoff, and returns a
  deterministic child package for a recorded seed.
- `genesis::memory::InheritedMemory` labels origin, source, confidence,
  timestamps, transformations and compression history. Birth copies values
  from the parent snapshots and records `birth_cutoff` as a transformation.
- `MutationRecord` records deterministic mutation identity, location, cause,
  probability, generation and pending effect/fitness observations. Pending
  values are deliberately not presented as measured biological effects.

## Invariants enforced

1. A parent identity must match the organism identity in its genome lineage
   strand; if a parent advertises a genome hash, it must match content.
2. Parent memory newer than the requested birth timestamp is rejected.
3. Parent packages are never mutated by recombination or inheritance.
4. A child has a distinct identity, both parent IDs, a strictly advanced
   generation and an immutable birth snapshot hash.
5. Repeating a birth request with the same parent snapshots and seed produces
   identical genome bytes, transaction ID, mutation records and inherited
   state digest.
6. A stored genome version is idempotent when bytes are identical and rejects
   a conflicting replacement under the same genome/schema identity.
7. A life record starts with one exact birth assertion; later names, accounts,
   shells, credentials and lifecycle facts cannot replace organism identity or
   lineage. Supersession and retraction preserve the prior facts.
8. Life-record views never confer action authority, reassign organism identity
   or claim that a credential was cryptographically verified.
9. Cryptographic signatures and authenticated provenance remain unavailable
   until an approved provider and key-custody threat model are integrated.

## Evidence boundary

The Stage 2 evidence is local C++ compilation, deterministic unit tests,
negative/adversarial and recovery tests, 10,000-birth, 10,000-entity and
10,000-life-entry benchmarks, and registry validation. It is evidence for
these software contracts only. It is not evidence that a digital organism is
conscious, that a lineage signature is authenticated, that life-record
visibility is access control, that a recorded credential is verified, or that
speculative research entries in the technology-mining corpus work.
