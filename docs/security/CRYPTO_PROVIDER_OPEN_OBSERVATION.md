# Bounded cryptographic provider-open observation

Architecture version: 0.29.0

Status: integration-tested observation boundary; no key or cryptographic
operation is enabled

Last reviewed: 2026-09-05

## Purpose

Genesis can now distinguish a registered Windows CNG key storage provider
(KSP) from one that can actually be loaded at a particular moment. The
`REQ-CRYPTO-PROVIDER-OPEN-001` route opens one caller-selected provider only
after its exact name appears in a verified registration inventory, reads one
provider-level implementation-type property, releases the provider handle, and
records the result.

This is an observation, not a qualification. It does not enumerate, open,
create, import, export, use, rotate, recover, or delete any key. It does not
perform encryption, decryption, signing, verification, hashing, key agreement,
random generation, or attestation. It does not authenticate an identity,
authenticate provenance, authorize an action, verify a module certificate, or
declare the operating environment secure.

## Native call boundary

On Windows, the implementation performs this bounded sequence:

1. verify the canonical provider-registration inventory and its digest;
2. validate the requested provider name as bounded canonical UTF-8;
3. require the provider name to match an observed registration exactly;
4. convert that exact name to UTF-16 with invalid-input rejection;
5. call
   [`NCryptOpenStorageProvider`](https://learn.microsoft.com/en-us/windows/win32/api/ncrypt/nf-ncrypt-ncryptopenstorageprovider)
   with the API-required zero flags;
6. call
   [`NCryptGetProperty`](https://learn.microsoft.com/en-us/windows/win32/api/ncrypt/nf-ncrypt-ncryptgetproperty)
   once for `NCRYPT_IMPL_TYPE_PROPERTY`, passing `NCRYPT_SILENT_FLAG` so the
   property query cannot request interactive UI;
7. release a successfully opened provider handle with
   [`NCryptFreeObject`](https://learn.microsoft.com/en-us/windows/win32/api/ncrypt/nf-ncrypt-ncryptfreeobject);
   and
8. canonicalize the result into self-verifying, single-line JSON evidence.

Microsoft documents provider open as loading and initializing a KSP; it is not
described here as a registry-only read. The open function defines no flags, so
Genesis passes zero. The only queried value is the provider-level DWORD
[`NCRYPT_IMPL_TYPE_PROPERTY`](https://learn.microsoft.com/en-us/windows/win32/seccng/key-storage-property-identifiers).
Recognized provider-reported bits are hardware, software, removable, hardware
random-number generator, and virtual isolation. The raw DWORD is retained and
unrecognized bits are separated rather than silently discarded.

Both open and property-query documentation warn against calling these APIs from
a service's `StartService` processing because a deadlock can occur. Genesis
therefore exposes an explicit synchronous probe. Any future service adapter must
invoke it only after service startup and outside that callback.

## Fail-closed state model

The append-only status vocabulary is:

- `observed`: the registered provider opened, its implementation type was read,
  and its handle was released;
- `unsupported_platform`: no native provider API was called;
- `inventory_unavailable`: registration could not first be established;
- `not_registered`: the exact requested name was absent, so open was denied
  before the native API;
- `open_failed`: the provider returned a native open failure and no property
  query followed;
- `property_query_failed`: open succeeded, the property query failed, and the
  provider handle was released; and
- `release_failed`: release returned an error, whether the property query had
  succeeded or failed.

The probe rejects a corrupt inventory, an invalid provider name, and an
observation time earlier than the inventory it depends on. A successful record
requires zero native status codes for open, query, and release. A failure record
cannot retain contradictory positive state. Provider name, registration
inventory, raw statuses, flags, timing, all positive and negative booleans, and
schema identity are bound into the evidence digest.

Every valid record fixes these claims to false:

- `key_enumerated`;
- `key_opened`;
- `key_created`;
- `key_exported`;
- `cryptographic_operation_executed`; and
- `provider_qualified`.

Verification rejects any attempt to turn those fields into capability claims.
Native handles are local temporaries and never appear in the public record,
JSON, repository, registry, or logs.

## Local Windows evidence

Two release probes were run on the local Windows x86-64 desktop on 2026-09-05.
Each run first captured its own verified registration inventory, so its
time-and-latency-bound inventory digest is intentionally different.

### Software KSP

`Microsoft Software Key Storage Provider` matched a registration, opened,
returned implementation flags `0x00000002` (provider-reported software), and
released successfully. The bounded route took 7,896 microseconds. Its evidence
digest was
`8942db4acf2490e18e0aee6737a30cb3e4dada6f7519240cdb298756dffe7e2b`.

This proves only that this named KSP completed the open/property/release route
in that process at that time. It does not approve the provider or demonstrate
the safety, persistence, non-exportability, correctness, or authorization of a
key operation.

### Platform/TPM KSP

`Microsoft Platform Crypto Provider` also matched a registration, but its open
attempt returned `0x80090030` after 13,239 microseconds. Microsoft identifies
that status as `NTE_DEVICE_NOT_READY`: the device required by the provider was
not ready for use. No property query followed and no provider handle was
returned for Genesis to release. The evidence digest was
`dba496f5e96b9573fe7dd0bcf2ade26c7aa331fef1916d863de2f37da07af078`.

This is a valuable negative observation: a registered platform provider did
not imply an operational TPM route. It is not, by itself, proof that a TPM is
absent, permanently faulty, disabled, unowned, or unsafe. Genesis performs no
automatic TPM reset, clear, ownership, firmware, BIOS, service, or policy
change in response.

### Cross-toolchain repeat

The same native path was rebuilt with MSVC 19.51 and repeated. The Software KSP
again opened, reported flags `0x00000002`, and released in 5,834 microseconds;
evidence digest
`c73e12a458b2ac27cad16b0008f07d886f411ab1811cf73337421d3fe4894918`.
The Platform KSP again returned `0x80090030`, this time after 12,060
microseconds; evidence digest
`68ba4d1c3ed410953cba7f2f7648eb9928046ac2e177df66c94019ca2d20bfa7`.
The matching categorical result across MinGW and MSVC reduces the chance of a
compiler-specific observation error, but it still does not qualify the host,
provider, device, module, or either toolchain.

Run a probe for one exact registered provider with:

```powershell
./build/genesis_crypto_provider_open_probe.exe `
  "Microsoft Software Key Storage Provider"
```

The command returns zero only for `observed`; all bounded failure states return
nonzero after writing their evidence record. Output is not automatically
persisted or committed.

## Verification

`genesis_provider_open_observation_tests` covers:

- append-only status serialization and unknown-status rejection;
- canonical implementation-flag derivation, including unknown bits;
- deterministic evidence and provider-name digest binding;
- tamper rejection for names, flags, handles, key claims, and evidence digests;
- valid and contradictory open, property-query, release, missing-registration,
  unsupported-platform, and inventory-unavailable state combinations;
- corrupt-inventory and causal-time rejection;
- JSON positive and permanent-negative evidence;
- denial before the native API for an unregistered provider;
- a real Windows Software KSP open/property/release integration route; and
- an explicit no-native-call fallback on non-Windows builds.

There is deliberately no throughput benchmark. Loading an operating-system or
hardware provider repeatedly can create provider-side work and would not
measure cryptographic performance. Each one-shot observation records elapsed
latency as environment-specific diagnostic evidence instead.

## Remaining gates

Before any native key operation can be enabled, Genesis still requires:

1. exact provider/module binary and operational-environment identity;
2. licensing, supply-chain, signature, certificate, algorithm-policy, and
   threat review for that exact route;
3. an independently reviewed qualification tied to current observation;
4. an authenticated actor and deny-by-default authorization decision;
5. a narrow provider-owned, non-exportable key adapter with explicit create,
   open, use, rotate, recovery, zeroization, and destruction semantics;
6. misuse, UI, cancellation, crash, timeout, partial-failure, and recovery
   tests without recording secrets or native handles;
7. fuzzing, sanitizers, coverage thresholds, external security review, and a
   named OS/toolchain/provider/device qualification matrix; and
8. signed and authenticated evidence persistence where security decisions rely
   on records across process or host boundaries.

Until those gates close, this module cannot turn the existing provider registry
or key-custody metadata into an executable key route.
