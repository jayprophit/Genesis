# Typed entity addressing and registry recovery

Genesis 0.25 implements the bounded foundation for `REQ-ENTITY-ADDRESS-001`
and `REQ-ENTITY-PERSIST-001`. It gives governed things stable typed addresses
and preserves versioned relationship facts without confusing those facts with
organism identity, authentication, ownership authority or action permission.

## Public contracts

- `genesis::identity::EntityAddress` identifies an organism, shell, component,
  record, place, credential, experiment, account, person, organization,
  dataset, model, service or explicitly custom entity.
- `derive_entity_id` produces a deterministic, type-prefixed address from the
  registry namespace, entity kind and caller-canonicalized local key. The full
  SHA-256 digest is retained rather than truncated.
- `EntityRegistry` rejects malformed evidence digests, duplicate registrations,
  type-changing identifiers, capacity overflow and relations with missing or
  incompatible endpoints.
- `EntityRelation` keeps an append-only revision history. Versions are
  contiguous, immutable identity fields cannot change, record time cannot move
  backward, and an ended, revoked or superseded history cannot be reopened.
- `EntityRegistryStore` writes a schema-tagged, whole-record checksummed binary
  snapshot through the shared `ImmutableSnapshotFiles` primitive. It bounds and
  flushes the temporary file, then publishes without replacing an existing
  version. An identical retry is idempotent; different bytes under the same
  namespace/version are a conflict. The same reviewed file primitive now backs
  the digital life-record store rather than duplicating durability code.

## Identity and authority boundaries

An entity address is a collision-resistant local registry address. It is not a
cryptographic identity credential, global legal identifier or proof that an
entity exists outside the registry. The current SHA-256 implementation supplies
deterministic content addressing and corruption detection only. Authentication
and signatures remain gated by `REQ-CRYPTO-001`; the secret-free key-custody
evidence registry does not close the operational key or authentication gate.

Only an entity registered with `EntityKind::organism` resolves through
`organism_identity`. A name, account, person record, credential, shell or device
can be related to an organism but cannot replace its persistent identity or
lineage. `relations_for` deliberately returns `action_authorized=false` and
`organism_identity_reassigned=false`; a later deny-by-default policy engine must
evaluate any proposed action independently.

The first typed endpoint rules are deliberately narrow:

- `embodies`: organism to shell;
- `located_at`: the object must be a place;
- `issued_by` and `subject_of`: the subject must be a credential;
- `supersedes`: both entities must have the same kind.

Other relations retain their typed endpoints without implying a permission.
Custody, legal ownership, credentials, life records and succession each remain
separate registered increments and are not claimed complete by this foundation.

## Persistence and recovery invariants

1. A persisted registry must contain the declared registrar as an organism
   entity in the same namespace.
2. Entity and relation counts, individual fields and the complete file are
   bounded before reconstruction.
3. Unknown schemas, invalid enums, invalid optional fields, duplicate records,
   relation gaps, dangling endpoints, trailing bytes and checksum corruption are
   rejected.
4. Storage filenames accept only bounded alphanumeric, dot, dash and underscore
   namespace/version components; records must be regular non-link files.
5. Read recovery verifies both the requested namespace and registrar binding.
6. Temporary records are flushed before atomic no-replacement publication.
   This protects immutable snapshots against process interruption and competing
   writers. Ultimate power-loss guarantees still depend on the host filesystem
   and storage hardware.

## Evidence

`genesis_identity_tests` covers enum codecs, deterministic typed separation,
duplicates, retyping, evidence syntax, capacity limits, dangling/self/invalid
relations, endpoint rules, version gaps, terminal rewrites, identity and
authorization separation, deterministic serialization, corruption, future
schemas, path safety, immutable conflicts, idempotence, and namespace/registrar
binding.

Shared-boundary coverage in `genesis_life_record_tests` additionally races two
different writers against one immutable version and verifies exactly one
winner, a classified conflict for the loser, exact winner recovery and
rejection of non-regular targets.

The local Windows benchmark registers 10,000 entities and 20,000 relationship
versions, then persists and exactly reconstructs an 11,079,071-byte snapshot.
The recorded run built the registry in 438 ms and completed the durable write,
read and verification round trip in 1,111 ms. The deterministic snapshot digest
was `4d473e497350ae668107277f568d6b5e59a8e1f4cc2e49c13ae93ff284d408a1`.
These are local reference results,
not portable latency guarantees.
