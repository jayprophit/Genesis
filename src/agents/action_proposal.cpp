#include "genesis/agents/action_proposal.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "genesis/runtime/runtime.hpp"

namespace genesis::agents {

namespace {

bool identifier(std::string_view value) {
    return !value.empty() && value.size() <= 128U
        && std::isalnum(static_cast<unsigned char>(value.front())) != 0
        && std::all_of(value.begin(), value.end(), [](unsigned char character) {
               return std::isalnum(character) != 0 || character == '_'
                   || character == '-' || character == '.';
           });
}

bool evidence_digest(std::string_view value) {
    return value.size() == 64U
        && std::all_of(value.begin(), value.end(), [](unsigned char character) {
               return std::isxdigit(character) != 0;
           });
}

// A resource names one concrete target. Patterns and absolute references are
// refused here for the same reason the policy layer refuses them for grants: a
// proposal is an ask, and an ask expressed as a pattern could be answered as a
// pattern.
bool concrete_resource(std::string_view value) {
    if (value.empty() || value.size() > 512U) return false;
    if (value.find("..") != std::string_view::npos) return false;
    return std::none_of(value.begin(), value.end(), [](char character) {
        return character == '*' || character == '?' || character == '['
            || character == ']' || character == '{' || character == '}';
    });
}

} // namespace

std::string_view to_string(ProposalDisposition disposition) noexcept {
    switch (disposition) {
    case ProposalDisposition::refused: return "refused";
    case ProposalDisposition::proposed: return "proposed";
    case ProposalDisposition::approval_required: return "approval_required";
    }
    return "refused";
}

std::string ActionProposal::digest() const {
    // Content-addressed and order-stable: the same intent must always produce
    // the same id, so a duplicate intent is recognizable instead of silently
    // becoming a second action.
    std::string material;
    material.reserve(512);
    material += genesis_id; material += '|';
    material += run_id; material += '|';
    material += capability_id; material += '|';
    material += tool_id; material += '|';
    material += action; material += '|';
    material += resource; material += '|';
    material += parameter_digest; material += '|';
    material += evidence_digest; material += '|';
    material += receipt_digest; material += '|';
    material += to_string(disposition); material += '|';
    material += std::to_string(proposed_at);
    return runtime::sha256(material);
}

ActionProposalEmitter::ActionProposalEmitter(
    std::string genesis_id,
    const cognition::SelfCapabilityModel* capabilities,
    const AdapterRegistry* tools)
    : genesis_id_(std::move(genesis_id)), capabilities_(capabilities), tools_(tools) {}

ActionProposal ActionProposalEmitter::refused(std::string rationale,
                                             const WorkflowStep& step,
                                             const std::string& resource,
                                             const std::string& parameter_digest,
                                             const organism::LoopReceipt& receipt,
                                             std::uint64_t proposed_at) const {
    ActionProposal proposal;
    proposal.genesis_id = genesis_id_;
    proposal.run_id = "";
    proposal.capability_id = "";
    proposal.tool_id = step.tool_id;
    proposal.action = step.action;
    proposal.resource = resource;
    proposal.parameter_digest = parameter_digest;
    proposal.evidence_digest = "";
    proposal.receipt_digest = receipt.digest();
    proposal.disposition = ProposalDisposition::refused;
    proposal.rationale = std::move(rationale);
    proposal.proposed_at = proposed_at;
    proposal.proposal_id = "apx-" + proposal.digest().substr(0, 16U);
    return proposal;
}

ActionProposal ActionProposalEmitter::propose(const RunRecord& run,
                                              const WorkflowStep& step,
                                              const WorkflowDefinition& workflow,
                                              const std::string& resource,
                                              const std::string& parameter_digest,
                                              const cognition::AuthorizationDecision& authorization,
                                              const organism::LoopReceipt& receipt,
                                              std::uint64_t proposed_at) const {
    if (!identifier(genesis_id_)) {
        return refused("canonical Genesis identity is missing or malformed", step,
                       resource, parameter_digest, receipt, proposed_at);
    }
    if (capabilities_ == nullptr || tools_ == nullptr) {
        return refused("proposal requires the capability model and tool registry", step,
                       resource, parameter_digest, receipt, proposed_at);
    }
    // A denied run has no authority context to propose within, and a loop that
    // did not complete has not observed anything to act on.
    if (run.state == RunState::denied) {
        return refused("run is denied: " + run.diagnostic, step, resource,
                       parameter_digest, receipt, proposed_at);
    }
    if (!run.request.run_id.empty() && run.request.workflow_id != workflow.workflow_id) {
        return refused("step belongs to a workflow the run did not prepare", step,
                       resource, parameter_digest, receipt, proposed_at);
    }
    if (!identifier(run.request.run_id)) {
        return refused("run id is missing or malformed", step, resource,
                       parameter_digest, receipt, proposed_at);
    }
    if (receipt.safety_denied) {
        // The safety gate is Genesis' own refusal. Proposing anyway would route
        // around the very stage that exists to stop it.
        return refused("safety gate denied this loop: no proposal is emitted", step,
                       resource, parameter_digest, receipt, proposed_at);
    }
    if (!evidence_digest(authorization.decision_evidence_digest)) {
        return refused("authorization decision carries no evidence digest", step,
                       resource, parameter_digest, receipt, proposed_at);
    }
    if (!concrete_resource(resource)) {
        return refused("resource is not one concrete in-workspace target", step,
                       resource, parameter_digest, receipt, proposed_at);
    }
    if (!evidence_digest(parameter_digest)) {
        return refused("parameter digest is missing or malformed", step, resource,
                       parameter_digest, receipt, proposed_at);
    }

    const auto* tool = tools_->find(step.tool_id);
    if (tool == nullptr || tool->kind != AdapterKind::tool) {
        return refused("no tool adapter is registered for this step", step, resource,
                       parameter_digest, receipt, proposed_at);
    }
    if (!tools_->is_qualified(step.tool_id, proposed_at)) {
        return refused("tool adapter is not qualified at the proposal time", step,
                       resource, parameter_digest, receipt, proposed_at);
    }
    if (!tool->capabilities.contains(step.action)) {
        return refused("tool adapter does not declare the requested action", step,
                       resource, parameter_digest, receipt, proposed_at);
    }
    if (!evidence_digest(tool->qualification_digest)) {
        return refused("tool adapter qualification carries no evidence digest", step,
                       resource, parameter_digest, receipt, proposed_at);
    }

    // The capability model is the gate that already knows identity, policy,
    // safety, qualification and calibrated competence. It reports whether an
    // effect MAY be executed; it does not grant anything here, and its answer
    // is carried as an input for P25 rather than as the decision.
    if (!capabilities_->may_execute(step.tool_id, authorization)) {
        return refused("capability model will not permit this effect", step, resource,
                       parameter_digest, receipt, proposed_at);
    }

    ActionProposal proposal;
    proposal.genesis_id = genesis_id_;
    proposal.run_id = run.request.run_id;
    proposal.capability_id = step.tool_id;
    proposal.tool_id = tool->adapter_id;
    proposal.action = step.action;
    proposal.resource = resource;
    proposal.parameter_digest = parameter_digest;
    proposal.evidence_digest = tool->qualification_digest;
    proposal.receipt_digest = receipt.digest();
    proposal.disposition = step.requires_approval ? ProposalDisposition::approval_required
                                                  : ProposalDisposition::proposed;
    proposal.rationale = "proposed for p25 evaluation from drive '" + receipt.drive.name
        + "' and goal '" + receipt.goal.description + "'";
    proposal.proposed_at = proposed_at;
    proposal.proposal_id = "apx-" + proposal.digest().substr(0, 16U);
    return proposal;
}

} // namespace genesis::agents