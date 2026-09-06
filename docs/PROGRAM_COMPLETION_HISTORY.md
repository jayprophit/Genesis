# Program completion history and reconciliation

Genesis completion percentages are evidence-gate calculations, not estimates of
lines of code or claims that the digital organism is production-complete. A
percentage can change because a gate gains evidence, because a previously broad
component is split into honest subcomponents, or because a new permanent scope
is added to the denominator.

Use this command to compare two committed snapshots:

```powershell
pwsh -NoProfile -File tools/compare_program_completion.ps1 `
  -BaseRef c000a85 -TargetRef 6c7fe96
```

Use `-TargetRef WORKTREE` to compare a commit with the current generated report.

## 26.17% to 26.86% reconciliation

The 26.17% table belongs to commit `c000a85`. The 26.86% table belongs to
commit `6c7fe96`.

| Measure | `c000a85` | `6c7fe96` | Change |
|---|---:|---:|---|
| Requirements | 156 | 160 | +4 |
| Completion components | 112 | 113 | +1 |
| Security family | 11.25% | 19.44% | +8.19 points |
| Genesis total | 26.17% | 26.86% | +0.69 points |

No requirement or completion component was removed.

The four added requirements were:

- `REQ-CRYPTO-PROVIDER-001`;
- `REQ-CRYPTO-PROVIDER-PERSIST-001`;
- `REQ-CRYPTO-AGILITY-001`;
- `REQ-KEY-CUSTODY-001`.

The earlier `SECURITY-CRYPTO` row mixed two different ideas under “approved
cryptographic providers” and scored 15%. Commit `6c7fe96` redefined that row as
the implemented evidence-only qualification registry, scoring 85%, and added
`SECURITY-CRYPTO-OPERATIONS` at the original 15% so real provider execution did
not inherit the registry's progress. `SECURITY-KEYS` remained 10%.

Security is one of 12 equally averaged top-level families. Its 8.19-point
increase therefore contributes approximately `8.19 / 12 = 0.6825` points to
the Genesis total; rounding the complete component tree produces 26.86%.

## 26.86% to 27.23% key-custody increment

The exact 0.27.0 security snapshot (the commit containing this section) is
compared with parent commit `6c7fe96` below. It excludes the independent,
uncommitted agent-platform increment.

| Measure | `6c7fe96` | 0.27.0 security snapshot | Change |
|---|---:|---:|---:|
| Requirements | 160 | 163 | +3 |
| Completion components | 113 | 115 | +2 |
| Networking family | 12.00% | 12.00% | unchanged |
| Security family | 19.44% | 23.89% | +4.45 points |
| Genesis total | 26.86% | 27.23% | +0.37 points |

No requirement or completion component was removed. The added requirements
are `REQ-KEY-CUSTODY-PERSIST-001`, `REQ-KEY-PREFLIGHT-001` and
`REQ-KEY-OPERATIONS-001`. The added components are
`SECURITY-KEY-LIFECYCLE` and `SECURITY-KEY-OPERATIONS`.

`REQ-KEY-CUSTODY-001` changed from 10/`SPECIFIED` to 90/`BENCHMARKED` after
the secret-free lifecycle, transition, succession and recovery-store slice was
implemented and measured. The former broad `SECURITY-KEYS` leaf at 10% became
a parent: evidence-only lifecycle/recovery is 85%, while real provider-backed
key operations remain 15%. The parent therefore reports 50%, without allowing
metadata progress to imply that keys were generated, protected or used.

Security is still one of 12 equal top-level families, so its 4.45-point change
contributes approximately `4.45 / 12 = 0.3708` points. Full-tree rounding
produces the 27.23% Genesis result.

## 27.23% to 27.48% provider-registration inventory increment

The exact 0.28.0 security snapshot (the commit containing this section) is
compared with its 0.27.0 parent below. It excludes the independent, uncommitted
agent-platform increment.

| Measure | 0.27.0 security snapshot | 0.28.0 security snapshot | Change |
|---|---:|---:|---:|
| Requirements | 163 | 164 | +1 |
| Completion components | 115 | 117 | +2 |
| Networking family | 12.00% | 12.00% | unchanged |
| Security family | 23.89% | 26.94% | +3.05 points |
| Genesis total | 27.23% | 27.48% | +0.25 points |

No requirement or completion component was removed. The added requirement is
`REQ-CRYPTO-PLATFORM-INVENTORY-001`. The former broad
`SECURITY-PLATFORMS` leaf at 15% became a parent with two separately gated
children: read-only provider-registration inventory at 70%, and named platform,
hardware, device and provider qualification at the unchanged 15%. The parent
therefore reports 42.5% without letting a registered provider name imply that a
provider opened, a key was accessed, an operation ran, or the route was
qualified.

The new inventory leaf closes design, implementation, unit, integration and
documentation gates. Benchmark, security review, recovery and platform
qualification remain open. Security is one of 12 equal top-level families, so
its 3.05-point change contributes approximately `3.05 / 12 = 0.2542` points;
full-tree rounding produces the 27.48% Genesis result.

## 27.48% provider-open observation increment

The exact 0.29.0 security snapshot (the commit containing this section) is
compared with its 0.28.0 parent below. It excludes the independent, uncommitted
agent-platform increment.

| Measure | 0.28.0 security snapshot | 0.29.0 security snapshot | Change |
|---|---:|---:|---:|
| Requirements | 164 | 165 | +1 |
| Completion components | 117 | 119 | +2 |
| Networking family | 12.00% | 12.00% | unchanged |
| Security family | 26.94% | 26.94% | unchanged |
| Genesis total | 27.48% | 27.48% | unchanged |

No requirement or completion component was removed. The added requirement is
`REQ-CRYPTO-PROVIDER-OPEN-001`. The two added components are the
`SECURITY-PLATFORM-OBSERVATION` grouping node and the separately gated
`SECURITY-PLATFORM-PROVIDER-OPEN` leaf; the existing inventory leaf was moved
under that grouping without losing evidence.

Both observation leaves score 70%: design, implementation, unit, integration
and documentation gates are closed, while benchmark, security review, recovery
and platform qualification remain open. Their parent therefore remains 70%,
and `SECURITY-PLATFORMS` remains the average of 70% observation and 15% named
qualification: 42.5%. The denominator expansion prevents a safe observation
slice from inflating platform or whole-program completion. A successful KSP
open and a truthful native failure are evidence, not provider qualification.

## 27.48% to 28.56% agent-control-plane increment

The exact 0.30.0 snapshot adds the bounded agent definition, immutable platform
definition persistence and evidence-bearing adapter lifecycle after the 0.29.0
provider-open baseline.

| Measure | 0.29.0 security snapshot | 0.30.0 agent snapshot | Change |
|---|---:|---:|---:|
| Requirements | 165 | 167 | +2 |
| Completion components | 119 | 121 | +2 |
| Networking family | 12.00% | 25.00% | +13.00 points |
| Security family | 26.94% | 26.94% | unchanged |
| Genesis total | 27.48% | 28.56% | +1.08 points |

No requirement or completion component was removed. The added requirements are
`REQ-AGENT-PLATFORM-001` and `REQ-AGENT-PLATFORM-PERSIST-001`; the added
components are `NETWORK-AGENT-DEFINITIONS` and
`NETWORK-AGENT-PERSISTENCE`.

The definition leaf closes design, implementation, unit-test and documentation
gates for 55%. The persistence leaf additionally closes recovery for 60%.
Integration, benchmark, security review and platform qualification remain open.
The adapter registry rejects caller-supplied observation or qualification,
requires canonical probe and independent qualification evidence, checks finite
validity at preflight, and demotes on a failed probe. Those controls are still a
static control plane: no model, browser, scheduler, secret provider, API, UI or
external action runs, and evidence inputs are not automatically trusted.

Networking is one of 12 equally averaged top-level families. Its 13-point
increase contributes approximately `13 / 12 = 1.0833` points; full-tree
rounding produces the 28.56% Genesis result.

## 28.56% to 30.26% cloud-intake, design-asset and repository-audit increment

The exact 0.31.0 snapshot adds a bounded inventory of the user-owned cloud
ChatGPT Genesis project, an original accessible visual/asset foundation, and a
deterministic whole-repository file manifest.

| Measure | 0.30.0 agent snapshot | 0.31.0 audit/design snapshot | Change |
|---|---:|---:|---:|
| Requirements | 167 | 170 | +3 |
| Completion components | 121 | 124 | +3 |
| Foundation family | 38.57% | 43.89% | +5.32 points |
| Platform family | 10.00% | 25.00% | +15.00 points |
| Networking family | 25.00% | 25.00% | unchanged |
| Security family | 26.94% | 26.94% | unchanged |
| Genesis total | 28.56% | 30.26% | +1.70 points |

No requirement or completion component was removed. The added requirements are
`REQ-CHATGPT-PROJECT-INTAKE-001`,
`REQ-DESIGN-ASSET-GOVERNANCE-001` and
`REQ-REPOSITORY-CONTENT-AUDIT-001`. Their matching leaves are
`FOUNDATION-CLOUD-INTAKE`, `PLATFORM-DESIGN-ASSETS` and
`FOUNDATION-REPOSITORY-AUDIT`.

The cloud traversal reached the oldest page for 47 conversations through 264
pages, 2,400 turns and 4,522 returned message items. It explicitly records 994
items truncated by the bounded reader, seven reported attachments whose image
bytes were unavailable, and the absence of a stable raw cloud-object digest.
Reaching the oldest page is therefore not labelled a lossless export. The public
registry uses opaque sequential IDs; conversation titles, project identifiers
and cloud locators are retained only in the ignored machine-local mapping.

The asset foundation contains four original generated concept boards, two
original SVGs, design tokens and the exact prompt record. The manifest validates
paths, sizes, hashes, media types, dimensions, JSON and accessible SVG metadata.
This closes a 70% asset-governance leaf, not the UI shell or any embodied-device
implementation. Hardware, flight, sensing, life support, materials, safety,
privacy and platform claims remain open.

The repository inventory separately hashes every canonical file outside Git and
disposable build metadata, using canonical-LF hashes for working text and raw
hashes for preserved/binary input. Its manifest and dated audit report exclude
themselves to prevent circular hashes; that exclusion is part of the requirement.

Foundation and Platform are two of 12 equally averaged top-level families. Their
combined 20.32-point increase contributes approximately
`20.32 / 12 = 1.6933` points; full-tree rounding produces 30.26%.

## Local uncommitted scopes

Completion reports generated from a dirty working tree are not published
baselines. They can include multiple independent increments. Before a commit,
compare the exact staged snapshot and report its percentage separately from the
combined working tree. The commit hash, component registry, requirement
registry and generated report together identify a reproducible percentage.

## Accounting rules

1. Never replace a historical percentage without naming its commit.
2. Report requirements and component counts with the percentage.
3. List added, removed and re-scoped IDs.
4. Separate implementation evidence from denominator expansion.
5. Preserve real operations, platform qualification and security review as
   separate gates from control models or evidence registries.
6. Generate and verify the report from the exact staged tree before publishing.
