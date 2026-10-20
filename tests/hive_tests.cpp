#include "genesis/agents/hive.hpp"
#include "test_support.hpp"

#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

namespace {

using genesis::agents::AuthorityGrant;
using genesis::agents::AuthorityRegistry;
using genesis::agents::Hive;
using genesis::agents::HiveAgent;
using genesis::agents::HiveDelegationError;
using genesis::agents::HiveMessage;
using genesis::agents::HiveRole;
using genesis::agents::MessageError;
using genesis::agents::MessageKind;

// The counter is the harness's, so the summary line reports
// the checks that were really made.
int& g_checks = genesis::testing::g_checks;

void require(bool condition, const char* message) {
    ++g_checks;
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_equal(MessageError actual, MessageError expected, const char* message) {
    ++g_checks;
    if (actual != expected) {
        std::cerr << "expected " << genesis::agents::to_string(expected) << " but got "
                  << genesis::agents::to_string(actual) << ": " << message << "\n";
        throw std::runtime_error(message);
    }
}

void require_equal(HiveDelegationError actual, HiveDelegationError expected,
                   const char* message) {
    ++g_checks;
    if (actual != expected) {
        std::cerr << "expected " << genesis::agents::to_string(expected) << " but got "
                  << genesis::agents::to_string(actual) << ": " << message << "\n";
        throw std::runtime_error(message);
    }
}

/// A grant that permits reading and writing documents and using the
/// network, for a bounded time.
AuthorityGrant make_grant(const std::string& subject,
                          const std::string& capability,
                          std::set<std::string, std::less<>> actions,
                          bool network,
                          bool filesystem_write,
                          std::uint64_t expires_at) {
    AuthorityGrant grant;
    grant.subject_id = subject;
    grant.capability_id = capability;
    grant.allowed_actions = std::move(actions);
    grant.network_allowed = network;
    grant.filesystem_write_allowed = filesystem_write;
    grant.granted_at = 1'000;
    grant.expires_at = expires_at;
    return grant;
}

HiveAgent make_agent(const std::string& id, HiveRole role) {
    HiveAgent agent;
    agent.agent_id = id;
    agent.display_name = id;
    agent.role = role;
    return agent;
}

HiveMessage make_message(const std::string& id,
                          const std::string& sender,
                          const std::string& recipient,
                          const std::string& capability,
                          const std::string& action,
                          std::string body) {
    HiveMessage message;
    message.message_id = id;
    message.sender_id = sender;
    message.recipient_id = recipient;
    message.kind = MessageKind::request;
    message.capability_id = capability;
    message.action = action;
    message.body = std::move(body);
    message.sent_at = 2'000;
    return message;
}

/// A hive with three agents and a coordinator that holds a broad grant.
struct Fixture {
    AuthorityRegistry authority;
    Hive hive{authority};

    Fixture() {
        // The fixture checks its own setup: a fixture that silently
        // failed to register an agent would make every test below it
        // prove something other than what it claims.
        require(hive.register_agent(make_agent("coordinator", HiveRole::coordinator)),
                "the coordinator should be registered");
        require(hive.register_agent(make_agent("researcher", HiveRole::researcher)),
                "the researcher should be registered");
        require(hive.register_agent(make_agent("coder", HiveRole::coder)),
                "the coder should be registered");

        require(hive.grant(make_grant("coordinator", "documents",
                                     {"read", "write", "summarise"}, true, true, 9'000)),
                "the coordinator's grant should be registered");
        require(hive.grant(make_grant("researcher", "documents", {"read"}, false, false, 9'000)),
                "the researcher's grant should be registered");
        require(hive.grant(make_grant("coder", "code", {"read", "write"}, false, true, 9'000)),
                "the coder's grant should be registered");
    }
};

// ---------------------------------------------------------------------------
// The agents.
// ---------------------------------------------------------------------------

void test_an_agent_is_registered() {
    Fixture f;
    require(f.hive.size() == 3, "the hive should hold three agents");
    require(f.hive.find_agent("researcher") != nullptr, "the researcher should be found");
    require(f.hive.find_agent("nobody") == nullptr, "an unknown agent should not be found");
}

void test_a_duplicate_agent_is_refused() {
    Fixture f;
    require(!f.hive.register_agent(make_agent("researcher", HiveRole::researcher)),
            "a duplicate agent id must be refused");
    require(f.hive.size() == 3, "the hive must not have grown");
}

void test_an_agent_without_an_id_is_refused() {
    Fixture f;
    require(!f.hive.register_agent(make_agent("", HiveRole::specialist)),
            "an agent without an id must be refused");
}

// ---------------------------------------------------------------------------
// Delegation: it may only narrow.
// ---------------------------------------------------------------------------

void test_a_delegation_may_narrow() {
    Fixture f;
    const auto error = f.hive.delegate("coordinator", "researcher", "documents",
                                       {"read"}, false, false, 8'000);
    require_equal(error, HiveDelegationError::none,
                  "the coordinator may narrow its scope to the researcher");
    const AuthorityGrant* child = f.authority.find_grant("researcher", "documents");
    require(child != nullptr, "the child should hold the narrowed grant");
    require(child->allowed_actions.size() == 1, "the child should hold one action");
    require(!child->network_allowed, "the child should not hold the network");
}

void test_a_delegation_cannot_widen_the_actions() {
    Fixture f;
    const auto error = f.hive.delegate("researcher", "coder", "documents",
                                       {"read", "write"}, false, false, 8'000);
    require_equal(error, HiveDelegationError::widens_scope,
                  "the researcher may not delegate an action it does not hold");
    require(f.authority.find_grant("coder", "documents") == nullptr,
            "the child must be left untouched");
}

void test_a_delegation_cannot_widen_the_network() {
    Fixture f;
    const auto error = f.hive.delegate("researcher", "coder", "documents",
                                       {"read"}, true, false, 8'000);
    require_equal(error, HiveDelegationError::widens_scope,
                  "the researcher may not delegate the network it does not hold");
}

void test_a_delegation_cannot_widen_the_filesystem() {
    Fixture f;
    const auto error = f.hive.delegate("researcher", "coder", "documents",
                                       {"read"}, false, true, 8'000);
    require_equal(error, HiveDelegationError::widens_scope,
                  "the researcher may not delegate filesystem writes it does not hold");
}

void test_a_delegation_cannot_widen_the_lifetime() {
    Fixture f;
    const auto error = f.hive.delegate("coordinator", "researcher", "documents",
                                       {"read"}, false, false, 99'000);
    require_equal(error, HiveDelegationError::widens_scope,
                  "a parent may not hand out a longer lifetime than it has");
}

void test_a_delegation_may_shorten_the_lifetime() {
    Fixture f;
    const auto error = f.hive.delegate("coordinator", "researcher", "documents",
                                       {"read"}, false, false, 5'000);
    require_equal(error, HiveDelegationError::none, "a shorter lifetime is a narrowing");
    const AuthorityGrant* child = f.authority.find_grant("researcher", "documents");
    require(child != nullptr, "the child should hold the grant");
    require(child->expires_at == 5'000, "the child should hold the shorter lifetime");
}

void test_a_delegation_cannot_invent_a_capability() {
    Fixture f;
    const auto error = f.hive.delegate("researcher", "coder", "documents",
                                       {"read", "summarise"}, false, false, 8'000);
    require_equal(error, HiveDelegationError::widens_scope,
                  "the researcher may not delegate an action it does not hold");
}

void test_a_delegation_needs_a_known_parent_and_child() {
    Fixture f;
    require_equal(f.hive.delegate("nobody", "coder", "documents", {"read"}, false, false, 8'000),
                  HiveDelegationError::unknown_parent,
                  "a delegation from an unknown agent must be refused");
    require_equal(f.hive.delegate("researcher", "nobody", "documents", {"read"}, false, false, 8'000),
                  HiveDelegationError::unknown_child,
                  "a delegation to an unknown agent must be refused");
}

void test_a_delegation_to_itself_is_refused() {
    Fixture f;
    require_equal(f.hive.delegate("researcher", "researcher", "documents",
                                  {"read"}, false, false, 8'000),
                  HiveDelegationError::invalid_delegation,
                  "an agent may not delegate to itself");
}

void test_a_delegation_needs_a_capability_the_parent_holds() {
    Fixture f;
    require_equal(f.hive.delegate("researcher", "coder", "code", {"read"}, false, false, 8'000),
                  HiveDelegationError::parent_lacks_capability,
                  "a delegation of a capability the parent lacks must be refused");
}

void test_a_delegation_chain_stays_narrow() {
    Fixture f;
    // The coordinator narrows to the researcher, and the researcher
    // narrows further to the coder. What the coder holds cannot exceed
    // what the researcher holds, which cannot exceed what the coordinator
    // held.
    require_equal(f.hive.delegate("coordinator", "researcher", "documents",
                                  {"read"}, false, false, 8'000),
                  HiveDelegationError::none, "the first delegation should be accepted");
    require_equal(f.hive.delegate("researcher", "coder", "documents",
                                  {"read"}, false, false, 8'000),
                  HiveDelegationError::none, "the second delegation should be accepted");
    // The researcher holds only "read", so it cannot hand out "write",
    // and the coordinator's broader scope cannot be reached through it.
    require_equal(f.hive.delegate("researcher", "coder", "documents",
                                  {"read", "write"}, false, false, 8'000),
                  HiveDelegationError::widens_scope,
                  "the chain must not widen at the second level");
}

void test_a_delegated_grant_is_enforceable() {
    Fixture f;
    require_equal(f.hive.delegate("coordinator", "researcher", "documents",
                                  {"read"}, false, false, 8'000),
                  HiveDelegationError::none, "the delegation should be accepted");
    // The researcher now holds only "read" on documents, and it was
    // already holding only "read", so a write is refused.
    const auto error = f.hive.send(
        make_message("m1", "researcher", "coder", "documents", "write", "rewrite"),
        2'000);
    require_equal(error, MessageError::action_not_permitted,
                  "the narrowed grant must not permit a write");
}

// ---------------------------------------------------------------------------
// Messaging: authorised before it is sent.
// ---------------------------------------------------------------------------

void test_a_message_from_an_authorised_agent_is_delivered() {
    Fixture f;
    const auto error = f.hive.send(
        make_message("m1", "researcher", "coder", "documents", "read", "please read"),
        2'000);
    require_equal(error, MessageError::none, "the message should be accepted");
    require(f.hive.pending() == 1, "one message should be waiting");
    const auto inbox = f.hive.inbox("coder");
    require(inbox.size() == 1, "the coder should have one message");
    require(inbox.front().body == "please read", "the message should be intact");
}

void test_a_message_from_an_unauthorised_agent_is_refused() {
    Fixture f;
    const auto error = f.hive.send(
        make_message("m1", "researcher", "coder", "documents", "write", "rewrite"),
        2'000);
    require_equal(error, MessageError::action_not_permitted,
                  "an action the grant does not permit must be refused");
    require(f.hive.pending() == 0, "a refused message must not be delivered");
    require(f.hive.history().empty(), "a refused message must not be recorded");
}

void test_a_message_about_a_capability_the_sender_lacks_is_refused() {
    Fixture f;
    const auto error = f.hive.send(
        make_message("m1", "researcher", "coder", "code", "read", "have a look"),
        2'000);
    require_equal(error, MessageError::sender_not_authorised,
                  "a capability the sender does not hold must be refused");
}

void test_a_message_from_an_unknown_sender_is_refused() {
    Fixture f;
    const auto error = f.hive.send(
        make_message("m1", "nobody", "coder", "documents", "read", "hello"),
        2'000);
    require_equal(error, MessageError::unknown_sender,
                  "an unknown sender must be refused");
}

void test_a_message_to_an_unknown_recipient_is_refused() {
    Fixture f;
    const auto error = f.hive.send(
        make_message("m1", "researcher", "nobody", "documents", "read", "hello"),
        2'000);
    require_equal(error, MessageError::unknown_recipient,
                  "an unknown recipient must be refused");
}

void test_a_malformed_message_is_refused() {
    Fixture f;
    HiveMessage no_body = make_message("m1", "researcher", "coder", "documents",
                                       "read", "");
    require_equal(f.hive.send(no_body, 2'000), MessageError::invalid_message,
                  "a message with no body must be refused");
    HiveMessage no_capability = make_message("m2", "researcher", "coder", "", "read", "hi");
    require_equal(f.hive.send(no_capability, 2'000), MessageError::invalid_message,
                  "a message with no capability must be refused");
    HiveMessage no_action = make_message("m3", "researcher", "coder", "documents", "", "hi");
    require_equal(f.hive.send(no_action, 2'000), MessageError::invalid_message,
                  "a message with no action must be refused");
}

void test_an_expired_grant_refuses_the_message() {
    Fixture f;
    // The coordinator's grant ends at 9,000, and the message is made
    // after that.
    const auto error = f.hive.send(
        make_message("m1", "coordinator", "coder", "documents", "write", "later"),
        10'000);
    require_equal(error, MessageError::sender_grant_expired,
                  "an expired grant must refuse the message");
}

void test_messages_are_delivered_in_order() {
    Fixture f;
    require_equal(f.hive.send(make_message("m1", "researcher", "coder", "documents", "read", "first"), 2'000), MessageError::none,
                 "the message should be accepted");
    require_equal(f.hive.send(make_message("m2", "researcher", "coder", "documents", "read", "second"), 2'100), MessageError::none,
                 "the message should be accepted");
    require_equal(f.hive.send(make_message("m3", "researcher", "coder", "documents", "read", "third"), 2'200), MessageError::none,
                 "the message should be accepted");
    const auto inbox = f.hive.inbox("coder");
    require(inbox.size() == 3, "the coder should have three messages");
    require(inbox[0].body == "first", "the first message should be first");
    require(inbox[1].body == "second", "the second message should be second");
    require(inbox[2].body == "third", "the third message should be third");
}

void test_a_message_is_taken_once() {
    Fixture f;
    require_equal(f.hive.send(make_message("m1", "researcher", "coder", "documents", "read", "once"), 2'000), MessageError::none,
                 "the message should be accepted");
    const auto taken = f.hive.take("coder");
    require(taken.has_value(), "the message should be there to take");
    require(taken->body == "once", "the taken message should be the one sent");
    require(f.hive.pending() == 0, "the queue should be empty");
    require(!f.hive.take("coder").has_value(), "a taken message should not come back");
}

void test_a_broadcast_reaches_everyone_but_the_sender() {
    Fixture f;
    HiveMessage broadcast = make_message("m1", "coordinator", "", "documents",
                                         "summarise", "stand up");
    broadcast.kind = MessageKind::broadcast;
    const auto error = f.hive.send(broadcast, 2'000);
    require_equal(error, MessageError::none, "the broadcast should be accepted");
    require(f.hive.pending() == 2, "the broadcast should reach the two other agents");
    require(f.hive.inbox("researcher").size() == 1, "the researcher should receive it");
    require(f.hive.inbox("coder").size() == 1, "the coder should receive it");
    require(f.hive.inbox("coordinator").empty(),
            "the sender should not receive its own broadcast");
}

void test_the_history_records_what_was_sent() {
    Fixture f;
    require_equal(f.hive.send(make_message("m1", "researcher", "coder", "documents", "read", "one"), 2'000), MessageError::none,
                 "the message should be accepted");
    require_equal(f.hive.send(make_message("m2", "researcher", "coder", "documents", "read", "two"), 2'100), MessageError::none,
                 "the message should be accepted");
    require(f.hive.history().size() == 2, "the history should hold both messages");
    require(f.hive.history()[0].body == "one", "the history should be in order");
}

void test_the_hive_borrows_its_authority() {
    Fixture f;
    // The registry is the one the caller holds, so a grant made through
    // the hive is visible to the caller, and the other way about.
    f.hive.grant(make_grant("coder", "network", {"fetch"}, true, false, 9'000));
    const auto* grant = f.authority.find_grant("coder", "network");
    require(grant != nullptr, "the grant should be in the shared registry");
    require(&f.hive.authority() == &f.authority, "the hive should use the caller's registry");
}

void test_a_refused_delegation_leaves_the_child_untouched() {
    Fixture f;
    const auto before = f.hive.agents().size();
    const auto error = f.hive.delegate("researcher", "coder", "documents",
                                       {"read", "write"}, false, false, 8'000);
    require_equal(error, HiveDelegationError::widens_scope, "the delegation must be refused");
    require(f.hive.agents().size() == before, "the hive must not have changed");
    require(f.authority.find_grant("coder", "documents") == nullptr,
            "the child must hold no grant it was refused");
}

} // namespace

/**
 * The suite, wrapped so that a failing check prints what failed and
 * returns a non-zero exit code. The Microsoft C++ runtime shows a modal
 * dialog on an unhandled exception, and a modal dialog waits for a key
 * press that never comes.
 */
static int run_suite() {
test_an_agent_is_registered();
    test_a_duplicate_agent_is_refused();
    test_an_agent_without_an_id_is_refused();
    test_a_delegation_may_narrow();
    test_a_delegation_cannot_widen_the_actions();
    test_a_delegation_cannot_widen_the_network();
    test_a_delegation_cannot_widen_the_filesystem();
    test_a_delegation_cannot_widen_the_lifetime();
    test_a_delegation_may_shorten_the_lifetime();
    test_a_delegation_cannot_invent_a_capability();
    test_a_delegation_needs_a_known_parent_and_child();
    test_a_delegation_to_itself_is_refused();
    test_a_delegation_needs_a_capability_the_parent_holds();
    test_a_delegation_chain_stays_narrow();
    test_a_delegated_grant_is_enforceable();
    test_a_message_from_an_authorised_agent_is_delivered();
    test_a_message_from_an_unauthorised_agent_is_refused();
    test_a_message_about_a_capability_the_sender_lacks_is_refused();
    test_a_message_from_an_unknown_sender_is_refused();
    test_a_message_to_an_unknown_recipient_is_refused();
    test_a_malformed_message_is_refused();
    test_an_expired_grant_refuses_the_message();
    test_messages_are_delivered_in_order();
    test_a_message_is_taken_once();
    test_a_broadcast_reaches_everyone_but_the_sender();
    test_the_history_records_what_was_sent();
    test_the_hive_borrows_its_authority();
    test_a_refused_delegation_leaves_the_child_untouched();
    return 0;
}

int main() {
    return genesis::testing::run(run_suite, "hive_tests");
}
