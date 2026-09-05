# Full-Stack AI Agent Automation Platform

Status: additive architecture and control-plane scaffold for Genesis 0.30.0. Last reviewed 2026-09-06. External execution is unavailable.

## Purpose and boundaries

This capability lets an operator define agents, grant narrow tools, assemble workflows, attach triggers, require human approval and inspect runs. It extends Genesis through `genesis::agents`; it does not create a second runtime, memory store, security policy or source of truth.

The C++ core owns definitions, validation, registry identity, deterministic run preparation, limits and denial decisions. A future local Control Center may use a versioned API, but must not duplicate those rules. OpenAI or another model provider, Clerk or another identity provider, Browserbase or another browser executor, and Next.js/React/Tailwind or another interface stack are optional adapters. None is a required dependency or currently operational.

## Component map

| Requested capability | Canonical owner / scaffold | Current status |
|---|---|---|
| Agent Factory | `AgentFactory` plus JSON schema/templates | Validates explicit definitions; natural-language compilation unavailable |
| Agent Registry | `AgentRegistry` and `AgentPlatformStore` | Duplicate-safe with immutable snapshot recovery |
| Agent Runtime | `AgentRuntime` over deterministic runtime contracts | Preflight only; no autonomous executor |
| Model Router | evidence-qualified model-kind `AdapterRegistry` entries | Lifecycle and expiry gate only; no provider linked |
| Tool Registry | evidence-qualified tool-kind `AdapterRegistry` entries | Capability, independent qualification, expiry and failure-demotion gates |
| Workflow Engine | `WorkflowDefinition`, ordered steps and `WorkflowRegistry` | Persisted bounded plan; execution/checkpointing pending |
| Scheduler/Trigger Engine | `TriggerDefinition` and `TriggerRegistry` | Persisted definitions; wall-clock/event service pending |
| Memory | existing `genesis::memory` capability | Reuse required; agent-specific retrieval policy pending |
| Permissions/Secrets | explicit `ToolGrant`; existing security boundary | Grants scaffolded; secret values must never enter definitions/logs |
| Observability | `RunRecord` plus future provenance events | Preflight diagnostics only |
| Security/Sandbox | required sandbox flag, limits, approval gate | Policy model only; no sandbox adapter qualified |
| API | future versioned boundary around C++ owners | Not implemented |
| Control Center | future replaceable operator UI | Not implemented |

## Execution and approval flow

`trigger -> immutable run request -> registry lookup -> workflow/limit validation -> adapter qualification -> grant check -> human approval when required -> sandboxed execution -> bounded memory/provenance record`

A trigger never grants authority. Model output is untrusted input. Every external action is checked against the registered agent grant at execution time. Actions with financial, destructive, account, communication, credential, installation or broad-write impact require explicit human approval regardless of model recommendation. Approval must bind the run ID, plan digest, tool/action set, expiry and approving identity; changed plans require new approval.

## Adapter evidence gate

Adapters move only through `declared -> observed -> qualified`. Registration rejects a caller-supplied `observed` or `qualified` state. A successful probe with a canonical evidence digest is required for observation; a separately identified qualifier, qualification digest and finite validity interval are required for qualification. Static preflight checks that both the model route and every tool route are still inside that interval. A later failed probe immediately demotes a qualified adapter and a fresh successful probe is required before requalification.

These records prove only that the control-plane lifecycle was followed. They do not prove that a real adapter was honestly probed or independently assessed; production evidence must still be issued by a trusted platform-observation and qualification boundary. Declared configuration is not proof of access, observed connectivity is not permission to execute, and control-plane qualification is not operation authorization. Provider qualification additionally requires pinned dependencies, authentication handling, scoped credentials, timeout/retry behavior, rate-limit handling, audit redaction, failure isolation, revocation and representative tests.

## API direction

A future API should expose versioned resources for agents, tools/models, workflows, triggers, approvals and runs. It must use idempotency keys for mutations, optimistic concurrency for definitions, pagination for logs, authenticated operator identity and structured denial responses. Secret material is referenced by opaque handles and resolved only inside the qualified adapter boundary.

## Security defaults

### Static preflight checks (2026-09-05)

Workflow registration and direct run preparation now share structural validation:
workflow IDs, step IDs, tool IDs and actions must be present, and step IDs must be
unique. Each current workflow step represents one tool call. Preparation checks
both `max_steps` and `max_tool_calls` before scanning the plan, then requires the
requested action in both the agent grant and the tool's declared capabilities.
Invalid approval, adapter and trigger enum values are rejected at registration.

`always` requests review for all plans. Because the scaffold has no trusted risk
classifier, `risky_actions` also requests review for all tool actions. `never`
can pass static preflight only when no mandatory step review is present and the
matching grant enables neither network access nor filesystem writes. Either of
those grant flags conservatively requires review, even if a particular action
might not use the permission. Action names such as `read` are not proof of safety.

`ready` means static checks passed. It does not authorize execution or prove
sandboxing, verified approval, provider operation, runtime budget enforcement
or absence of side effects. The registry no longer accepts caller-supplied
qualification, but its evidence inputs are still caller supplied; these checks
cannot replace a trusted observer, trusted qualifier, action metadata and an
execution-time policy boundary. Snapshot version 1 remains unchanged and
recovered definitions undergo the same strengthened validation and preflight
checks.

Tests include direct-qualification rejection, observation evidence, independent
qualification, validity intervals, failure demotion and requalification; model
and tool gates; capability/grant mismatch; duplicate and empty workflow fields;
approval modes; mandatory review; sandbox requirements; network/write grants;
separate and exact budget boundaries; invalid enums; and policy preservation
after recovery.

### Execution requirements

- Deny undeclared tools, actions and adapters that are not qualified.
- Require finite runtime, step, tool-call and memory limits.
- Default filesystem access to read-only and network access to denied.
- Isolate browser sessions, processes, working directories and credentials per run.
- Redact secrets and sensitive payloads from prompts, traces and diagnostics.
- Treat retrieved pages, tool results and conversation imports as untrusted data.
- Support cancellation, approval expiry, replay detection and append-only audit events.

## Delivery slices

1. Completed foundation slice: definition schema/template, registries, adapter evidence state, bounded workflow model and approval-aware preflight tests.
2. Completed persistence foundation: one checksummed immutable snapshot for the agent, workflow and trigger registries, with atomic commit, conflict denial and corruption tests. Schema migration beyond version 1 remains pending.
3. Runtime slice: checkpoints, cancellation, retry policy, provenance events and memory policy integration.
4. Security slice: approval receipts, opaque secret-provider interface and qualified sandbox/tool adapters.
5. Product slice: versioned local API and replaceable Control Center, followed by optional provider-specific adapters.

No UI, hosted service, scheduled execution, browser automation, model inference or secret store is claimed by the current scaffold.
