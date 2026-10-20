#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace genesis::agents {

/**
 * Authority: the boundary that decides what an agent may actually do.
 *
 * The agent platform already describes what an agent is *allowed* to do:
 * a ToolGrant names a tool, the actions permitted on it, and whether the
 * network or filesystem writes are permitted. What it did not have was
 * anything that EVALUATED a grant. A grant that nothing checks is a
 * description, not a boundary, and a description can be ignored.
 *
 * This module is that evaluation. It is the Genesis side of the Aetherius
 * P25 arrangement: Genesis proposes an action, the authority decides
 * whether the subject may perform it, and nothing that Genesis does can
 * widen its own scope.
 *
 * The rules, in order of precedence:
 *
 *  1. DEFAULT DENY. A capability that is not granted is refused. There is
 *     no implicit capability and no implied permission.
 *  2. A grant covers one capability. Holding a grant for one capability
 *     says nothing about any other.
 *  3. A grant covers the actions it lists. An action that is not listed
 *     is refused, even for a capability the subject holds.
 *  4. An expired grant is refused. Expiry is absolute, not advisory.
 *  5. A DELEGATED GRANT CAN ONLY NARROW. A child cannot hold a capability
 *     its parent lacks, an action the parent does not permit, a broader
 *     network or filesystem permission, or a longer lifetime. This is the
 *     rule that stops a nested worker from escalating.
 *
 * Every decision carries its reason. A refusal that cannot be explained
 * is a refusal that cannot be audited, and an un-auditable refusal is
 * indistinguishable from a bug.
 */

/// The outcome of evaluating a capability request.
enum class AuthorityDecision {
    /// The subject holds the grant and the action is permitted.
    granted,
    /// The subject holds the grant, but not for this action.
    action_not_permitted,
    /// The subject holds no grant for this capability at all.
    capability_not_granted,
    /// A grant exists but its lifetime has ended.
    expired,
    /// The request was malformed and could not be evaluated.
    invalid_request,
};

[[nodiscard]] std::string_view to_string(AuthorityDecision decision) noexcept;

/// A request to perform an action under a capability.
struct CapabilityRequest final {
    /// Who is asking.
    std::string subject_id;
    /// The capability being exercised.
    std::string capability_id;
    /// The specific action within the capability.
    std::string action;
    /// When the request was made, in the caller's clock.
    std::uint64_t requested_at{};
};

/// A grant of one capability to one subject, for a bounded set of actions.
struct AuthorityGrant final {
    /// Who holds the grant.
    std::string subject_id;
    /// The capability granted.
    std::string capability_id;
    /// The actions permitted. An empty set permits nothing.
    std::set<std::string, std::less<>> allowed_actions;
    /// Whether network access is permitted under this grant.
    bool network_allowed{false};
    /// Whether filesystem writes are permitted under this grant.
    bool filesystem_write_allowed{false};
    /// When the grant was issued.
    std::uint64_t granted_at{};
    /// When the grant ends. Zero means it never expires.
    std::uint64_t expires_at{};
};

/// Why a delegation was refused.
enum class DelegationError {
    /// The delegation was accepted.
    none,
    /// The parent holds no grant for the capability being delegated.
    parent_lacks_capability,
    /// The child was granted an action the parent does not permit.
    action_widens_scope,
    /// The child was granted network access the parent lacks.
    network_widens_scope,
    /// The child was granted filesystem writes the parent lacks.
    filesystem_widens_scope,
    /// The child was granted a longer lifetime than the parent has.
    lifetime_widens_scope,
    /// The parent grant is malformed.
    invalid_parent,
    /// The child subject is missing or the same as the parent.
    invalid_child,
};

[[nodiscard]] std::string_view to_string(DelegationError error) noexcept;

/**
 * The registry of grants, and the evaluator of requests against them.
 *
 * The registry holds grants keyed by subject and capability. A subject may
 * hold several grants for different capabilities and must hold a grant for
 * a capability to exercise it.
 */
class AuthorityRegistry final {
public:
    /// A registry that refuses everything until grants are registered.
    [[nodiscard]] static AuthorityRegistry with_default_deny();

    /**
     * Register a grant. Returns false when the grant is malformed or when
     * the subject already holds a grant for that capability: a second grant
     * for the same pair would silently shadow the first, and a silent
     * shadow is how a scope widens by accident.
     */
    [[nodiscard]] bool register_grant(AuthorityGrant grant);

    /// Remove a subject's grant for a capability. Returns whether one was removed.
    [[nodiscard]] bool revoke(const std::string& subject_id, const std::string& capability_id);

    /// The grant a subject holds for a capability, or null.
    [[nodiscard]] const AuthorityGrant* find_grant(const std::string& subject_id,
                                                    const std::string& capability_id) const;

    /// Every capability a subject holds a grant for.
    [[nodiscard]] std::vector<std::string> capabilities_of(const std::string& subject_id) const;

    /// How many grants are registered.
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * Evaluate a request against the registered grants.
     *
     * The decision is the only thing the caller acts on. A caller that
     * receives `granted` may perform the action; any other decision means
     * it must not, and the reason is available in the decision itself.
     */
    [[nodiscard]] AuthorityDecision evaluate(const CapabilityRequest& request) const;

    /**
     * Issue a narrowed grant to a child subject, derived from a parent's
     * grant, and register it.
     *
     * The child may hold no capability the parent lacks, no action the
     * parent does not permit, no network or filesystem permission the
     * parent lacks, and no longer lifetime than the parent has. A
     * delegation that would widen the scope is refused and `out` is left
     * untouched.
     */
    [[nodiscard]] DelegationError delegate(const AuthorityGrant& parent,
                                           const std::string& child_subject_id,
                                           const std::string& capability_id,
                                           const std::set<std::string, std::less<>>& actions,
                                           bool network_allowed,
                                           bool filesystem_write_allowed,
                                           std::uint64_t expires_at,
                                           AuthorityGrant& out);

    /**
     * Derive a narrowed grant without registering it. The same rules apply
     * as `delegate`; this is the form a caller uses when the child grant
     * is to be handed over rather than retained.
     */
    [[nodiscard]] DelegationError narrow(const AuthorityGrant& parent,
                                         const std::string& child_subject_id,
                                         const std::string& capability_id,
                                         const std::set<std::string, std::less<>>& actions,
                                         bool network_allowed,
                                         bool filesystem_write_allowed,
                                         std::uint64_t expires_at,
                                         AuthorityGrant& out) const;

    /**
     * Whether a grant is currently live at a given time. A grant with a
     * zero expiry never ends.
     */
    [[nodiscard]] static bool is_live(const AuthorityGrant& grant, std::uint64_t at) noexcept;

    /**
     * Whether a grant permits an action, ignoring expiry. Exposed so that
     * a caller can distinguish "this action was never permitted" from
     * "this action is not permitted any more".
     */
    [[nodiscard]] static bool permits_action(const AuthorityGrant& grant,
                                             const std::string& action) noexcept;

private:
    /// subject -> capability -> grant
    std::map<std::string, std::map<std::string, AuthorityGrant, std::less<>>, std::less<>> grants_;
};

} // namespace genesis::agents
