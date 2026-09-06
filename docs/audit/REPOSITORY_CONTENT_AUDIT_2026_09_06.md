# Genesis Repository Content Audit — 2026-09-06

## Decision

The authoritative Genesis checkout has one coherent source tree and one
regenerable top-level `build/` directory. This audit found no duplicate
full-file hashes, empty canonical files, or executable-source
`TODO`/`FIXME`/`XXX`/`HACK` markers. All 226 canonical source files are recorded
in `registry/repository_content.tsv` with a role, byte count, line count, media
class and SHA-256 digest.

This is a deterministic content inventory and structural audit. It is not a
claim that every line has received independent semantic review, that every
registered requirement is implemented, or that generated concept art proves a
working interface or device.

## Scope and exclusions

The inventory recursively covers the authoritative checkout at
`C:/Users/jpowe/Desktop/Genesis`. It deliberately excludes:

- `.git/`, because version-control object storage is not project source;
- `.genesis-local/`, because it contains private machine-local source locators
  that must never be published or treated as canonical runtime data;
- `build/` and `build-*`, because compiled and generated products are
  disposable evidence outputs rather than canonical input;
- `registry/repository_content.tsv`, because a manifest cannot contain its own
  stable hash; and
- `docs/audit/REPOSITORY_CONTENT_AUDIT_*.md`, because this report describes the
  manifest and would otherwise create a circular update.

Reparse-point source files are rejected. Working text uses SHA-256 after CRLF is
normalized to LF so Windows and Linux CI can compare the same canonical
content. Preserved source text and media use raw-byte SHA-256.

## Exact inventory snapshot

| Measure | Result |
|---|---:|
| Canonical files | 226 |
| Total bytes | 12,274,512 |
| Counted text lines | 102,595 |
| Text files | 220 |
| Image files | 6 |
| Other binary files | 0 |
| Empty files | 0 |
| Duplicate full-file hash groups | 0 |

SVGs are counted as images, while their accessible XML content is checked
separately by the governed-asset validator. The four PNG concept boards, two
SVGs, one design-token JSON file and one prompt record are also governed by
`registry/assets.tsv`; `assets/README.md` is the only deliberately unregistered
asset-tree file because it documents the registry itself.

### Top-level ownership

| Category | Files | Bytes | Counted lines |
|---|---:|---:|---:|
| `.github` | 1 | 1,076 | 40 |
| `assets` | 9 | 8,679,641 | 172 |
| `benchmarks` | 15 | 58,488 | 983 |
| `docs` | 40 | 1,618,394 | 75,833 |
| `include` | 43 | 135,590 | 3,659 |
| `provenance` | 2 | 10,021 | 69 |
| `registry` | 29 | 820,152 | 4,329 |
| repository root | 6 | 20,597 | 316 |
| `schemas` | 1 | 2,042 | 36 |
| `src` | 44 | 606,719 | 11,629 |
| `templates` | 1 | 539 | 13 |
| `tests` | 14 | 257,106 | 4,469 |
| `tools` | 21 | 64,147 | 1,047 |

### File types

| Extension | Files | Bytes |
|---|---:|---:|
| `.cpp` | 75 | 924,866 |
| `.hpp` | 43 | 135,590 |
| `.md` | 39 | 242,623 |
| `.tsv` | 30 | 828,004 |
| `.ps1` | 19 | 61,594 |
| `.txt` | 6 | 1,407,061 |
| `.json` | 4 | 5,029 |
| `.png` | 4 | 8,662,571 |
| `.svg` | 2 | 5,209 |
| no extension | 1 | 511 |
| `.gitattributes` | 1 | 328 |
| `.gitignore` | 1 | 50 |
| `.yml` | 1 | 1,076 |

## Structural findings

1. `include/genesis/` and `src/` remain the public-interface and implementation
   owners. Tests and benchmarks are separate evidence layers rather than
   parallel implementations.
2. Only one top-level build directory exists: `build/`. Historical
   `build-clean`, `build-final`, `build-next` and stage-labelled output trees are
   absent.
3. The repository remains C++20-first. CMake owns the build graph, PowerShell
   owns bounded repository automation, and JSON/TSV/Markdown/YAML are data or
   configuration formats. No second application runtime has been introduced.
4. The user-owned cloud Genesis conversation project has its own bounded,
   anonymized public intake registry and abstraction. The direct locator map is
   Git-ignored and local-only. Raw private conversation bodies have not been
   turned into runtime memory or copied wholesale into production code.
5. Generated images are under `assets/`, carry content hashes and claim
   boundaries, and do not alter organism runtime state.

## Marker review

A broad case-insensitive marker scan returned 21 occurrences. They are accounted
for as follows:

- no `TODO`, `FIXME`, `XXX` or `HACK` marker occurs in `include/`, `src/`,
  `tests/`, `tools/` or `CMakeLists.txt`;
- two cryptographic benchmark strings explicitly say that synthetic digests are
  benchmark-only placeholders and do not claim provider qualification;
- two agent-platform documentation rows state that the API and Control Center
  are not implemented;
- the asset manifest states that illustrative Control Center values are not
  implementation evidence;
- preserved specifications and research reports discuss placeholders or hacks
  as source text, not active code; and
- `registry/fdrive_links.tsv` retains nine example, redacted or placeholder
  links as `UNVERIFIED`, rather than treating them as external evidence.

These occurrences are honesty boundaries or preserved research data. They are
not silently completed work.

## Open evidence gaps

- The cloud reader reached the oldest page for all 47 identified project
  conversations, but 994 returned items were truncated and seven reported
  attachments were unavailable through the text interface. The intake is not a
  lossless export.
- The generated Control Center is visual guidance only. There is no implemented
  operator frontend, live accessibility qualification or device-control route.
- Suit, orb, vehicle, platform, sensing, materials, power, flight, life-support,
  privacy and safety ideas remain concept or research inputs until separately
  engineered and qualified.
- Passing local tests does not establish deployment, platform, hardware,
  cryptographic, security, performance or mass-adoption qualification.
- File hashes prove exact snapshot membership and detect stale inventory; they
  do not prove correctness, originality, licensing, safety or requirements
  coverage by themselves.

## Reproduction

From the authoritative repository root:

```powershell
pwsh -NoProfile -File tools/inventory_repository_content.ps1
pwsh -NoProfile -File tools/validate_repository_content.ps1
pwsh -NoProfile -File tools/validate_assets.ps1
pwsh -NoProfile -File tools/validate_chatgpt_project_intake.ps1
pwsh -NoProfile -File tools/validate_registry.ps1
```

Any canonical source change must be followed by inventory regeneration. CI
rejects a stale content manifest.
