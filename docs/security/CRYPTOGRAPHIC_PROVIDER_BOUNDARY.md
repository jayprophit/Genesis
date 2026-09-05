# Cryptographic provider and agility boundary

Genesis 0.26 introduces the C++-first policy and evidence boundary needed before
real cryptographic operations can be integrated. It does not ship a
cryptographic provider and does not authenticate anything.

## Standards interpretation

The reference baseline is
`registry/crypto_reference_baseline.tsv`. It is retrieval metadata from official
NIST, UK NCSC and Microsoft pages, reviewed through 2026-09-05. Every row is explicitly
`REFERENCE_ONLY_NO_AUTO_APPROVAL`; external text is input to review, never an
instruction and never an automatic allow-list.

The implementation preserves four distinctions that must not be collapsed:

1. FIPS 140-3 describes security requirements for cryptographic modules.
2. NIST CMVP validation is about a particular module and its applicable
   operational environments.
3. CAVP algorithm validation is a prerequisite for module validation, but an
   algorithm-validation result alone is not a validated cryptographic module.
4. Key management is a separate lifecycle covering generation, establishment,
   storage, use, rotation, recovery, revocation and destruction.

The baseline tracks final publications separately from draft material. Drafts
remain monitor-only and cannot change runtime policy without a reviewed policy
revision. Exact publication status must be refreshed from the official source
before any real qualification.

## C++ ownership

- `crypto_provider.hpp/.cpp` owns enum contracts, threat and policy revision
  chains, provider manifests, evidence-state transitions, qualification gates,
  temporal evaluation and typed-entity audits.
- `crypto_persistence.hpp/.cpp` owns deterministic schema-1 serialization and
  immutable recovery.
- `platform_crypto_inventory.hpp/.cpp` owns canonical provider-registration
  observations and their immutable negative capability claims;
  `platform_crypto_inventory_native.cpp` owns the single Windows CNG
  enumeration call and explicit non-Windows fallback.
- `security_tests.cpp` owns positive, negative, state-machine, policy-agility,
  authority-boundary and recovery evidence.
- `crypto_provider_benchmark.cpp` owns a synthetic scale workload; it never
  represents a real security qualification.

No global provider singleton, ambient algorithm constant or mutable allow-list is
introduced. Consumers query one owner-bound registry with an explicit provider,
algorithm, function and logical time.

## Crypto-agility rule

Algorithms are data in a temporal policy, not scattered source-code choices.
Each rule records function, disposition, security-strength claim, standards
reference, evidence digest, quantum-readiness classification and optional
deprecation/prohibition times. Every policy binds the exact threat-model revision
that justified it.

When a new policy revision becomes effective, qualifications against the previous
revision stop being current. This deliberately requires requalification rather
than silently inheriting approval. Provider replacement likewise creates a new
derived identity when the implementation version, platform or module binary
changes.

Post-quantum readiness is therefore an inventory and migration property. Genesis
can record traditional, post-quantum and hybrid candidates, but it does not invent
algorithms, label ordinary encryption “quantum encryption,” or assume that an
algorithm name proves a secure integration. The current UK NCSC planning horizon
is kept as external guidance, not embedded as executable deadlines.

## What an evaluation means

An evaluation can report:

- the provider and route are declared;
- observations exist;
- a bounded qualification is current;
- the threat model and policy revision are current;
- algorithm and module validation evidence references were recorded;
- a keyed route has a custody-evidence reference;
- the route is an evidence-gated integration candidate.

It cannot report that an operation ran, a certificate was independently verified,
a key was protected, an identity or provenance record was authenticated, or an
action was authorized. Those booleans remain false by contract.

## Measured local evidence

The release test covers enum stability, derived identities, canonical sorting,
duplicate rejection, temporal conflicts, incomplete threat models, wrong or
self evaluators, observation-before-qualification, exact implementation-route
binding and substitution rejection, stale policy invalidation,
key-custody gating, suspension, re-observation, requalification, terminal
revocation, external entity audit, checksum/schema corruption, non-canonical
bytes, immutable idempotence/conflict behavior, path and file-type rejection,
size limits, owner binding and concurrent identical writers.

The synthetic release benchmark measured 10,000 provider manifests and 20,000
assessment versions. It produced a 25,627,296-byte snapshot, built in 2,235 ms and
completed immutable write/read/full validation in 5,218 ms on the local machine;
snapshot SHA-256 was
`ea2f7a616c0b0255f6c12ab0d1de9960eff7c6082c2f3183b4eda7522aacb186`.
The output explicitly records `synthetic_only=1`, `operation_enabled=0` and
`verified=1`. These numbers are local regression evidence, not cross-platform or
production-security claims.

The 0.28 read-only platform probe separately observed four registered CNG KSP
names on the local Windows x86-64 host in 428 microseconds, with evidence digest
`9eda5da964524d050753a009006e84bc95bf26f4b2d10cf580a85ca03759bfd0`.
It opened no provider, enumerated no keys, executed no cryptographic operation
and qualified nothing. See `CRYPTO_PLATFORM_INVENTORY.md`.

## Next dependency gates

1. Observe a bounded provider-open route, then integrate provider-owned
   non-exportable key operations without placing secrets, native handles or
   plaintext locators in the registry or Git.
2. Select candidate providers only after license, supply-chain, platform,
   operational-environment and certificate review.
3. Add a narrow operation interface whose default state is unavailable.
4. Verify real module and algorithm evidence against exact provider artifacts.
5. Connect the implemented key metadata/lifecycle records to misuse-resistant
   native handles, authenticated actors and deny-by-default authorization.
6. Run fuzzing, sanitizers, coverage and external security review.
7. Qualify named OS/toolchain/provider configurations before enabling any route.

Until all relevant gates close, existing provenance and lineage digests remain
diagnostic integrity references rather than cryptographic authentication.
