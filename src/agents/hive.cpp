#include "genesis/agents/hive.hpp"

#include <algorithm>
#include <utility>

namespace genesis::agents {

namespace {

/// Whether a message is well formed. A message that names no capability
/// exercises nothing, and a message with no body says nothing.
bool is_well_formed(const HiveMessage& message) {
    return !message.message_id.empty() && !message.sender_id.empty() &&
           !message.capability_id.empty() && !message.action.empty() &&
           !message.body.empty();
}

} // namespace

std::string_view to_string(HiveRole role) noexcept {
    switch (role) {
        case HiveRole::coordinator:
            return "coordinator";
        case HiveRole::researcher:
            return "researcher";
        case HiveRole::analyst:
            return "analyst";
        case HiveRole::writer:
            return "writer";
        case HiveRole::coder:
            return "coder";
        case HiveRole::reviewer:
            return "reviewer";
        case HiveRole::executor:
            return "executor";
        case HiveRole::specialist:
            return "specialist";
    }
    return "specialist";
}

std::string_view to_string(MessageKind kind) noexcept {
    switch (kind) {
        case MessageKind::request:
            return "request";
        case MessageKind::response:
            return "response";
        case MessageKind::broadcast:
            return "broadcast";
        case MessageKind::delegate:
            return "delegate";
        case MessageKind::collaborate:
            return "collaborate";
        case MessageKind::review:
            return "review";
    }
    return "request";
}

std::string_view to_string(MessageError error) noexcept {
    switch (error) {
        case MessageError::none:
            return "none";
        case MessageError::invalid_message:
            return "invalid_message";
        case MessageError::unknown_sender:
            return "unknown_sender";
        case MessageError::unknown_recipient:
            return "unknown_recipient";
        case MessageError::sender_not_authorised:
            return "sender_not_authorised";
        case MessageError::action_not_permitted:
            return "action_not_permitted";
        case MessageError::sender_grant_expired:
            return "sender_grant_expired";
    }
    return "none";
}

std::string_view to_string(HiveDelegationError error) noexcept {
    switch (error) {
        case HiveDelegationError::none:
            return "none";
        case HiveDelegationError::unknown_parent:
            return "unknown_parent";
        case HiveDelegationError::unknown_child:
            return "unknown_child";
        case HiveDelegationError::parent_lacks_capability:
            return "parent_lacks_capability";
        case HiveDelegationError::widens_scope:
            return "widens_scope";
        case HiveDelegationError::invalid_delegation:
            return "invalid_delegation";
    }
    return "none";
}

bool Hive::register_agent(HiveAgent agent) {
    if (agent.agent_id.empty()) {
        return false;
    }
    if (index_.find(agent.agent_id) != index_.end()) {
        // Two agents with one identifier would make every message to that
        // identifier ambiguous, and an ambiguous recipient is a message
        // nobody can audit.
        return false;
    }
    index_.emplace(agent.agent_id, agents_.size());
    agents_.push_back(std::move(agent));
    return true;
}

const HiveAgent* Hive::find_agent(std::string_view agent_id) const noexcept {
    const auto it = index_.find(agent_id);
    if (it == index_.end()) {
        return nullptr;
    }
    return &agents_[it->second];
}

std::vector<HiveAgent> Hive::agents() const {
    return agents_;
}

std::size_t Hive::size() const noexcept {
    return agents_.size();
}

bool Hive::grant(AuthorityGrant grant) {
    return authority_->register_grant(std::move(grant));
}

HiveDelegationError Hive::delegate(const std::string& parent_id,
                                   const std::string& child_id,
                                   const std::string& capability_id,
                                   const std::set<std::string, std::less<>>& actions,
                                   bool network_allowed,
                                   bool filesystem_write_allowed,
                                   std::uint64_t expires_at) {
    if (parent_id.empty() || child_id.empty() || capability_id.empty() || actions.empty()) {
        return HiveDelegationError::invalid_delegation;
    }
    if (find_agent(parent_id) == nullptr) {
        return HiveDelegationError::unknown_parent;
    }
    if (find_agent(child_id) == nullptr) {
        return HiveDelegationError::unknown_child;
    }
    if (parent_id == child_id) {
        // A subject delegating to itself is not a delegation, and it would
        // launder a widened scope through the child.
        return HiveDelegationError::invalid_delegation;
    }

    const AuthorityGrant* parent = authority_->find_grant(parent_id, capability_id);
    if (parent == nullptr) {
        return HiveDelegationError::parent_lacks_capability;
    }

    // The child's own scope is a second ceiling. A delegation may narrow
    // the parent or the child; it may not widen either. A child that
    // already holds a broad grant and is handed a narrow one is narrowed,
    // which is the ordinary case: a coordinator that wants a researcher
    // to read only, when the researcher may also write.
    const AuthorityGrant* existing = authority_->find_grant(child_id, capability_id);
    if (existing != nullptr) {
        for (const auto& action : actions) {
            if (!AuthorityRegistry::permits_action(*existing, action)) {
                return HiveDelegationError::widens_scope;
            }
        }
        if (network_allowed && !existing->network_allowed) {
            return HiveDelegationError::widens_scope;
        }
        if (filesystem_write_allowed && !existing->filesystem_write_allowed) {
            return HiveDelegationError::widens_scope;
        }
        if (existing->expires_at != 0U) {
            if (expires_at == 0U || expires_at > existing->expires_at) {
                return HiveDelegationError::widens_scope;
            }
        }
    }

    // The authority engine holds the rule that a delegation may only
    // narrow. The hive does not restate it, and cannot be made to mean
    // something else by changing code here.
    AuthorityGrant child;
    const DelegationError outcome = authority_->narrow(*parent, child_id, capability_id,
                                                      actions, network_allowed,
                                                      filesystem_write_allowed, expires_at,
                                                      child);
    if (outcome != DelegationError::none) {
        return HiveDelegationError::widens_scope;
    }
    if (existing != nullptr) {
        // The child's previous grant is replaced by the narrowed one. The
        // revocation is expected to succeed: the grant was found a moment
        // ago, and nothing else can have taken it.
        if (!authority_->revoke(child_id, capability_id)) {
            return HiveDelegationError::widens_scope;
        }
    }
    if (!authority_->register_grant(std::move(child))) {
        return HiveDelegationError::widens_scope;
    }
    return HiveDelegationError::none;
}

MessageError Hive::send(HiveMessage message, std::uint64_t at) {
    if (!is_well_formed(message)) {
        return MessageError::invalid_message;
    }
    if (find_agent(message.sender_id) == nullptr) {
        return MessageError::unknown_sender;
    }
    if (!message.recipient_id.empty() && find_agent(message.recipient_id) == nullptr) {
        return MessageError::unknown_recipient;
    }

    // The sender must be permitted to do what the message says. A message
    // is a claim about what its sender is doing, and an unauthorised claim
    // is a refusal.
    const AuthorityGrant* grant = authority_->find_grant(message.sender_id,
                                                         message.capability_id);
    if (grant == nullptr) {
        return MessageError::sender_not_authorised;
    }
    if (!AuthorityRegistry::is_live(*grant, at)) {
        return MessageError::sender_grant_expired;
    }
    if (!AuthorityRegistry::permits_action(*grant, message.action)) {
        return MessageError::action_not_permitted;
    }

    history_.push_back(message);
    if (message.recipient_id.empty()) {
        // A broadcast goes to every agent but the sender: an agent does not
        // receive its own broadcast back.
        for (const auto& agent : agents_) {
            if (agent.agent_id == message.sender_id) {
                continue;
            }
            HiveMessage copy = message;
            copy.recipient_id = agent.agent_id;
            queue_.push_back(std::move(copy));
        }
    } else {
        queue_.push_back(std::move(message));
    }
    return MessageError::none;
}

std::vector<HiveMessage> Hive::inbox(std::string_view agent_id) const {
    std::vector<HiveMessage> out;
    for (const auto& message : queue_) {
        if (message.recipient_id == agent_id) {
            out.push_back(message);
        }
    }
    return out;
}

std::optional<HiveMessage> Hive::take(std::string_view agent_id) {
    const auto it = std::find_if(queue_.begin(), queue_.end(),
                                 [&](const HiveMessage& m) { return m.recipient_id == agent_id; });
    if (it == queue_.end()) {
        return std::nullopt;
    }
    HiveMessage message = *it;
    queue_.erase(it);
    return message;
}

std::size_t Hive::pending() const noexcept {
    return queue_.size();
}

const std::vector<HiveMessage>& Hive::history() const noexcept {
    return history_;
}

AuthorityRegistry& Hive::authority() noexcept {
    return *authority_;
}

} // namespace genesis::agents
