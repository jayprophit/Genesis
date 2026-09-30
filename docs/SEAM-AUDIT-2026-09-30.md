# Genesis seam audit — what is actually implemented

Date: 2026-09-30
Commit audited: `fa13316` (clean-clone baseline, 21/21 ctest)
State vocabulary is used strictly: ABSENT / RESEARCHED / DESIGNED / PLANNED /
PARTIAL / IMPLEMENTED / TESTED / INTEGRATED / VERIFIED.

This audit exists because the vertical-slice plan assumed Genesis could drive
work. It cannot, and it says so itself. Everything below is read from the
headers and implementations in this repository, not from a design document.

## Summary

Genesis is a well-built, honestly-scoped **cognition and evidence core**. It is
not an executor, and it is explicit about that in its own diagnostics. The
"bounded real action" in the vertical slice is performed by Agent Bridge; the
job still missing on the Genesis side is **proposing** an action with evidence,
not performing one.

## Seam classification

| Seam | State | Evidence in this repository |
| --- | --- | --- |
| Persistent identity | IMPLEMENTED + TESTED | `identity/genesis_identity.*`, `entity_registry.*`, `life_record.*`; suites `genesis_identity_tests`, `genesis_genesis_identity_tests`, `genesis_life_record_tests` green from a clean clone |
| Adapter lifecycle | IMPLEMENTED + TESTED | `agents/platform.*`: declare -> record_probe -> qualify, each evidence-digest bearing; `is_qualified(id, at)` honours expiry |
| Agent / workflow / trigger registries | IMPLEMENTED + TESTED | `AgentRegistry`, `WorkflowRegistry`, `TriggerRegistry` with `snapshot()`; `agent_platform_tests` |
| Run preflight | IMPLEMENTED (preflight only) | `AgentRuntime::prepare()` — validates run id, input digest, step/tool limits, qualified model adapter, sandbox requirement, tool capability, agent grant; returns `ready` / `pending_approval` / `denied` |
| **Run execution** | **ABSENT** | `prepare()` is documented and self-reported as "static preflight passed; execution remains unavailable" / "human approval required; execution remains unavailable" |
| **Worker lifecycle** | **ABSENT** | no worker, process, spawn, heartbeat, timeout, cancellation, result reconciliation or checkpoint interaction anywhere in `include/` or `src/` |
| Organism loop: perception and state | IMPLEMENTED + TESTED | `sense`, `perceive`, `interoception`, `attention_proxy`, `mental_state`, `recall`, `predict` all `executed=true` in `organism/loop.cpp` |
| Organism loop: drive, goal, safety gate | IMPLEMENTED + TESTED | `drive`, `goal`, `safety_gate` executed, with honest `denied` / `allowed` notes |
| Organism loop: learning, memory, receipt | IMPLEMENTED + TESTED | `learn`, `memory_update`, `homeostasis`, `allostasis`, `receipt` executed; `observe` and `prediction_error` executed or explicitly `SKIPPED` with a reason |
| **Organism loop: planning** | **ABSENT** | `MarkMissing(receipt.stages, LoopStage::plan_na, "no action planner")` |
| **Organism loop: action** | **ABSENT** | `MarkMissing(receipt.stages, LoopStage::action_na, "no actuator; Ready never authorizes")` |
| Determinism | IMPLEMENTED + TESTED | `runtime::LogicalClock` never reads wall-clock time; event envelopes are immutable with SHA-256 payload digests |
| Memory / knowledge graphs | IMPLEMENTED + TESTED | `memory/graph.*`, `memory/origin.*`, `cognition/world_model.*`, `cognition/continuity.*`; `memory_cognition_tests`, `learning_tests` |
| Security / custody | IMPLEMENTED + TESTED | `security/*`; `security_tests`, `key_custody_tests`, `platform_crypto_inventory_tests`, `provider_open_observation_tests` |
| Registry / requirements | IMPLEMENTED + TESTED | `requirements/registry.*`, `provenance/ledger.*`; `research_registry_tests` |
| Bridge / P25 integration | ABSENT here | no HTTP client, no policy evaluation, no authority call in this repository; by design, Agent Bridge owns execution and P25 owns authority |
| Recovery / checkpoint of a run | ABSENT | persistence exists per-subsystem (`*_persistence.*`), but there is no run-level checkpoint to resume, because there is no run |

## The `RunState` gap, stated precisely

`agents/platform.hpp` declares:

    enum class RunState { pending_approval, ready, running, succeeded, failed, denied };

Only three of those six values are ever produced, by `prepare()`:
`pending_approval`, `ready`, `denied`. `running`, `succeeded` and `failed` are
unreachable — there is no code path that sets them. A reader seeing the enum
would reasonably assume a run can start, finish or fail; it cannot.

This is not a bug in a test and it is not a wiring mistake. It is a scope
boundary the code states out loud. Any plan that assigns "worker crash handling"
to Genesis as a *test* task is wrong: there is no worker to crash.

## What this changes in the vertical slice

1. **"Bounded real action" belongs to Agent Bridge**, which is real and proven
   (21/21 native gates plus the IDE vertical slice over live HTTP). Genesis
   supplies perception, attention, recall, drive, goal, the safety gate and the
   receipt.
2. **The missing Genesis link is proposal, not execution.** To close the loop,
   Genesis needs to emit a bounded, evidence-bearing *action request* that P25
   and Agent Bridge can accept — while still not executing it. That is exactly
   the `plan_na` / `action_na` boundary, and the safety gate already produces
   the `denied` / `allowed` verdict it would attach to.
3. **Worker lifecycle is a separate, larger capability** that does not exist
   yet in any owner. It must not be recorded as a failing test or as Genesis
   behaviour. If the vertical slice needs a real temporary worker, the executor
   has to be built, with startup, heartbeat, timeout, crash, cancellation,
   failed-result, reconciliation, cleanup and checkpoint interaction — and
   `Worker identity != Genesis identity` has to hold throughout.
4. **`Ready never authorizes` is an invariant worth keeping.** Genesis marking a
   step as allowed is not authority, and the code already refuses to let it be.

## Recommended next increments, in order

1. Genesis: an evidence-bearing action-request record and proposal emitter that
   requires a safety-gate allow plus a qualified tool adapter, producing a
   request rather than performing anything. Testable purely in C++.
2. Genesis-in-IDE: run a real scenario where Genesis proposes, the IDE shows
   the request, the human decides, Agent Bridge executes, and Genesis observes
   the result. That is the loop the harness was built for.
3. Only then: decide where a real temporary worker and its lifecycle belong, and
   register that as the new capability it is rather than as a missing test.
