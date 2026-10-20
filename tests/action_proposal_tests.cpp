// Genesis action-proposal seam (plan_na / action_na boundary).
//
// What is proven here:
//   - Genesis can propose one bounded effect with evidence, and cannot perform it;
//   - the proposal carries the CANONICAL Genesis identity, never a new one;
//   - every refusal names the gate that refused it, and a refusal is a valid
//     outcome rather than an exception or a silent drop;
//   - a safety-gate denial stops the proposal at Genesis, before P25 is involved;
//   - one proposal names one resource, so authority can never be read as
//     covering a second, unapproved target;
//   - the same intent always yields the same proposal id.

#include "genesis/genesis.hpp"
#include "test_support.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using genesis::agents::ActionProposal;
using genesis::agents::ActionProposalEmitter;
using genesis::agents::AdapterDescriptor;
using genesis::agents::AdapterKind;
using genesis::agents::AdapterRegistry;
using genesis::agents::AdapterState;
using genesis::agents::AgentDefinition;
using genesis::agents::AgentRegistry;
using genesis::agents::ProposalDisposition;
using genesis::agents::RunRequest;
using genesis::agents::RunRecord;
using genesis::agents::RunState;
using genesis::agents::ToolGrant;
using genesis::agents::WorkflowDefinition;
using genesis::agents::WorkflowStep;
using genesis::agents::WorkflowRegistry;
using genesis::cognition::AuthorizationDecision;
using genesis::cognition::SelfCapabilityModel;
using genesis::organism::LoopReceipt;

int failures = 0;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::string digest(char character) {
    return std::string(64U, character);
}

const char* kGenesisId = "genesis-canonical-0001";

AuthorizationDecision allowed_decision() {
    return AuthorizationDecision{true, true, true, digest('a')};
}

AdapterRegistry qualified_tools() {
    AdapterRegistry tools;
    AdapterDescriptor descriptor("tool-writer", AdapterKind::tool,
                                 AdapterState::declared, {"write", "read"});
    require(tools.declare(descriptor), "tool adapter declares");
    require(tools.record_probe("tool-writer", true, "observer-1", digest('c'), 100U),
            "tool adapter records a successful probe");
    require(tools.qualify("tool-writer", "qualifier-1", digest('d'), 100U, 10'000U),
            "tool adapter qualifies");
    return tools;
}

WorkflowDefinition write_workflow() {
    WorkflowStep step;
    step.step_id = "step-1";
    step.tool_id = "tool-writer";
    step.action = "write";
    step.requires_approval = true;
    WorkflowDefinition workflow;
    workflow.workflow_id = "wf-write";
    workflow.steps.push_back(step);
    return workflow;
}

RunRecord ready_run() {
    RunRequest request;
    request.run_id = "run-1";
    request.agent_id = "agent-1";
    request.workflow_id = "wf-write";
    request.input_digest = digest('e');
    request.prepared_at = 100U;
    return RunRecord{request, RunState::ready, 0U, "preflight ok"};
}

RunRecord denied_run() {
    RunRecord record = ready_run();
    record.state = RunState::denied;
    record.diagnostic = "workflow action is outside the agent grant";
    return record;
}

LoopReceipt completed_receipt() {
    LoopReceipt receipt;
    receipt.completed = true;
    receipt.safety_denied = false;
    receipt.event_id = "evt-1";
    receipt.drive.name = "curiosity";
    receipt.goal.description = "record the current state";
    return receipt;
}

LoopReceipt safety_denied_receipt() {
    LoopReceipt receipt = completed_receipt();
    receipt.safety_denied = true;
    return receipt;
}

SelfCapabilityModel qualified_capabilities() {
    SelfCapabilityModel model("organism-1", 8U);
    require(model.declare_capability("tool-writer", "route", digest('c'), 100U),
            "capability declares");
    for (int trial = 0; trial < 6; ++trial) {
        require(model.record_outcome("tool-writer", true, digest('f'),
                                     100U + static_cast<std::uint64_t>(trial)),
                "capability outcome recorded");
    }
    require(model.qualify("tool-writer", digest('d'), 110U), "capability qualifies");
    return model;
}

AgentDefinition agent_def() {
    AgentDefinition definition;
    definition.agent_id = "agent-1";
    definition.display_name = "Bounded writer";
    definition.purpose = "bounded writer";
    definition.model_route = "model-1";
    ToolGrant grant;
    grant.tool_id = "tool-writer";
    grant.allowed_actions.insert("write");
    grant.filesystem_write_allowed = true;
    definition.tool_grants.push_back(grant);
    return definition;
}

// A capability model is stateful, so callers that only need "a usable model"
// take a fresh one per proposal rather than sharing a qualification.
SelfCapabilityModel capabilities() {
    return qualified_capabilities();
}

ActionProposal emit(const SelfCapabilityModel& capabilities,
                    const AdapterRegistry& tools,
                    const RunRecord& run,
                    const WorkflowStep& step,
                    const WorkflowDefinition& workflow,
                    const std::string& resource,
                    const AuthorizationDecision& authorization,
                    const LoopReceipt& receipt) {
    ActionProposalEmitter emitter(kGenesisId, &capabilities, &tools);
    return emitter.propose(run, step, workflow, resource, digest('e'), authorization,
                           receipt, 200U);
}

void test_emits_an_evidence_bearing_proposal() {
    const auto capabilities = qualified_capabilities();
    const auto tools = qualified_tools();
    const auto workflow = write_workflow();
    const auto proposal = emit(capabilities, tools, ready_run(), workflow.steps.front(),
                               workflow, "notes.txt", allowed_decision(),
                               completed_receipt());
    require(proposal.disposition == ProposalDisposition::approval_required,
            "a step requiring approval is proposed as approval_required");
    require(proposal.genesis_id == kGenesisId, "proposal carries the canonical Genesis identity");
    require(proposal.run_id == "run-1", "proposal correlates to the run, not a new identity");
    require(proposal.tool_id == "tool-writer", "proposal names the qualified tool adapter");
    require(proposal.action == "write", "proposal names the requested action");
    require(proposal.resource == "notes.txt", "proposal names exactly one resource");
    require(proposal.authority_owner == "p25", "authority is always P25, never Genesis");
    require(proposal.evidence_digest == digest('d'), "proposal cites the adapter qualification evidence");
    require(proposal.receipt_digest == completed_receipt().digest(),
            "proposal cites the loop receipt that motivated it");
    require(proposal.proposal_id.rfind("apx-", 0U) == 0U, "proposal has a content-addressed id");
    require(proposal.parameter_digest == digest('e'), "proposal fixes the intended parameters");
}

void test_proposal_is_stable_for_the_same_intent() {
    const auto capabilities = qualified_capabilities();
    const auto tools = qualified_tools();
    const auto workflow = write_workflow();
    const auto first = emit(capabilities, tools, ready_run(), workflow.steps.front(),
                            workflow, "notes.txt", allowed_decision(),
                            completed_receipt());
    const auto second = emit(capabilities, tools, ready_run(), workflow.steps.front(),
                             workflow, "notes.txt", allowed_decision(),
                             completed_receipt());
    require(first.proposal_id == second.proposal_id,
            "the same intent yields the same proposal id, so a duplicate is recognizable");
    require(first.digest() == second.digest(), "the same intent yields the same digest");

    const auto other = emit(capabilities, tools, ready_run(), workflow.steps.front(),
                             workflow, "other.txt", allowed_decision(),
                             completed_receipt());
    require(other.proposal_id != first.proposal_id,
            "a different resource yields a different proposal id");
}

void test_refuses_without_a_canonical_genesis_identity() {
    const auto capabilities = qualified_capabilities();
    const auto tools = qualified_tools();
    const auto workflow = write_workflow();
    ActionProposalEmitter empty("", &capabilities, &tools);
    const auto proposal = empty.propose(ready_run(), workflow.steps.front(), workflow,
                                        "notes.txt", digest('e'), allowed_decision(),
                                        completed_receipt(), 200U);
    require(proposal.disposition == ProposalDisposition::refused,
            "a proposal without a canonical Genesis identity is refused");
    require(proposal.rationale.find("Genesis identity") != std::string::npos,
            "the refusal names the missing identity");
}

void test_safety_gate_denial_stops_the_proposal_at_genesis() {
    const auto capabilities = qualified_capabilities();
    const auto tools = qualified_tools();
    const auto workflow = write_workflow();
    const auto proposal = emit(capabilities, tools, ready_run(), workflow.steps.front(),
                               workflow, "notes.txt", allowed_decision(),
                               safety_denied_receipt());
    require(proposal.disposition == ProposalDisposition::refused,
            "a safety-gate denial refuses the proposal");
    require(proposal.rationale.find("safety gate") != std::string::npos,
            "the refusal names the safety gate");
    require(proposal.authority_owner == "p25",
            "a refused proposal still names P25 as the authority owner");
}

void test_denied_run_is_not_proposed() {
    const auto capabilities = qualified_capabilities();
    const auto tools = qualified_tools();
    const auto workflow = write_workflow();
    const auto proposal = emit(capabilities, tools, denied_run(), workflow.steps.front(),
                               workflow, "notes.txt", allowed_decision(),
                               completed_receipt());
    require(proposal.disposition == ProposalDisposition::refused,
            "a denied run produces no proposal");
    require(proposal.rationale.find("denied") != std::string::npos,
            "the refusal names the denied run");
}

void test_refuses_unqualified_or_undeclared_tool() {
    const auto capabilities = qualified_capabilities();
    const auto workflow = write_workflow();

    AdapterRegistry unqualified;
    AdapterRegistry declared_only;
    AdapterDescriptor descriptor("tool-writer", AdapterKind::tool, AdapterState::declared,
                                 {"write", "read"});
    require(declared_only.declare(descriptor),
            "declared only: never probed or qualified");

    const auto expired = emit(capabilities, unqualified, ready_run(),
                              workflow.steps.front(), workflow, "notes.txt",
                              allowed_decision(), completed_receipt());
    require(expired.disposition == ProposalDisposition::refused,
            "an unregistered tool adapter produces no proposal");

    const auto unproven = emit(capabilities, declared_only, ready_run(),
                               workflow.steps.front(), workflow, "notes.txt",
                               allowed_decision(), completed_receipt());
    require(unproven.disposition == ProposalDisposition::refused,
            "a declared but unqualified tool adapter produces no proposal");
    require(unproven.rationale.find("not qualified") != std::string::npos,
            "the refusal names the missing qualification");

    auto tools = qualified_tools();
    const auto undeclared = emit(capabilities, tools, ready_run(),
                                 workflow.steps.front(), workflow, "notes.txt",
                                 allowed_decision(), completed_receipt());
    require(undeclared.disposition == ProposalDisposition::proposed
                || undeclared.disposition == ProposalDisposition::approval_required,
            "a declared action is proposable");

    WorkflowStep delete_step = workflow.steps.front();
    delete_step.action = "delete";   // the adapter does not declare it
    const auto undeclared_action = emit(capabilities, tools, ready_run(), delete_step,
                                        workflow, "notes.txt", allowed_decision(),
                                        completed_receipt());
    require(undeclared_action.disposition == ProposalDisposition::refused,
            "an action the adapter does not declare is refused");
}

void test_refuses_when_the_capability_model_will_not_permit() {
    const auto tools = qualified_tools();
    const auto workflow = write_workflow();
    SelfCapabilityModel model("organism-1", 8U);
    model.declare_capability("tool-writer", "route", digest('c'), 100U);
    // Declared but never qualified: competence is not established.
    const auto proposal = emit(model, tools, ready_run(), workflow.steps.front(),
                               workflow, "notes.txt", allowed_decision(),
                               completed_receipt());
    require(proposal.disposition == ProposalDisposition::refused,
            "an unqualified capability produces no proposal");
    require(proposal.rationale.find("capability model") != std::string::npos,
            "the refusal names the capability model");
}

void test_refuses_a_decision_without_evidence() {
    const auto capabilities = qualified_capabilities();
    const auto tools = qualified_tools();
    const auto workflow = write_workflow();
    AuthorizationDecision no_evidence{true, true, true, "not-a-digest"};
    const auto proposal = emit(capabilities, tools, ready_run(), workflow.steps.front(),
                               workflow, "notes.txt", no_evidence, completed_receipt());
    require(proposal.disposition == ProposalDisposition::refused,
            "an authorization input without evidence digest is refused");
    require(proposal.rationale.find("evidence digest") != std::string::npos,
            "the refusal names the missing evidence digest");
}

void test_refuses_a_resource_that_is_not_one_concrete_target() {
    const auto capabilities = qualified_capabilities();
    const auto tools = qualified_tools();
    const auto workflow = write_workflow();
    for (const char* resource : {"*.txt", "notes.*", "../escape.txt", "a/../b.txt",
                                 ""}) {
        const auto proposal = emit(capabilities, tools, ready_run(),
                                   workflow.steps.front(), workflow, resource,
                                   allowed_decision(), completed_receipt());
        require(proposal.disposition == ProposalDisposition::refused, resource);
        require(proposal.rationale.find("concrete") != std::string::npos, resource);
    }
}

void test_multi_resource_work_requires_a_proposal_per_resource() {
    const auto capabilities = qualified_capabilities();
    const auto tools = qualified_tools();
    const auto workflow = write_workflow();
    // A move is two proposals, because authority for a source is not authority
    // for a destination.
    const auto source = emit(capabilities, tools, ready_run(), workflow.steps.front(),
                             workflow, "source.txt", allowed_decision(),
                             completed_receipt());
    const auto destination = emit(capabilities, tools, ready_run(), workflow.steps.front(),
                                  workflow, "destination.txt", allowed_decision(),
                                  completed_receipt());
    require(source.proposal_id != destination.proposal_id,
            "source and destination are distinct proposals");
    require(source.resource == "source.txt" && destination.resource == "destination.txt",
            "each proposal names exactly one of the two resources");
}

void test_step_without_approval_is_proposed_for_p25_only() {
    const auto capabilities = qualified_capabilities();
    const auto tools = qualified_tools();
    auto workflow = write_workflow();
    workflow.steps.front().requires_approval = false;
    const auto proposal = emit(capabilities, tools, ready_run(), workflow.steps.front(),
                               workflow, "notes.txt", allowed_decision(),
                               completed_receipt());
    require(proposal.disposition == ProposalDisposition::proposed,
            "a step not requiring approval is still only proposed, never authorized");
    require(proposal.authority_owner == "p25",
            "even an approval-free proposal defers authority to P25");
}

void test_preflight_and_proposal_agree_on_readiness() {
    // The emitter is downstream of preflight. With the default conservative
    // approval mode (risky_actions) preflight lands in pending_approval and the
    // proposal is approval_required; with approval never demanded it lands in
    // ready and the proposal is still only proposed. Neither path authorizes
    // anything: both defer to P25.
    AgentRegistry agents;
    require(agents.register_agent(agent_def()), "agent registers");
    auto unattended_writer = agent_def();
    unattended_writer.agent_id = "agent-3";
    unattended_writer.display_name = "Unattended writer";
    unattended_writer.approval_mode = genesis::agents::ApprovalMode::never;
    require(agents.register_agent(unattended_writer), "unattended writer registers");
    auto unattended_reader = agent_def();
    unattended_reader.agent_id = "agent-2";
    unattended_reader.display_name = "Unattended reader";
    unattended_reader.approval_mode = genesis::agents::ApprovalMode::never;
    unattended_reader.tool_grants.front().filesystem_write_allowed = false;
    unattended_reader.tool_grants.front().allowed_actions = {"read"};
    require(agents.register_agent(unattended_reader), "unattended reader registers");
    AdapterRegistry models;
    AdapterDescriptor model("model-1", AdapterKind::model, AdapterState::declared, {"chat"});
    require(models.declare(model), "model declares");
    require(models.record_probe("model-1", true, "observer-1", digest('c'), 100U),
            "model probe recorded");
    require(models.qualify("model-1", "qualifier-1", digest('9'), 100U, 10'000U),
            "model qualifies");
    const auto auto_tools = qualified_tools();
    const genesis::agents::AgentRuntime runtime;

    // approval_mode=never does not silence a filesystem write grant.
    auto write_request = ready_run().request;
    write_request.agent_id = "agent-3";
    const auto still_gated = runtime.prepare(write_request, agents, write_workflow(),
                                            models, auto_tools);
    require(still_gated.state == RunState::pending_approval,
            "a filesystem-write grant requires approval regardless of the agent mode");

    auto read_request = ready_run().request;
    read_request.agent_id = "agent-2";
    auto read_only = write_workflow();
    read_only.steps.front().action = "read";
    read_only.steps.front().requires_approval = false;
    const auto ready = runtime.prepare(read_request, agents, read_only,
                                       models, auto_tools);
    require(ready.state == RunState::ready,
            "a read-only, unattended workflow preflights to ready");
    const auto ready_proposal = emit(capabilities(), auto_tools, ready,
                                     read_only.steps.front(), read_only,
                                     "notes.txt", allowed_decision(), completed_receipt());
    require(ready_proposal.disposition == ProposalDisposition::proposed,
            "a ready run proposes; it still does not authorize");
    require(ready_proposal.run_id == "run-1",
            "the proposal correlates to the preflighted run");

    // An action the agent's grant does not cover is denied by preflight, so
    // there is nothing left to propose.
    auto ungranted = ready_run().request;
    ungranted.agent_id = "agent-2";
    const auto denied = runtime.prepare(ungranted, agents, write_workflow(),
                                        models, auto_tools);
    require(denied.state == RunState::denied,
            "an action outside the agent grant is denied before any proposal");
}

void test_disposition_vocabulary_is_stable() {
    require(genesis::agents::to_string(ProposalDisposition::refused) == "refused", "refused");
    require(genesis::agents::to_string(ProposalDisposition::proposed) == "proposed", "proposed");
    require(genesis::agents::to_string(ProposalDisposition::approval_required)
                == "approval_required", "approval_required");
}

} // namespace

/**
 * The suite, wrapped so that a failing check prints what failed and
 * returns a non-zero exit code. The Microsoft C++ runtime shows a modal
 * dialog on an unhandled exception, and a modal dialog waits for a key
 * press that never comes.
 */
static int run_suite() {
test_emits_an_evidence_bearing_proposal();
    test_proposal_is_stable_for_the_same_intent();
    test_refuses_without_a_canonical_genesis_identity();
    test_safety_gate_denial_stops_the_proposal_at_genesis();
    test_denied_run_is_not_proposed();
    test_refuses_unqualified_or_undeclared_tool();
    test_refuses_when_the_capability_model_will_not_permit();
    test_refuses_a_decision_without_evidence();
    test_refuses_a_resource_that_is_not_one_concrete_target();
    test_multi_resource_work_requires_a_proposal_per_resource();
    test_step_without_approval_is_proposed_for_p25_only();
    test_preflight_and_proposal_agree_on_readiness();
    test_disposition_vocabulary_is_stable();

    if (failures != 0) {
        std::cerr << failures << " action-proposal assertion(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "genesis action proposal tests passed\n";
    return EXIT_SUCCESS;
}

int main() {
    return genesis::testing::run(run_suite, "action_proposal_tests");
}