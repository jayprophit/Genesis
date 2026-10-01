#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "genesis/agents/platform.hpp"
#include "genesis/cognition/capability.hpp"
#include "genesis/organism/loop.hpp"

namespace genesis::agents {

// An action proposal is INTENT to perform one action. It is never an
// authorization and never an execution.
//
// The organism loop marks `plan_na` ("no action planner") and `action_na`
// ("no actuator; Ready never authorizes") as absent. This is the missing link
// at that boundary, and it is deliberately the smallest possible one: Genesis
// decides WHAT should happen, P25 decides WHETHER it may, Agent Bridge performs
// it, and the world verifies what happened.
//
// Invariants this type exists to hold:
//   PROPOSAL            != AUTHORIZATION
//   PROPOSAL            != EXECUTION
//   PROPOSAL ACCEPTED    != EFFECT ACHIEVED
//   ACTION ID           != GENESIS ID
//   EVIDENCE FOR ACTION != PERMISSION FOR ACTION
//
// Genesis mints no identity here: the canonical Genesis identity is supplied by
// the caller and carried through unchanged, so a proposal, its authorization and
// its result all correlate to one Genesis rather than creating a new one.
enum class ProposalDisposition : std::uint8_t {
    refused,             // Genesis declines to propose; the gate that failed is named
    proposed,            // emitted; P25 evaluation expected, no human decision expected
    approval_required,   // emitted; P25 evaluation expected AND a human decision expected
};

[[nodiscard]] std::string_view to_string(ProposalDisposition disposition) noexcept;

// The authority owner is always P25. Genesis records who must decide; it never
// records a decision of its own, and there is deliberately no "authorized"
// field, because a proposal that can express its own authority can grant it.
inline constexpr std::string_view AUTHORITY_OWNER{"p25"};

struct ActionProposal final {
    // Correlation, not identity. proposal_id is derived from the content so the
    // same intent yields the same id; genesis_id is the canonical identity.
    std::string proposal_id;
    std::string genesis_id;
    std::string run_id;
    std::string capability_id;

    // The requested effect: one capability, one tool, one action, one resource.
    // One resource per proposal is deliberate - an operation touching several
    // resources (move, copy) needs one proposal per resource, so authority can
    // never be read as covering a destination nobody approved.
    std::string tool_id;
    std::string action;
    std::string resource;

    // Evidence, not permission. parameter_digest fixes exactly what was
    // intended; evidence_digest and receipt_digest explain why, citing the
    // qualified adapter and the loop receipt that motivated the request.
    std::string parameter_digest;
    std::string evidence_digest;
    std::string receipt_digest;

    ProposalDisposition disposition{ProposalDisposition::refused};
    std::string rationale;
    std::string authority_owner{std::string(AUTHORITY_OWNER)};
    std::uint64_t proposed_at{};

    [[nodiscard]] std::string digest() const;
};

// Emits proposals. It never performs anything and never authorizes anything.
class ActionProposalEmitter final {
public:
    // genesis_id must be the canonical Genesis identity supplied by the owner.
    // The emitter deliberately has no way to create one.
    ActionProposalEmitter(std::string genesis_id,
                          const cognition::SelfCapabilityModel* capabilities,
                          const AdapterRegistry* tools);

    [[nodiscard]] const std::string& genesis_id() const noexcept { return genesis_id_; }

    // Proposes one effect. Every failure path returns a refused proposal naming
    // the gate that refused it, rather than throwing or silently dropping the
    // request: a refusal is a valid system outcome and must be legible.
    [[nodiscard]] ActionProposal propose(const RunRecord& run,
                                         const WorkflowStep& step,
                                         const WorkflowDefinition& workflow,
                                         const std::string& resource,
                                         const std::string& parameter_digest,
                                         const cognition::AuthorizationDecision& authorization,
                                         const organism::LoopReceipt& receipt,
                                         std::uint64_t proposed_at) const;

private:
    [[nodiscard]] ActionProposal refused(std::string rationale,
                                         const WorkflowStep& step,
                                         const std::string& resource,
                                         const std::string& parameter_digest,
                                         const organism::LoopReceipt& receipt,
                                         std::uint64_t proposed_at) const;

    std::string genesis_id_;
    const cognition::SelfCapabilityModel* capabilities_;
    const AdapterRegistry* tools_;
};

} // namespace genesis::agents