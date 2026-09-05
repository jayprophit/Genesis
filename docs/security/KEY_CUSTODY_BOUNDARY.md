# Key custody metadata and operation boundary

## Status

Genesis 0.27 implements an owner-bound, secret-free key-custody metadata
registry. It records the lifecycle and evidence for opaque external key handles;
it does not generate, import, export, store, recover, destroy or use key material.

The implemented result is a control and evidence layer. A successful operation
preflight is only an `evidence_gated_execution_candidate_only`. It is not proof
that an operation ran, a key exists in a provider, an actor was authenticated,
provenance was authenticated, or an action was authorized.

## Permanent secret boundary

`KeyHandleManifest` can persist:

- owner, provider, algorithm, function and exact implementation-route identity;
- a SHA-256 digest of an external provider-owned locator;
- origin and export-policy classifications;
- permitted-use classifications;
- recorded custodian, recovery-authority and operator entity IDs;
- custody-policy, creation and attestation evidence digests;
- cryptoperiod, generation and predecessor metadata.

It has no field for:

- symmetric, private, seed or derived key bytes;
- a plaintext provider key name or locator;
- a PIN, password, passphrase or recovery secret;
- an API token, provider credential or authentication cookie;
- an encrypted or wrapped key blob;
- an ephemeral provider-native handle.

The external locator digest detects a changed binding in Genesis metadata. It is
not a locator, authentication tag or proof that a provider protects the
referenced object. Provider adapters will eventually resolve their own opaque
locators through a separate non-persistent process boundary.

A digest does not make a predictable locator secret: an attacker can test
guesses offline. Future adapters must therefore derive this digest from a
provider-scoped, high-entropy opaque locator (or a similarly non-enumerable
binding), and must never hash a human-readable key name as if hashing alone
provided confidentiality.

## Lifecycle

```text
provisioned -> active -> suspended -> active
                   |          |
                   +--------> retired -> compromised -> destroyed
                   |                         ^
                   +-------------------------+

provisioned -> compromised -> destroyed
provisioned ----------------> destroyed
destroyed -> terminal; revival is forbidden
```

Activation and resume require all of the following at the transition's event
time:

1. the key metadata binds a declared provider capability exactly;
2. the provider route is currently an evidence-gated integration candidate;
3. algorithm, function, implementation route and implementation digest match;
4. the qualification's custody-evidence digest equals the key's custody-policy
   digest;
5. the transition is within the half-open cryptoperiod `[not_before,
   not_after)`;
6. the recorded actor has the required custody role.

These checks do not authenticate the actor. Entity IDs and role membership are
evidence-bearing registry facts; a future authentication and authorization
boundary remains mandatory before operational use.

## Usage and preflight

Key usages are typed and restricted by cryptographic function. For example, a
digital-signature handle may be registered for `sign` and/or `verify`, but it
cannot silently become an encryption or wrapping key.

Preflight denies by default and evaluates:

- registry validity and owner/namespace binding;
- actor existence and recorded role membership;
- current lifecycle state;
- cryptoperiod;
- requested typed usage;
- a bounded operation-context digest;
- current provider qualification;
- exact route and implementation binding;
- exact custody-policy evidence binding.

Even when every check passes, these fields remain false:

```text
cryptographic_operation_executed = false
identity_authenticated = false
provenance_authenticated = false
action_authorized = false
```

## Rotation and recovery lineage

A successor must:

- name an already registered predecessor;
- have generation `predecessor.generation + 1`;
- retain the same owner and cryptographic function;
- use a different external-locator digest;
- be active before a succession link is recorded.

A rotation record requires the recorded custodian, a retired predecessor and a
non-recovery successor. A recovery record requires the separately recorded
recovery authority, a predecessor with an actual compromise event, and a
successor whose origin is `recovered`. A destroyed record is terminal; recovery
creates a new identity rather than reviving it.

One predecessor and one successor may occur in at most one succession link. The
succession ledger is ordered, append-only and digest-chained.

## Persistence and recovery

`KeyCustodyStore` writes canonical, checksummed, immutable snapshots through the
shared atomic snapshot boundary. Restore enforces:

- magic and schema version;
- bounded fields, collection sizes and capacities;
- enum ranges;
- checksum, canonical encoding and full-input consumption;
- derived registry and key identities;
- owner binding;
- transition and succession chains;
- lifecycle, cryptoperiod, role and lineage invariants;
- duplicate and conflicting-version rejection;
- regular-file and path-identifier constraints.

The checksum detects accidental or untrusted-record corruption; it is not a
signature or message authentication code. Authenticated snapshots remain gated
on operational cryptography and key custody.

## Standards baseline

The design topics are informed by, but do not claim conformance to:

- [NIST SP 800-57 Part 1 Revision 5](https://csrc.nist.gov/pubs/sp/800/57/pt1/r5/final),
  covering key-management functions, protection, cryptoperiods, inventory and
  recovery considerations;
- [NIST SP 800-130](https://csrc.nist.gov/pubs/sp/800/130/final), a framework for
  cryptographic key-management system design specifications;
- [NIST SP 800-133 Revision 2](https://csrc.nist.gov/pubs/sp/800/133/r2/final),
  the current final key-generation recommendation in the recorded baseline;
- [NIST SP 800-133 Revision 3 initial public draft](https://csrc.nist.gov/pubs/sp/800/133/r3/ipd),
  monitored as draft input only;
- [NIST key-management publications](https://csrc.nist.gov/Projects/Key-Management/publications),
  used to distinguish final material from changing drafts;
- [Microsoft CNG key storage providers](https://learn.microsoft.com/en-us/windows/win32/seccertenroll/cng-key-storage-providers),
  recorded only as platform documentation for a possible future Windows route.

Every external row remains `REFERENCE_ONLY_NO_AUTO_APPROVAL`. Microsoft CNG,
including its software, smart-card and platform/TPM key storage providers, is
not observed or qualified merely because the operating system documents it.

## Remaining operational gates

Before any key route becomes operational Genesis still requires:

1. a provider adapter with RAII-owned native handles and zeroization review;
2. observed provider/KSP inventory and exact binary/platform evidence;
3. real generation, open, use, rotation, deletion and failure-behaviour tests;
4. authentication and deny-by-default authorization above entity-role facts;
5. entropy and random-generation qualification;
6. crash and power-loss testing around provider and metadata transactions;
7. secure backup/recovery policy or an explicit non-recoverable-key policy;
8. fuzzing, sanitizers, coverage thresholds and external security review;
9. named operating-system, toolchain, module and hardware qualification;
10. authenticated migration and recovery records.
