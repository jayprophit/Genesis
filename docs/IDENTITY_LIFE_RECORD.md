# Identity-bound digital life records

Genesis 0.25 implements the bounded software contracts for
`REQ-LIFE-RECORD-001` and `REQ-LIFE-RECORD-PERSIST-001`. A
`DigitalLifeRecord` is an append-only, evidence-referenced history bound to one
organism identity, one immutable lineage anchor and typed registry entities. It
is not a government identity document, passport, credential wallet, legal
personhood claim or source of action authority.

## Identity anchors

Each record permanently carries:

- the complete `LineageIdentity` and a derived SHA-256 lineage-anchor digest;
- the typed organism entity address and a distinct typed record address;
- the entity-registry namespace in which both addresses were registered; and
- a bounded entry capacity fixed when the record is created.

The first entry must be one exact birth assertion. Its effective time,
continuity event and birth-snapshot digest must agree with the immutable
lineage identity. A second birth entry is rejected. Names, associations,
residences, embodiments and later lifecycle facts never replace the organism
address, birth event, lineage identity or birth snapshot.

## Entry model

The append-only schema distinguishes birth, name, milestone, development,
education, training, skill, competence, employment, project, research,
achievement, certification, license, interest, community role, association,
residence, embodiment, lifecycle, retirement, legacy and explicitly custom
records. Serialized enum values are append-only migration boundaries.

Every entry records:

- its evidence class, visibility label and assertion, supersession or
  retraction disposition;
- digests for the value, supporting evidence and referenced authorization
  evidence;
- the typed source entity and any typed related entity;
- its effective interval and monotonically ordered recording time;
- the matching autobiographical-continuity event ID and digest; and
- its sequence, previous-entry digest and deterministic entry digest.

Evidence classes describe whether the source is self-claimed, taught,
observed, tested, certified, an official record or derived. They are not trust
scores. A digest proves neither authenticity nor authorization by itself.
Visibility is retained as policy input; this module does not yet enforce
private, trusted-shared or public-summary disclosure.

## Temporal history and references

An entry can be superseded or retracted only by a later entry of the same kind,
and one target cannot acquire multiple successors. Name history permits one
open recognized name at a time, preserves the original name, supports
effective-time views and requires an explicit retraction before a new name
history is started.

All source and related entities must exist when the entry is recorded. The
first typed related-entity rules are deliberately narrow:

- an association requires a related entity;
- an embodiment must reference a shell;
- a residence must reference a place;
- a certification or license must reference a credential; and
- a birth or name entry cannot attach a related entity.

`audit_references` checks the registry, identity anchors, endpoint types and
registration times. `audit_continuity` independently checks that referenced
autobiographical events belong to the same organism and match their recorded
digests and times. Structural verification rebuilds and checks the digest
chain, sequence, temporal, birth and name-history invariants.

## Authority and trust boundary

Every returned `LifeRecordView` explicitly reports
`action_authorized=false`, `organism_identity_reassigned=false` and
`credential_cryptographically_verified=false`. The
`authorization_evidence_digest` is only a reference for a future policy
decision. It is not validated authorization. Likewise, credential-shaped
entries do not implement issuer signatures, revocation, selective disclosure
or legal validity.

For this reason the core life-record requirement is scored at 90%, with its
security gate still open. Deny-by-default disclosure policy, an operationally
qualified cryptographic provider, provider-backed key handling, authentication,
authorization and credential validation must be implemented and qualified
separately before that gate can close.

## Immutable persistence

`LifeRecordStore` uses the shared `ImmutableSnapshotFiles` boundary also used
by `EntityRegistryStore`. Domain stores keep responsibility for schema parsing,
whole-record SHA-256 checksums and identity binding, while the shared primitive
performs bounded reads, durable temporary-file flush and atomic
no-replacement publication.

The life-record schema serializes the complete lineage identity, anchor,
namespace and typed entity bindings, capacity and all entries. Recovery rejects
unsupported schemas, corruption, invalid enum or optional encodings, malformed
lengths, trailing bytes, unsafe paths, oversized records, immutable conflicts,
and mismatched record, organism or lineage bindings. Identical writes are
idempotent; different bytes for the same record/version conflict. Host
filesystem and storage hardware still determine ultimate power-loss behavior.

## Evidence

`genesis_life_record_tests` covers all enum codecs, the exact and unique birth
boundary, pre-birth effective-date rejection, immutable identity,
original/current/effective-time names,
supersession, retraction and restart, duplicate-successor and parallel-name
rejection, source and typed-reference rules, registry and continuity audits,
capacity, deterministic serialization, corruption, future schemas,
idempotence, conflicts, concurrent writers, non-regular targets, path and size
limits, and organism/lineage recovery bindings.

The local Windows benchmark appends 10,000 entries and exactly reconstructs a
7,108,659-byte immutable snapshot. The recorded run built the record in 359 ms
and completed the durable write, read and verification round trip in 586 ms.
The deterministic snapshot digest was
`a737ba36a83f6084dac7d6fd6500eb3e7a279686358c839258b660189493821c`.
These are local reference measurements, not portable latency guarantees.
