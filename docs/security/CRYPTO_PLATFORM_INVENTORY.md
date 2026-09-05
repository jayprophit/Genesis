# Read-only cryptographic platform inventory

Architecture version: 0.28.0

## Purpose and boundary

Genesis can now observe which CNG key storage providers (KSPs) are registered
on a Windows host without opening a provider or touching a key. This closes the
first observation step for `REQ-CRYPTO-PLATFORM-INVENTORY-001`; it does not
qualify a provider, a TPM, an algorithm, a cryptographic module, or the host.

The native backend makes exactly one silent
[`NCryptEnumStorageProviders`](https://learn.microsoft.com/en-us/windows/win32/api/ncrypt/nf-ncrypt-ncryptenumstorageproviders)
call. Microsoft documents that API as returning registered KSP names and
requires its result buffer to be released with
[`NCryptFreeBuffer`](https://learn.microsoft.com/en-us/windows/win32/api/ncrypt/nf-ncrypt-ncryptfreebuffer).
Provider names are converted directly from Windows UTF-16 to UTF-8 with
[`WideCharToMultiByte`](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte)
using invalid-character rejection.

Microsoft also warns that provider enumeration must not be called from a
service's `StartService` processing because it can deadlock. Genesis therefore
exposes collection as an explicit call; service integrations must invoke it only
after startup and outside that lifecycle callback.

The probe does not call `NCryptOpenStorageProvider`, enumerate stored keys, open
or create a key, request a TPM operation, export material, or execute
cryptography. Its evidence record permanently sets all corresponding fields to
false, and verification rejects a record if any caller changes them to true.

## Stable C++ contract

`platform_crypto_inventory.hpp` exposes:

- an append-only status enum with `observed`, `unsupported_platform`, and
  `enumeration_failed` states;
- a bounded draft-to-canonical factory that sorts provider names, rejects
  duplicates and control text, and binds names to SHA-256 evidence digests;
- a self-verifying inventory record with explicit negative capability claims;
- the native, read-only probe; and
- deterministic single-line JSON for local evidence capture.

At most 256 provider names and 4,096 UTF-8 bytes per name are admitted. Empty,
ASCII/C1-control-bearing, malformed, overlong, surrogate and out-of-range UTF-8
input is rejected. A failed enumeration cannot retain partial provider data.
Unsupported operating systems produce a valid explicit `unsupported_platform`
record instead of a simulated Windows result.

## Observed local evidence

On 2026-09-05, the release probe observed four registered provider names on the
local Windows x86-64 host in 428 microseconds. The evidence record digest was
`9eda5da964524d050753a009006e84bc95bf26f4b2d10cf580a85ca03759bfd0`.
The registrations were:

- Microsoft Passport Key Storage Provider;
- Microsoft Platform Crypto Provider;
- Microsoft Smart Card Key Storage Provider; and
- Microsoft Software Key Storage Provider.

This is machine- and time-specific registration evidence. The
[Microsoft CNG KSP overview](https://learn.microsoft.com/en-us/windows/win32/seccertenroll/cng-key-storage-providers)
explains the role of these provider categories, but neither that documentation
nor local registration proves that a provider opens successfully, that a TPM is
present or healthy, that an exact module or operational environment has a valid
certificate, or that Genesis can safely perform an operation.

Run the read-only probe with:

```powershell
./build/genesis_crypto_platform_probe.exe
```

The output contains registration names and digests but no keys, handles,
credentials, secrets, or key locators. Host evidence is printed to standard
output and is not automatically persisted or committed.

## Verification

`genesis_platform_crypto_inventory_tests` covers enum migration stability,
canonical ordering, duplicate and control-text rejection, status/evidence
consistency, digest tamper detection, rejection of provider-open and
qualification claim escalation, deterministic JSON boundaries, the real
Windows enumeration route, and the explicit non-Windows fallback.

Repeated enumeration is intentionally not presented as a performance
benchmark: it would measure an environment-sensitive operating-system registry
query and could add provider-side load without demonstrating cryptographic
throughput. The single observation records latency only as diagnostic evidence.

## Remaining gates

Registration is the first state, not qualification. Before any key operation is
enabled, Genesis still requires at least:

1. an exact provider/module artifact identity and applicable operational
   environment;
2. license, provenance, supply-chain, certificate, algorithm and threat-policy
   review;
3. a bounded provider-open observation that remains separate from key access;
4. misuse-resistant, non-exportable native-handle adapters with authenticated
   actors and deny-by-default authorization;
5. failure, zeroization, rotation, recovery and deletion evidence;
6. fuzzing, sanitizer, coverage, external security and named-platform evidence;
   and
7. explicit qualification records tied to the exact route.

Until those gates close, `provider_qualified`,
`cryptographic_operation_executed`, `identity_authenticated`, provenance
authentication, and action authorization remain false.
