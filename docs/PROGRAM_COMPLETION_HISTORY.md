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

## Combined local working-tree view

The concurrent local agent-platform increment adds two requirements and two
network components. When both independent scopes are generated together, the
working tree contains 165 requirements and 117 components, networking is
21.43%, security is 23.89%, and Genesis is 28.01%. That combined figure is not
the published security baseline until the agent-platform scope receives its
own review and commit.

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
