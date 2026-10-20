#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "genesis/agents/authority.hpp"

namespace genesis::agents {

/**
 * The hive: several agents, and the traffic between them.
 *
 * The authority engine answers "may this subject do this". It does not
 * answer "may this agent hand that scope to that agent", nor "may this
 * agent say this to that agent". Those are the questions that make a
 * set of agents an organism rather than a queue of workers, and they are
 * what the recovered AI Digital Twin design called a swarm with
 * delegation, a message bus and a capability-scoped task pool.
 *
 * Three properties hold, and the tests prove each one:
 *
 *  1. DELEGATION CAN ONLY NARROW. A parent hands a child a scope that is
 *     within its own, never outside it. This is not re-implemented here:
 *     the delegation goes through the authority engine, so the rule is
 *     the one that already governs grants, and a child cannot become a
 *     back door to a wider scope.
 *
 *  2. A DELEGATION CHAIN STAYS NARROW. A child may delegate further,
 *     and what it delegates is bounded by what it holds, which is
 *     bounded by what the root held. Escalation through two levels is
 *     impossible because each level can only subtract.
 *
 *  3. A MESSAGE IS AUTHORISED BEFORE IT IS SENT. A sender must hold a
 *     grant permitting the action the message exercises. A message from
 *     an agent that holds nothing is refused, and a message about an
 *     action its grant does not permit is refused. Default deny, as
 *     everywhere else in the authority engine.
 *
 * Nothing here executes anything. The hive routes and records; the
 * workers it addresses do the work, and the authority engine decides
 * whether they may.
 */

/// The role an agent plays in the hive.
enum class HiveRole {
    coordinator,
    researcher,
    analyst,
    writer,
    coder,
    reviewer,
    executor,
    specialist,
};

[[nodiscard]] std::string_view to_string(HiveRole role) noexcept;

/// What a message is for.
enum class MessageKind {
    /// A request for work.
    request,
    /// A reply to a request.
    response,
    /// A message to every agent at once.
    broadcast,
    /// A handoff of work, with the scope that travels with it.
    delegate,
    /// A request to work together.
    collaborate,
    /// A request for review.
    review,
};

[[nodiscard]] std::string_view to_string(MessageKind kind) noexcept;

/// One message between agents.
struct HiveMessage final {
    /// The message's own identifier.
    std::string message_id;
    /// Who sent it.
    std::string sender_id;
    /// Who it is for. Empty means every agent.
    std::string recipient_id;
    /// What it is for.
    MessageKind kind{MessageKind::request};
    /// The capability the message exercises.
    std::string capability_id;
    /// The action within that capability.
    std::string action;
    /// The message body.
    std::string body;
    /// When it was sent, in the caller's clock.
    std::uint64_t sent_at{};
    /// Whether the recipient is expected to answer.
    bool requires_response{false};
};

/// Why a message was refused.
enum class MessageError {
    /// The message was accepted.
    none,
    /// The message was malformed: no sender, no body, or no capability.
    invalid_message,
    /// The sender is not an agent of this hive.
    unknown_sender,
    /// The recipient is not an agent of this hive.
    unknown_recipient,
    /// The sender holds no grant for the capability the message exercises.
    sender_not_authorised,
    /// The sender's grant does not permit the action.
    action_not_permitted,
    /// The sender's grant has ended.
    sender_grant_expired,
};

[[nodiscard]] std::string_view to_string(MessageError error) noexcept;

/// Why a delegation was refused.
enum class HiveDelegationError {
    /// The delegation was accepted.
    none,
    /// The parent is not an agent of this hive.
    unknown_parent,
    /// The child is not an agent of this hive.
    unknown_child,
    /// The parent holds no grant for the capability being delegated.
    parent_lacks_capability,
    /// The delegation would widen the scope.
    widens_scope,
    /// The delegation was malformed.
    invalid_delegation,
};

[[nodiscard]] std::string_view to_string(HiveDelegationError error) noexcept;

/// One agent of the hive.
struct HiveAgent final {
    /// The agent's identifier.
    std::string agent_id;
    /// A name for it.
    std::string display_name;
    /// The role it plays.
    HiveRole role{HiveRole::specialist};
    /// The skills it claims. A claim, not a permission: authority decides.
    std::set<std::string, std::less<>> skills;
    /// How many tasks it may hold at once.
    std::size_t max_concurrent_tasks{1};
    /// How many it holds now.
    std::size_t in_flight{0};
};

/**
 * The hive.
 *
 * The authority registry is borrowed, not owned. Authority belongs to
 * Genesis as a whole, and a hive that kept a private copy of it would be
 * a place where a grant was true in one place and false in another.
 */
class Hive final {
public:
    explicit Hive(AuthorityRegistry& authority) noexcept : authority_(&authority) {}

    /// Register an agent. It holds no authority until grants are given.
    [[nodiscard]] bool register_agent(HiveAgent agent);

    /// The agent, or null.
    [[nodiscard]] const HiveAgent* find_agent(std::string_view agent_id) const noexcept;

    /// Every agent, in registration order.
    [[nodiscard]] std::vector<HiveAgent> agents() const;

    /// How many agents are registered.
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * Grant a capability to an agent, through the authority engine.
     *
     * The hive does not decide whether the grant is sound; the authority
     * engine does. This exists so that a caller has one place to grant
     * and one place to delegate.
     */
    [[nodiscard]] bool grant(AuthorityGrant grant);

    /**
     * Delegate a narrowed scope from one agent to another.
     *
     * The child may hold no capability the parent lacks, no action the
     * parent does not permit, no network or filesystem permission the
     * parent lacks, and no longer lifetime than the parent has. The
     * rule is the authority engine's, and a delegation that would widen
     * the scope is refused and the child is left as it was.
     */
    [[nodiscard]] HiveDelegationError delegate(const std::string& parent_id,
                                               const std::string& child_id,
                                               const std::string& capability_id,
                                               const std::set<std::string, std::less<>>& actions,
                                               bool network_allowed,
                                               bool filesystem_write_allowed,
                                               std::uint64_t expires_at);

    /**
     * Send a message.
     *
     * The sender must hold a grant that permits the action the message
     * exercises, and that grant must still be live. A message that is
     * refused is not delivered and is not recorded.
     */
    [[nodiscard]] MessageError send(HiveMessage message, std::uint64_t at);

    /// The messages waiting for an agent, oldest first.
    [[nodiscard]] std::vector<HiveMessage> inbox(std::string_view agent_id) const;

    /// Take the next message for an agent. The oldest first.
    [[nodiscard]] std::optional<HiveMessage> take(std::string_view agent_id);

    /// How many messages are undelivered.
    [[nodiscard]] std::size_t pending() const noexcept;

    /// Every message that was sent, in order. The hive's record.
    [[nodiscard]] const std::vector<HiveMessage>& history() const noexcept;

    /// The authority registry the hive uses.
    [[nodiscard]] AuthorityRegistry& authority() noexcept;

private:
    /// The agents, in registration order.
    std::vector<HiveAgent> agents_;
    /// agent id -> index into agents_
    std::map<std::string, std::size_t, std::less<>> index_;
    /// The undelivered messages, in the order they arrived.
    std::vector<HiveMessage> queue_;
    /// Every message sent.
    std::vector<HiveMessage> history_;
    /// The authority registry, borrowed.
    AuthorityRegistry* authority_;
};

} // namespace genesis::agents
