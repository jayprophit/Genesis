#include "genesis/agents/authority.hpp"

namespace genesis::agents {

std::string_view to_string(AuthorityDecision decision) noexcept {
    switch (decision) {
    case AuthorityDecision::granted:
        return "granted";
    case AuthorityDecision::action_not_permitted:
        return "action_not_permitted";
    case AuthorityDecision::capability_not_granted:
        return "capability_not_granted";
    case AuthorityDecision::expired:
        return "expired";
    case AuthorityDecision::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

std::string_view to_string(DelegationError error) noexcept {
    switch (error) {
    case DelegationError::none:
        return "none";
    case DelegationError::parent_lacks_capability:
        return "parent_lacks_capability";
    case DelegationError::action_widens_scope:
        return "action_widens_scope";
    case DelegationError::network_widens_scope:
        return "network_widens_scope";
    case DelegationError::filesystem_widens_scope:
        return "filesystem_widens_scope";
    case DelegationError::lifetime_widens_scope:
        return "lifetime_widens_scope";
    case DelegationError::invalid_parent:
        return "invalid_parent";
    case DelegationError::invalid_child:
        return "invalid_child";
    }
    return "unknown";
}

AuthorityRegistry AuthorityRegistry::with_default_deny() {
    return AuthorityRegistry{};
}

bool AuthorityRegistry::register_grant(AuthorityGrant grant) {
    if (grant.subject_id.empty() || grant.capability_id.empty()) {
        return false;
    }
    auto& by_capability = grants_[grant.subject_id];
    if (by_capability.find(grant.capability_id) != by_capability.end()) {
        // A second grant for the same subject and capability would shadow
        // the first silently. Refusing it keeps the scope visible.
        return false;
    }
    by_capability.emplace(grant.capability_id, std::move(grant));
    return true;
}

bool AuthorityRegistry::revoke(const std::string& subject_id, const std::string& capability_id) {
    auto subject = grants_.find(subject_id);
    if (subject == grants_.end()) {
        return false;
    }
    const bool removed = subject->second.erase(capability_id) > 0U;
    if (subject->second.empty()) {
        grants_.erase(subject);
    }
    return removed;
}

const AuthorityGrant* AuthorityRegistry::find_grant(const std::string& subject_id,
                                                    const std::string& capability_id) const {
    const auto subject = grants_.find(subject_id);
    if (subject == grants_.end()) {
        return nullptr;
    }
    const auto capability = subject->second.find(capability_id);
    if (capability == subject->second.end()) {
        return nullptr;
    }
    return &capability->second;
}

std::vector<std::string> AuthorityRegistry::capabilities_of(const std::string& subject_id) const {
    std::vector<std::string> out;
    const auto subject = grants_.find(subject_id);
    if (subject == grants_.end()) {
        return out;
    }
    out.reserve(subject->second.size());
    for (const auto& entry : subject->second) {
        out.push_back(entry.first);
    }
    return out;
}

std::size_t AuthorityRegistry::size() const noexcept {
    std::size_t total = 0U;
    for (const auto& subject : grants_) {
        total += subject.second.size();
    }
    return total;
}

bool AuthorityRegistry::is_live(const AuthorityGrant& grant, std::uint64_t at) noexcept {
    if (grant.expires_at == 0U) {
        return true;
    }
    return at < grant.expires_at;
}

bool AuthorityRegistry::permits_action(const AuthorityGrant& grant,
                                        const std::string& action) noexcept {
    return grant.allowed_actions.find(action) != grant.allowed_actions.end();
}

AuthorityDecision AuthorityRegistry::evaluate(const CapabilityRequest& request) const {
    if (request.subject_id.empty() || request.capability_id.empty() || request.action.empty()) {
        return AuthorityDecision::invalid_request;
    }
    const AuthorityGrant* grant = find_grant(request.subject_id, request.capability_id);
    if (grant == nullptr) {
        // Default deny. There is no implicit capability.
        return AuthorityDecision::capability_not_granted;
    }
    if (!is_live(*grant, request.requested_at)) {
        return AuthorityDecision::expired;
    }
    if (!permits_action(*grant, request.action)) {
        return AuthorityDecision::action_not_permitted;
    }
    return AuthorityDecision::granted;
}

DelegationError AuthorityRegistry::narrow(const AuthorityGrant& parent,
                                          const std::string& child_subject_id,
                                          const std::string& capability_id,
                                          const std::set<std::string, std::less<>>& actions,
                                          bool network_allowed,
                                          bool filesystem_write_allowed,
                                          std::uint64_t expires_at,
                                          AuthorityGrant& out) const {
    if (parent.subject_id.empty() || parent.capability_id.empty() || child_subject_id.empty()) {
        return DelegationError::invalid_parent;
    }
    if (child_subject_id == parent.subject_id) {
        // A subject delegating to itself is not a delegation, and it would
        // be a way to launder a widened scope through the child.
        return DelegationError::invalid_child;
    }
    if (parent.capability_id != capability_id) {
        // The child may not be granted a capability the parent does not
        // hold. The parent's own capability is the ceiling.
        return DelegationError::parent_lacks_capability;
    }
    if (actions.empty()) {
        // A delegation with no actions grants nothing, and a grant that
        // grants nothing is a grant that will be read as if it granted
        // something.
        return DelegationError::action_widens_scope;
    }
    for (const auto& action : actions) {
        if (!permits_action(parent, action)) {
            return DelegationError::action_widens_scope;
        }
    }
    if (network_allowed && !parent.network_allowed) {
        return DelegationError::network_widens_scope;
    }
    if (filesystem_write_allowed && !parent.filesystem_write_allowed) {
        return DelegationError::filesystem_widens_scope;
    }
    if (parent.expires_at != 0U) {
        // A parent with a bounded lifetime may not hand out a longer one.
        // A parent that never expires may hand out any lifetime, including
        // a bounded one, which is a narrowing.
        if (expires_at == 0U || expires_at > parent.expires_at) {
            return DelegationError::lifetime_widens_scope;
        }
    }
    if (parent.allowed_actions.find(capability_id) != parent.allowed_actions.end()) {
        // The capability is also an action name in some policies. Allowing
        // the child to hold the capability as an action the parent does not
        // permit would be a widening.
        if (!permits_action(parent, capability_id) &&
            actions.find(capability_id) != actions.end()) {
            return DelegationError::action_widens_scope;
        }
    }

    AuthorityGrant child;
    child.subject_id = child_subject_id;
    child.capability_id = capability_id;
    child.allowed_actions = actions;
    child.network_allowed = network_allowed;
    child.filesystem_write_allowed = filesystem_write_allowed;
    child.granted_at = parent.granted_at;
    child.expires_at = expires_at;
    out = std::move(child);
    return DelegationError::none;
}

DelegationError AuthorityRegistry::delegate(const AuthorityGrant& parent,
                                             const std::string& child_subject_id,
                                             const std::string& capability_id,
                                             const std::set<std::string, std::less<>>& actions,
                                             bool network_allowed,
                                             bool filesystem_write_allowed,
                                             std::uint64_t expires_at,
                                             AuthorityGrant& out) {
    const DelegationError error = narrow(parent, child_subject_id, capability_id, actions,
                                         network_allowed, filesystem_write_allowed, expires_at, out);
    if (error != DelegationError::none) {
        return error;
    }
    // The derived grant is only retained when the child does not already
    // hold one for the capability. A silent shadow is how a scope widens.
    if (find_grant(child_subject_id, capability_id) != nullptr) {
        return DelegationError::invalid_child;
    }
    return register_grant(out) ? DelegationError::none : DelegationError::invalid_child;
}

} // namespace genesis::agents
