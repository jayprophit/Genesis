# Genesis v0.6 Hardened Checkpoint Loader — Release Review

**Status:** `GENESIS_V06_HARDENED_LOADER_REVIEW_PUBLISHED`

**Review branch:** `review/genesis-v06-loader-hardening-20261030`
**Base commit:** `0de6bb6525f60fea2d2dd64532fc652be7475ded` (branch `audit/genesis-v06-evaluation-repair`)
**Review commit:** `1695ac84fc20c38dca3bd16b23d965247b593097` (verified on `origin/review/genesis-v06-loader-hardening-20261030`)

---

## 1. Committed files

| File | Status | Purpose |
|---|---|---|
| `genesis_model/checkpoint.py` | Modified (from `5ea1b76…`) | Fail-closed loader with byte-hash provenance keying |
| `genesis_model/tokenizer_provenance.json` | New | Trusted registry keyed by checkpoint file SHA-256 |
| `tests/test_checkpoint_integration.py` | New | 22-test expanded regression suite |
| `scripts/canonical_tokenizer_sha.py` | New | Operator utility to verify canonical tokenizer digest |
| `docs/reports/GENESIS_V06_HARDENED_LOADER_RELEASE_REVIEW.md` | New | This review document |

**Not committed (per scope):** `.pt` checkpoint files, model weights, local training datasets, private workspace data, restored local `tokenizer.json`, absolute machine paths, secrets/tokens, temporary scratch artifacts.

---

## 2. Checkpoint identity-validation mechanism (Phase 3 hardening)

### Trust anchor: checkpoint FILE bytes, not caller-controlled metadata

The legacy loader previously keyed the provenance registry on `meta.base` — a
field **inside** the checkpoint payload and therefore caller-controlled. The
hardened loader replaces this with **checkpoint file SHA-256** as the sole trust
anchor:

```
ckpt_sha = sha256(checkpoint_file_bytes)
entry = registry.by_checkpoint_sha256[ckpt_sha]   # must exist
```

A checkpoint cannot select a different trust record than the one pre-registered
for its exact byte content. `meta.base` is only **cross-checked** against the
trusted record; it is never authoritative.

### Registry record fields

Each entry in `by_checkpoint_sha256` contains:

| Field | Description |
|---|---|
| `format` | Must be `"genesis-ckpt-v1"` |
| `vocab_size` | Must match config |
| `architecture` | `d_model`, `n_layers`, `n_heads`, `n_kv_heads`, `d_ff`, `max_seq_len` — cross-checked against payload config |
| `parent_checkpoint_sha256` | Trusted parent identity (if known) |
| `tokenizer_canonical_sha256` | Expected canonical tokenizer digest (see below) |
| `special_token_layout` | `count`, `id_range`, `version` — must match `SPECIAL_TOKENS` |
| `meta_base` | Cross-checked against `meta.base`; mismatch is rejected |
| `notes` | Human-readable annotation |

### Canonical tokenizer digest

Algorithm (documented precisely for reproducibility):

```
canonical_sha = sha256(json.dumps(json.loads(tokenizer_file), sort_keys=True, ensure_ascii=False))
```

This differs from raw file-byte SHA-256: identical tokenizers serialized with
different whitespace compare equal, while a single changed merge breaks the
digest. See `scripts/canonical_tokenizer_sha.py`.

### Modern checkpoints (format_version=1)

Modern checkpoints carry an embedded tokenizer in the payload. They do **not**
consult the registry and follow the existing compatibility contract unchanged.

### Key hardening changes vs. Phase 1

| Aspect | Phase 1 (meta.base) | Phase 3 (this release) |
|---|---|---|
| Trust key | `meta.base` (caller-controlled) | Checkpoint file SHA-256 |
| Registry lookup | `by_meta_base` | `by_checkpoint_sha256` |
| Path traversal | Not checked | `meta.tokenizer_path` ignored; unknown meta keys dropped; co-located tokenizer only |
| Symlink safety | Not checked | Symlinked `tokenizer.json` rejected |
| Registry corruption | Uncaught | Malformed JSON raises `TokenizerCompatibilityError` |
| Registry location | Hard-coded | `GENESIS_TOKENIZER_REGISTRY` env override supported |

---

## 3. Test results

### Expanded integration suite

```
$ python -m pytest tests/test_checkpoint_integration.py -v
============================ 23 passed in 17.08s ============================
```

**23 tests (5 success + 18 negative):**

| # | Test | Category | Result |
|---|---|---|---|
| 1 | `test_modern_embedded_tokenizer` | Success — modern checkpoint | PASS |
| 2 | `test_legacy_with_verified_tokenizer` | Success — legacy + matching tokenizer | PASS |
| 3 | `test_no_fallback_training_invoked` | No silent training | PASS |
| 4 | `test_weights_byte_stable` | Byte-identical after load | PASS |
| 5 | `test_short_deterministic_inference` | Deterministic generation | PASS |
| 6 | `test_missing_co_located_tokenizer` | Negative | PASS |
| 7 | `test_wrong_vocab_272_no_merges` | 262-native vs BPE distinction | PASS |
| 8 | `test_replaced_companion_tokenizer` | Replaced companion | PASS |
| 9 | `test_tampered_merges_vocab_preserved` | Incompatible BPE merges | PASS |
| 10 | `test_checkpoint_digest_mismatch` | Digest mismatch | PASS |
| 11 | `test_modified_meta_base_rejected` | Modified `meta.base` | PASS |
| 12 | `test_different_checkpoint_same_meta_base` | Same `meta.base`, different ckpt | PASS |
| 13 | `test_missing_registry_entry` | Missing registry entry | PASS |
| 14 | `test_wrong_registry_fingerprint` | Wrong canonical digest | PASS |
| 15 | `test_wrong_architecture` | Architecture mismatch | PASS |
| 16 | `test_wrong_format_in_registry` | Format mismatch | PASS |
| 17 | `test_missing_trusted_fingerprint_field` | Missing tokenizer SHA field | PASS |
| 18 | `test_malformed_registry` | Malformed registry JSON | PASS |
| 19 | `test_unsupported_262_native_lineage` | 262-token GenesisSeed rejected | PASS |
| 20 | `test_corrupt_tokenizer_json` | Corrupted tokenizer | PASS |
| 21 | `test_bad_specials_rejected` | Invalid special-token layout | PASS |
| 22 | `test_symlink_escape_rejected` | Symlink/path-escape | PASS |
| 23 | `test_meta_tokenizer_path_not_honored` | `meta.tokenizer_path` ignored | PASS |

### Existing Genesis test suite

```
$ python -m pytest tests/ -q
.............................ss........................                         [100%]
32 passed, 2 skipped in 0s
```

**32 passed, 2 skipped, 0 failed.**

### Real checkpoint verification (actual recovered artifacts)

Loaded the recovered legacy checkpoint using the integrated `genesis_model.checkpoint`
module (not a scratch implementation):

| Artifact | Expected SHA-256 | Verified |
|---|---|---|
| Legacy checkpoint (`runs/sft_tool_retrain_seq256/genesis.pt`) | `29b6e129388c…` | ✅ |
| Parent checkpoint (`runs/sft_tool_compact_seq256/genesis.pt`) | `a498f8d16120…` | ✅ |
| Restored tokenizer (file SHA-256) | `8ac036e478b1…` | ✅ |
| Restored tokenizer (canonical SHA-256) | `a6585dbc305a…` | ✅ |

**Deterministic generation** (16 tokens, temperature=0.0, two runs identical):

```
Run 1: ' the parents were in the
species of the same'
Run 2: ' the parents were in the
species of the same'
```

**Byte-identical originals after testing:** legacy checkpoint and parent
checkpoint SHA-256 unchanged.

**Modern embedded-tokenizer checkpoint load:** parent checkpoint
(`format_version=1`) loads with embedded tokenizer; canonical SHA verified
(`a6585dbc…`).

---

## 4. Source and test diffs

### `genesis_model/checkpoint.py` diff

```
Original (committed at 0de6bb6, SHA 5ea1b76…):
  - Legacy loader: multi-path tokenizer search, silent fallback training via
    _train_fallback_tokenizer, meta.base-only provenance

Hardened (working tree, SHA 9a10e9c9…):
  + _file_sha256() — checkpoint file byte-hash (trust anchor)
  + _load_tokenizer_provenance() — env-overridable registry, malformed-JSON guard
  + _load_legacy_tokenizer() — fail-closed: file-hash lookup, format/arch/meta_base
    cross-check, symlink rejection, special-token ID validation, canonical SHA check
  + _train_fallback_tokenizer() — disabled (raises TokenizerCompatibilityError)
  + Metadata sanitisation: unknown/spoofed keys (e.g. meta.tokenizer_path) dropped
```

### `genesis_model/tokenizer_provenance.json`

Registry keyed by `by_checkpoint_sha256` with a single entry for the verified
legacy checkpoint (`29b6e129…`). Contains format, architecture, vocab, parent SHA,
tokenizer canonical SHA, special-token layout, and `meta_base` for cross-check.

### `tests/test_checkpoint_integration.py`

22 test functions (5 success, 18 negative) using synthetic, arch-identical
fixtures in isolated temp directories. The production registry is never mutated;
tests inject their own registry via `GENESIS_TOKENIZER_REGISTRY` env override.

---

## 5. Original artifact hash verification

| Artifact | SHA-256 |
|---|---|
| Original `checkpoint.py` (pre-hardening) | `5ea1b76ab13e96970ec30ad2b8911ce1ca3083431a0e4a9bd9a15935135c84d0` |
| Hardened `checkpoint.py` | `9a10e9c99540d3a461d83cf22044dfe83dcc09952e3c773474b7cc269f5fe8d0` |
| Provenance registry | `b5dee1f29c3ce9cbb0870e280d14a56d73b2e074d0255025630b798fdb858306` |
| Legacy checkpoint | `29b6e129388c005cb669eb818abc4d1a78ed25dcd4d06e56dc66031cbe60846d` |
| Parent checkpoint | `a498f8d161202e2b518927f41cd34f4cdd58e05b24e7c0fdd13cf589e8230954` |
| Restored tokenizer (file) | `8ac036e478b1f3c61e5e67ebc5729307c55225952c6dce3b02764cc4b97ecdd8` |
| Restored tokenizer (canonical) | `a6585dbc305aa25331f90cc1faa01054615b3bc0689aef6dfef5092583b27493` |

Original working tree in `Genesis-Model` was **not modified**: `checkpoint.py`
remains at `5ea1b76…` (71-line legacy loader, unmodified), all original `.pt`
files byte-identical before and after.

---

## 6. Remaining security limitations

1. **Registry authenticity:** The trust model note is explicit — checksums detect
   corruption or mismatch only when checked against a trusted reference. They do
   **not** provide cryptographic protection against an attacker who can modify both
   the registry and the checkpoint. Operator-controlled distribution of the
   registry (e.g. signed release artefacts) is out of scope for this review.
2. **`weights_only=True`** prevents arbitrary code execution during checkpoint
   deserialization, but does not guard against adversarial weights.
3. **SHAPE_ONLY candidates** (`bf2bec65…`, `5d2578b8…`) with partial merge overlap
   are rejected by canonical SHA mismatch, but operators should review any
   checkpoint whose registry entry is recently updated.
4. **No runtime attestation** of the registry file itself; integrity depends on
   the operator not replacing `tokenizer_provenance.json` with a forged copy.

---

## 7. Recommended merge procedure

1. Review the diff on `review/genesis-v06-loader-hardening-20261030`.
2. Run `python -m pytest tests/test_checkpoint_integration.py -v` (23 tests).
3. Run `python -m pytest tests/ -q` (32 passed, 2 skipped).
4. Verify artifact hashes against `§5 Original artifact hash verification`.
5. Merge into the audit branch (not `main`/`master`) via a PR with `TokenizerCompatibilityError`
   listed as a breaking change for any pipeline that loaded legacy checkpoints
   without a co-located, registered tokenizer.
6. The original 71-line legacy-loader change in the `Genesis-Model` working tree
   is preserved untouched.

---

## 8. Next native Genesis benchmark stage

With the checkpoint loader hardened and verified:

**Next stage: first-party Genesis intelligence evaluation.**

- Load the recovered legacy checkpoint (`sft_tool_compact_seq256`) with the verified
  tokenizer.
- Run the native 3-task cycle (tool validity, agent cycle, held-out evaluation)
  using `scripts/benchmark_native_3task_cycle.py` and the existing
  `genesis_native_seed_v06` evaluator.
- Compare against the parent checkpoint (`a498f8d1…`) to quantify the re-SFT
  quality delta.
- No model rebuild or tokenizer re-training is required.
