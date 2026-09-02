# Stage 2: life integrity and genetics

Stage 2 is the first executable boundary for a digital organism's inherited
state. It does not claim biological equivalence, consciousness, or
cryptographic identity. It provides deterministic, testable contracts for
genome records, lineage metadata, birth snapshots and origin-labelled memory.

## Public contracts

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
7. Cryptographic signatures and authenticated provenance remain unavailable
   until an approved provider and key-custody threat model are integrated.

## Evidence boundary

The Stage 2 evidence is local C++ compilation, deterministic unit tests,
negative/adversarial tests, a 10,000-birth benchmark and registry validation.
It is evidence for these software contracts only. It is not evidence that a
digital organism is conscious, that a lineage signature is authenticated, or
that speculative research entries in the technology-mining corpus work.
