# Cryptographic provider registry threat model

Status: implemented foundation, security review still open

Architecture version: 0.26.0

Last reviewed: 2026-09-05

## Scope

This threat model covers the Genesis registry that records cryptographic policy,
provider declarations, observations, qualifications, suspensions and
revocations. It covers integrity and recovery of those evidence records. It does
not cover secret-key generation or storage, cryptographic operations, identity
authentication, signature verification, remote protocols, secure boot, device
attestation or authorization. Those remain separate fail-closed requirements.

The registry is an evidence and decision-support boundary. A positive decision
means only that a provider route is an `integration_candidate` under the exact
recorded policy, threat-model revision, platform declaration and evidence set.
The following outputs are permanently false in this version:

- `cryptographic_operation_available`
- `identity_authenticated`
- `provenance_authenticated`
- `action_authorized`

## Assets

The protected assets are:

1. the registry identity and its binding to one typed owner entity;
2. threat-model and algorithm-policy revision histories;
3. provider implementation, version, platform and module-boundary identity;
4. functional, platform, module-validation and algorithm-validation references;
5. evaluator identity and independence evidence;
6. provider state transitions and terminal revocations;
7. immutable recovery bytes and their schema, checksum and owner binding;
8. truthful capability boundaries that prevent evidence records from becoming
   operational authority.

Keys, passwords, tokens, seeds, private key material and recovery secrets are not
registry assets because the registry must never receive or serialize them.

## Trust boundaries

```text
official primary references (untrusted input, reference-only)
                    |
                    v
       temporal policy and threat review
                    |
provider manifest -> observed route -> independent qualification
                    |                         |
                    +------ append-only ------+
                               |
                     immutable snapshot store
                               |
                     evidence-only evaluation
                               |
               no operation, authentication or authority
```

The caller, filesystem, provider manifest, evaluator assertions, external
validation references and retrieved standards metadata are all untrusted. The
typed entity registry is an independently validated dependency but can change
outside this module, so `audit_entities` rechecks owner and evaluator references.

## Threats and controls

| Threat | Required control in 0.26 |
|---|---|
| Supply-chain substitution | Provider ID binds implementation name, version, platform and module-binary digest; manifests also record source, license and build evidence digests. |
| Binary or module-boundary tampering | Manifest identity and chained assessment digests bind module and implementation evidence; changed bytes require a new provider identity. |
| Rollback | Threat, policy and assessment histories are append-only and temporally monotonic; immutable file versions cannot be replaced. |
| Replay | Assessment IDs are globally unique within the registry, state transitions are ordered and a qualification must reference the policy current at record time. |
| False or self qualification | Qualification requires a separately registered organization entity; the registry owner and person entities cannot qualify a provider. |
| Algorithm validation presented as module validation | Route-level algorithm evidence and provider-level module evidence are separate mandatory fields for qualification. |
| Stale standard or algorithm status | Policies are time-bounded revisions. A newer effective revision immediately invalidates a qualification tied to the older revision. |
| Algorithm break or quantum transition | Rules carry temporal dispositions and quantum-readiness classification; policy replacement is supported without changing consumer interfaces. |
| Weak randomness, key theft or key loss | These threats are mandatory before qualification, but actual entropy and custody controls remain unimplemented external gates. Keyed routes require a custody-evidence digest to become candidates. |
| Misconfiguration or route substitution | Provider, function, algorithm, implementation-route identifier and implementation digest must agree across the manifest, observation and qualification; policy must independently allow the algorithm/function pair. |
| Compromised device or platform | Platform evidence is recorded per route, but no platform is qualified by this repository yet. |
| Malicious dependency | Source/build/license evidence digests are required; software-composition analysis and external review remain open. |
| Side channel or implementation flaw | The model can record the threat and evidence, but measurement, fuzzing, constant-time review and sanitizer/platform qualification remain open. |
| Filesystem corruption | Schema magic, bounded lengths/counts, enum ranges, a payload checksum, internal digest chains and full reconstruction validation reject damaged records. |
| Unsafe file target or traversal | Shared immutable storage validates identifiers, rejects non-regular/link targets, bounds reads/writes, durably flushes temporary bytes and atomically publishes without replacement. |
| Evidence used as permission | Public evaluation returns evidence flags only and hard-codes all operation, authentication, provenance and authorization outputs to false. |

Qualification requires explicit coverage of supply-chain compromise, binary
tampering, rollback, key theft, weak randomness, algorithm break and quantum
cryptanalysis. Other categories can be added to a model without changing the
persistence schema.

## Provider evidence state machine

```text
declared --observation--> observed --qualification--> qualified
    |                         |              |
    +------revocation--------+--------------+----> revoked (terminal)
                              \             /
                               suspension
                                   |
                              re-observation
```

A declaration is never approval. A qualification cannot be recorded before a
route observation. A suspended provider needs a new observation and
qualification. A revoked provider cannot be revived; a replacement must have a
new manifest-derived provider identity.

## Qualification conditions

For each route, the registry requires all of the following before it can record a
qualification:

- the provider manifest declares the same algorithm/function pair, exact
  implementation-route identifier and implementation digest;
- a prior observation exists for the route;
- the referenced policy revision is current at assessment time;
- the exact referenced threat-model revision is current and covers the mandatory
  qualification threats;
- the algorithm rule is `approved` and not prohibited at assessment time;
- the qualification repeats the functional and platform evidence digests from a
  prior observation of that exact implementation route;
- algorithm-validation reference and evidence digest are present;
- module-validation reference and evidence digest are present;
- the qualification expiry does not outlive policy review;
- a separately registered organization is the evaluator.

These are structural and provenance gates, not proof that the referenced
certificate exists or applies to the exact binary. Human/security review and
machine-verifiable certificate ingestion remain required before a real provider
can be accepted.

## Recovery properties

`CryptoProviderStore` uses schema `GENESIS-CRYPTO-PROVIDER-REGISTRY`, version 1.
It serializes no key material. Deserialization bounds every field, count,
capacity, enum and optional value; rebuilds indexes; verifies every revision and
digest chain; and rejects non-canonical encodings. The shared immutable store
adds durable flush, publish-without-replacement, idempotent same-byte writes,
conflict detection, bounded reads and file-type/path protection.

The checksum is corruption detection, not a message-authentication code or
signature. An attacker who can replace both payload and checksum is outside the
protection of this layer. Authenticated storage, OS access control, signed release
artifacts and key custody remain open.

## Residual risk and open gates

- No real cryptographic library is linked, selected or approved.
- No real validation certificate has been verified against a module binary and
  operational environment.
- No secret-key lifecycle, hardware-backed custody, rotation, escrow, recovery
  ceremony or destruction mechanism exists.
- No entropy-source health test or random-generation route is operational.
- No fuzzing campaign, external security review, constant-time analysis or
  sanitizer/platform qualification has been completed for this module.
- Local SHA-256 values currently provide deterministic integrity references only.
- The test and benchmark records use unmistakably synthetic evidence strings and
  are not security qualifications.

Accordingly, `REQ-CRYPTO-001` remains open even though the provider
qualification registry and immutable recovery foundation are implemented.
