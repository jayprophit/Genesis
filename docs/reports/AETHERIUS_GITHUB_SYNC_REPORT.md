# Aetherius / Genesis GitHub Sync Report

**Date:** 2026-10-25
**Operator:** Hermes (local audit + push)
**Objective:** Publish the verified Aetherius↔Genesis IDE integration and
Agent Bridge evidence to dedicated GitHub review branches for ChatGPT (Kai)
inspection. **No force-push; nothing pushed to `main`/`master`.**

## 1. Repositories audited

| Project | Local git root | Default branch @ HEAD | GitHub remote | Review branch created? |
|---|---|---|---|---|
| Genesis-Model | `...\Genesis\Genesis-Model` | `genesis-native-v06` @ `7749ecc` (Python runtime lives on `main`/`f8484cb`) | `jayprophit/Genesis` | yes — `f8484cb9afce` (from `main`/`f8484cb`) |
| Aetherius-OS (IDE) | `...\Aetherius-OS\apps\IDE-Workspace` | (sub-repo of monorepo) `master` @ `87b7d3b` | `jayprophit/IDE-Workspace` | yes — `0a2519d7e2cb` (from `master`) |
| Aetherius-OS (top-level) | `...\Aetherius-OS` | `master` @ `87b7d3b` (= `origin/master`) | `jayprophit/Aetherius-OS` | yes — `83285063c276` (from `master`) |
| Agent-Bridge | `...\Agent-Bridge` | `main` @ `fda42f3` | `jayprophit/Agent-Bridge` | yes — `57304d08417d` (from `main`) |
| Universal-Bridge | `...\Universal-Bridge` | `main` @ `eee144d` | `jayprophit/Universal-Bridge` | no integration changes — excluded |
| Materials Atlas / MAT | `...\Materials-Atlas-Table-Codex---MAT` | `main` @ `94decf1` | `jayprophit/Materials-Atlas-Table-Codex---MAT` | no integration changes — excluded |

Repository boundaries preserved: the IDE code lives in the **separate**
`IDE-Workspace` repository (its `.git` is at `apps/IDE-Workspace`); the
top-level `Aetherius-OS` repository does not track the IDE source (no
`.gitmodules`; `apps/IDE-Workspace` is not a submodule entry).

## 2. Verification before commit (per repo)

| Check | Command | Result |
|---|---|---|
| Genesis runtime | `curl :8420/health`; `POST :8420/v1/agent` task | `/health` 200, 11 tools, `agent_bridge_active=true`; `phase4.add_numbers(47,58)=105` |
| Genesis Python import | `python -c "from genesis_model.server import serve"` | import ok |
| IDE TypeScript | `tsc --noEmit` (workspace/app) | PASS (0 errors) |
| IDE runtime tests | `vitest run --pool=threads` (bounds, useGenesisRuntime, runtime-mapping) | **50/50** PASS (14 + 36) |
| Agent-Bridge fast gate | recorded `build.yaml` gate | **2301 passed**, 18 skipped, 1 failed (MAT env-gated), 50 deselected; 2,158.66 s |

## 3. Commits created (each on its own review branch)

### IDE-Workspace — `0a2519d7e2cb`
`feat: connect Genesis runtime to Aetherius IDE via GenesisClient`
- `workspace/app/src/genesis-ui/runtime/GenesisClient.ts` (NEW — approved adapter)
- `workspace/app/src/genesis-ui/runtime/useGenesisChat.ts` (NEW — minimal parallel hook)
- `workspace/app/src/genesis-ui/runtime/runtimeTypes.ts` (`+ 'genesis'`)
- `workspace/app/src/genesis-ui/GenesisShell.tsx` (wire genesis source/base + modes)
- `workspace/app/src/genesis-ui/runtime/bounds.test.ts` (allowlist `GenesisClient.ts`)
- 5 files / +185 / -7

### Agent-Bridge — `57304d08417d`
`docs: record Agent-Bridge fast-gate results incl. Genesis runtime test classification`
- `build.yaml` (TODO-AB-001 OPEN→DONE + resolution)
- `docs/BRIDGE_FAILURE_DISPOSITION.md` (2026-10-25 fast-gate disposition)
- 2 files / +32 / -2
- (`tools/phase4_adapter.py` already on `main` @ `fda42f3`)

### Aetherius-OS (top-level) — `83285063c276`
`docs: record Agent-Bridge fast-gate DONE and Genesis CI verification status`
- `docs/PROJECT-TODOS.yaml` (TODO-AB-001 DONE + design-seed evidence)
- `docs/Aetherius-OS-Programme-Status.md` (NEW — "Genesis CI PASSED")
- 2 files

### Genesis-Model — `f8484cb9afce` (already published)
Branch created from `main`; contains the verified `genesis_model/server.py`
(`do_OPTIONS`, `/health` None-safe, `/v1/agent` dispatch) and
`genesis_model/aetherius_bridge.py` (`phase4.*` over `127.0.0.1:11434`).
Plus repo docs (`docs/GENESIS_STATUS.yaml`, `docs/GAP_ANALYSIS.md`, …).
A `docs/reports/` commit adding the acceptance report follows (see §5).

## 4. Files intentionally excluded from each push
- `.vite/`, `__pycache__/`, `node_modules/`, `.venv-torch-probe/` — build/venv (gitignored or excluded).
- Unrelated pre-existing untracked dirs in IDE-Workspace working tree (`workspace/clean-probe*`, `walkthrough-*`, `genesis-slice-*`) — autonomous-probe artefacts, not committed.
- `perf-run/screenshots/*.png` — synthetic-only, kept local, referenced (not committed).
- No secrets: local pre-commit secret scanner reported clean across 971 files (Agent-Bridge); Genesis/Aetherius pushes staged explicit doc/source files only.

## 5. Reports placed in `docs/reports/` (Genesis repo, review branch)
After review, copied/published:
- `GENESIS_IDE_LIVE_ACCEPTANCE_REPORT.md` — Gate D acceptance + controlled re-test appendix.
- `AETHERIUS_GITHUB_SYNC_REPORT.md` — this file.
- `KAI_GITHUB_REVIEW_HANDOFF.md` — direct links + key source paths.
(Originals outside git left in place; not moved/deleted.)

## 6. Performance measurements (re-test 2026-10-25)
- Warm HTTP `/v1/agent`: **4.07 s** (baseline 5.5–5.7 s).
- Warm graphical E2E (resident model): **5.9 s**.
- Cold graphical E2E (non-resident model): **77.4 s** (Ollama model load dominates; source of the ~110 s figure).
- CUDA: 564 MB VRAM (`size_vram`), 2117–2266 MiB, GPU util 14–24 %.
- Keep-alive: 30 m preload preserved across a Genesis call (1774 s → 1802 s remaining) — not overridden.

## 7. Status: AETHERIUS_GITHUB_REVIEW_READY
All four review branches pushed; local↔remote SHA verified identical on every
repo; validation passed (Genesis runtime →105, IDE tsc PASS + 50/50 tests, Agent-Bridge fast-gate 2301 passed).
