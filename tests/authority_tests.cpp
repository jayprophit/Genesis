#include "genesis/agents/authority.hpp"

#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

namespace {

using genesis::agents::AuthorityDecision;
using genesis::agents::AuthorityGrant;
using genesis::agents::AuthorityRegistry;
using genesis::agents::CapabilityRequest;
using genesis::agents::DelegationError;

int g_checks = 0;

void require(bool condition, const char* message) {
    ++g_checks;
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_equal(AuthorityDecision actual, AuthorityDecision expected, const char* message) {
    ++g_checks;
    if (actual != expected) {
        std::cerr << "expected " << genesis::agents::to_string(expected) << " but got "
                  << genesis::agents::to_string(actual) << ": " << message << "\n";
        throw std::runtime_error(message);
    }
}

void require_equal(DelegationError actual, DelegationError expected, const char* message) {
    ++g_checks;
    if (actual != expected) {
        std::cerr << "expected " << genesis::agents::to_string(expected) << " but got "
                  << genesis::agents::to_string(actual) << ": " << message << "\n";
        throw std::runtime_error(message);
    }
}

AuthorityGrant make_grant(std::string subject,
                           std::string capability,
                           std::set<std::string, std::less<>> actions,
                           bool network = false,
                           bool filesystem_write = false,
                           std::uint64_t granted_at = 100U,
                           std::uint64_t expires_at = 0U) {
    AuthorityGrant grant;
    grant.subject_id = std::move(subject);
    grant.capability_id = std::move(capability);
    grant.allowed_actions = std::move(actions);
    grant.network_allowed = network;
    grant.filesystem_write_allowed = filesystem_write;
    grant.granted_at = granted_at;
    grant.expires_at = expires_at;
    return grant;
}

CapabilityRequest make_request(std::string subject, std::string capability, std::string action,
                                std::uint64_t at = 100U) {
    CapabilityRequest request;
    request.subject_id = std::move(subject);
    request.capability_id = std::move(capability);
    request.action = std::move(action);
    request.requested_at = at;
    return request;
}

// -- default deny ----------------------------------------------------------

void test_an_ungranted_capability_is_refused() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    require_equal(registry.evaluate(make_request("agent-a", "fs.read", "read")),
                  AuthorityDecision::capability_not_granted,
                  "an ungranted capability must be refused");
}

void test_an_empty_registry_refuses_everything() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    require(registry.size() == 0U, "a fresh registry must hold nothing");
    require_equal(registry.evaluate(make_request("agent-a", "fs.read", "read")),
                  AuthorityDecision::capability_not_granted,
                  "an empty registry must refuse");
}

// -- one capability, one set of actions ------------------------------------

void test_a_grant_covers_only_its_capability() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    require(registry.register_grant(make_grant("agent-a", "fs.read", {"read"})),
            "grant registration failed");
    require_equal(registry.evaluate(make_request("agent-a", "fs.read", "read")),
                  AuthorityDecision::granted,
                  "the granted action must be permitted");
    require_equal(registry.evaluate(make_request("agent-a", "fs.write", "read")),
                  AuthorityDecision::capability_not_granted,
                  "a different capability must not be covered by the grant");
}

void test_a_grant_covers_only_its_actions() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    require(registry.register_grant(make_grant("agent-a", "fs.read", {"read", "stat"})),
            "grant registration failed");
    require_equal(registry.evaluate(make_request("agent-a", "fs.read", "stat")),
                  AuthorityDecision::granted,
                  "a listed action must be permitted");
    require_equal(registry.evaluate(make_request("agent-a", "fs.read", "delete")),
                  AuthorityDecision::action_not_permitted,
                  "an unlisted action must be refused");
}

void test_a_grant_with_no_actions_permits_nothing() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    require(registry.register_grant(make_grant("agent-a", "fs.read", {})),
            "grant registration failed");
    require_equal(registry.evaluate(make_request("agent-a", "fs.read", "read")),
                  AuthorityDecision::action_not_permitted,
                  "an empty action set must permit nothing");
}

void test_a_grant_is_for_one_subject_only() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    require(registry.register_grant(make_grant("agent-a", "fs.read", {"read"})),
            "grant registration failed");
    require_equal(registry.evaluate(make_request("agent-b", "fs.read", "read")),
                  AuthorityDecision::capability_not_granted,
                  "another subject must not inherit the grant");
}

// -- expiry ----------------------------------------------------------------

void test_an_expired_grant_is_refused() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    require(registry.register_grant(make_grant("agent-a", "fs.read", {"read"}, false, false, 100U, 200U)),
            "grant registration failed");
    require_equal(registry.evaluate(make_request("agent-a", "fs.read", "read", 199U)),
                  AuthorityDecision::granted,
                  "a live grant must be permitted");
    require_equal(registry.evaluate(make_request("agent-a", "fs.read", "read", 200U)),
                  AuthorityDecision::expired,
                  "a grant must be refused at its expiry");
    require_equal(registry.evaluate(make_request("agent-a", "fs.read", "read", 500U)),
                  AuthorityDecision::expired,
                  "a grant must stay refused after its expiry");
}

void test_a_grant_without_an_expiry_never_ends() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    require(registry.register_grant(make_grant("agent-a", "fs.read", {"read"})),
            "grant registration failed");
    require_equal(registry.evaluate(make_request("agent-a", "fs.read", "read", 1ULL << 40U)),
                  AuthorityDecision::granted,
                  "a grant with no expiry must not expire");
}

// -- registration hygiene ---------------------------------------------------

void test_a_second_grant_for_the_same_capability_is_refused() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    require(registry.register_grant(make_grant("agent-a", "fs.read", {"read"})),
            "the first grant must be accepted");
    require(!registry.register_grant(make_grant("agent-a", "fs.read", {"read", "write"})),
            "a shadowing second grant must be refused");
    require_equal(registry.evaluate(make_request("agent-a", "fs.read", "write")),
                  AuthorityDecision::action_not_permitted,
                  "the refused grant must not have widened the scope");
}

void test_a_malformed_grant_is_refused() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    require(!registry.register_grant(make_grant("", "fs.read", {"read"})),
            "a grant with no subject must be refused");
    require(!registry.register_grant(make_grant("agent-a", "", {"read"})),
            "a grant with no capability must be refused");
    require(registry.size() == 0U, "a refused grant must not be retained");
}

void test_revoking_removes_the_grant() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    require(registry.register_grant(make_grant("agent-a", "fs.read", {"read"})),
            "grant registration failed");
    require(registry.revoke("agent-a", "fs.read"), "revocation must report success");
    require_equal(registry.evaluate(make_request("agent-a", "fs.read", "read")),
                  AuthorityDecision::capability_not_granted,
                  "a revoked grant must be refused");
    require(!registry.revoke("agent-a", "fs.read"), "a second revocation must report nothing removed");
}

void test_capabilities_of_lists_what_a_subject_holds() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    require(registry.register_grant(make_grant("agent-a", "fs.read", {"read"})),
            "grant registration failed");
    require(registry.register_grant(make_grant("agent-a", "net.fetch", {"fetch"})),
            "grant registration failed");
    const auto capabilities = registry.capabilities_of("agent-a");
    require(capabilities.size() == 2U, "the subject must hold two capabilities");
    require(capabilities[0] == "fs.read" && capabilities[1] == "net.fetch",
            "the capabilities must be listed in order");
    require(registry.capabilities_of("agent-b").empty(),
            "a subject with no grants must have no capabilities");
}

// -- delegation -------------------------------------------------------------

void test_a_delegation_may_narrow() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    const AuthorityGrant parent = make_grant("parent", "fs.read", {"read", "stat"}, true, true);
    AuthorityGrant child;
    const DelegationError error = registry.delegate(parent, "child", "fs.read", {"read"}, false, false,
                                                    0U, child);
    require_equal(error, DelegationError::none, "a narrowing delegation must be accepted");
    require(child.subject_id == "child", "the child subject must be recorded");
    require(child.allowed_actions.size() == 1U, "the child must hold only the delegated action");
    require(!child.network_allowed, "the child must not gain network access");
    require(!child.filesystem_write_allowed, "the child must not gain filesystem writes");
}

void test_a_delegation_cannot_widen_the_actions() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    const AuthorityGrant parent = make_grant("parent", "fs.read", {"read"});
    AuthorityGrant child;
    const DelegationError error = registry.delegate(parent, "child", "fs.read", {"read", "write"},
                                                    false, false, 0U, child);
    require_equal(error, DelegationError::action_widens_scope,
                  "delegating an action the parent lacks must be refused");
}

void test_a_delegation_cannot_widen_the_network() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    const AuthorityGrant parent = make_grant("parent", "net.fetch", {"fetch"}, false, false);
    AuthorityGrant child;
    const DelegationError error = registry.delegate(parent, "child", "net.fetch", {"fetch"}, true,
                                                    false, 0U, child);
    require_equal(error, DelegationError::network_widens_scope,
                  "delegating network access the parent lacks must be refused");
}

void test_a_delegation_cannot_widen_the_filesystem() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    const AuthorityGrant parent = make_grant("parent", "fs.read", {"read"}, false, false);
    AuthorityGrant child;
    const DelegationError error = registry.delegate(parent, "child", "fs.read", {"read"}, false, true,
                                                    0U, child);
    require_equal(error, DelegationError::filesystem_widens_scope,
                  "delegating filesystem writes the parent lacks must be refused");
}

void test_a_delegation_cannot_widen_the_lifetime() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    const AuthorityGrant parent = make_grant("parent", "fs.read", {"read"}, false, false, 100U, 200U);
    AuthorityGrant child;
    require_equal(registry.delegate(parent, "child", "fs.read", {"read"}, false, false, 300U, child),
                  DelegationError::lifetime_widens_scope,
                  "delegating a longer lifetime must be refused");
    require_equal(registry.delegate(parent, "child", "fs.read", {"read"}, false, false, 0U, child),
                  DelegationError::lifetime_widens_scope,
                  "delegating an unbounded lifetime from a bounded parent must be refused");
    require_equal(registry.delegate(parent, "child", "fs.read", {"read"}, false, false, 200U, child),
                  DelegationError::none,
                  "delegating the same lifetime must be accepted");
}

void test_a_delegation_may_shorten_the_lifetime() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    const AuthorityGrant parent = make_grant("parent", "fs.read", {"read"}, false, false, 100U, 1000U);
    AuthorityGrant child;
    require_equal(registry.delegate(parent, "child", "fs.read", {"read"}, false, false, 500U, child),
                  DelegationError::none,
                  "a shorter lifetime must be accepted");
    require(child.expires_at == 500U, "the child lifetime must be the delegated one");
}

void test_a_delegation_cannot_invent_a_capability() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    const AuthorityGrant parent = make_grant("parent", "fs.read", {"read"});
    AuthorityGrant child;
    require_equal(registry.delegate(parent, "child", "net.fetch", {"fetch"}, false, false, 0U, child),
                  DelegationError::parent_lacks_capability,
                  "delegating a capability the parent lacks must be refused");
}

void test_a_delegation_cannot_target_its_own_subject() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    const AuthorityGrant parent = make_grant("parent", "fs.read", {"read"});
    AuthorityGrant child;
    require_equal(registry.delegate(parent, "parent", "fs.read", {"read"}, false, false, 0U, child),
                  DelegationError::invalid_child,
                  "a subject delegating to itself must be refused");
}

void test_a_delegation_with_no_actions_is_refused() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    const AuthorityGrant parent = make_grant("parent", "fs.read", {"read"});
    AuthorityGrant child;
    require_equal(registry.delegate(parent, "child", "fs.read", {}, false, false, 0U, child),
                  DelegationError::action_widens_scope,
                  "a delegation granting nothing must be refused");
}

void test_a_delegated_grant_is_enforceable() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    const AuthorityGrant parent = make_grant("parent", "fs.read", {"read", "stat"});
    AuthorityGrant child;
    require_equal(registry.delegate(parent, "child", "fs.read", {"read"}, false, false, 0U, child),
                  DelegationError::none,
                  "the delegation must be accepted");
    require_equal(registry.evaluate(make_request("child", "fs.read", "read")),
                  AuthorityDecision::granted,
                  "the delegated action must be permitted for the child");
    require_equal(registry.evaluate(make_request("child", "fs.read", "stat")),
                  AuthorityDecision::action_not_permitted,
                  "an action that was not delegated must be refused for the child");
    require_equal(registry.evaluate(make_request("child", "net.fetch", "fetch")),
                  AuthorityDecision::capability_not_granted,
                  "the child must not hold a capability that was never delegated");
}

void test_a_delegated_grant_does_not_lift_the_parent() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    const AuthorityGrant parent = make_grant("parent", "fs.read", {"read"});
    AuthorityGrant child;
    require_equal(registry.delegate(parent, "child", "fs.read", {"read"}, false, false, 0U, child),
                  DelegationError::none,
                  "the delegation must be accepted");
    // The parent still holds only what it held. Delegation moves nothing.
    require_equal(registry.evaluate(make_request("parent", "fs.write", "write")),
                  AuthorityDecision::capability_not_granted,
                  "the parent must not gain anything by delegating");
}

void test_a_second_delegation_to_the_same_child_is_refused() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    const AuthorityGrant parent = make_grant("parent", "fs.read", {"read", "stat"});
    AuthorityGrant child;
    require_equal(registry.delegate(parent, "child", "fs.read", {"read"}, false, false, 0U, child),
                  DelegationError::none,
                  "the first delegation must be accepted");
    require_equal(registry.delegate(parent, "child", "fs.read", {"stat"}, false, false, 0U, child),
                  DelegationError::invalid_child,
                  "a shadowing second delegation must be refused");
    require_equal(registry.evaluate(make_request("child", "fs.read", "stat")),
                  AuthorityDecision::action_not_permitted,
                  "the refused delegation must not have widened the child's scope");
}

void test_a_delegation_chain_stays_narrow() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    const AuthorityGrant root = make_grant("root", "fs.read", {"read", "stat", "write"}, true, true);
    AuthorityGrant middle;
    require_equal(registry.delegate(root, "middle", "fs.read", {"read", "stat"}, true, false, 0U, middle),
                  DelegationError::none,
                  "the first narrowing must be accepted");
    AuthorityGrant leaf;
    require_equal(registry.delegate(middle, "leaf", "fs.read", {"read"}, false, false, 0U, leaf),
                  DelegationError::none,
                  "the second narrowing must be accepted");
    require_equal(registry.delegate(leaf, "root", "fs.read", {"read", "write"}, true, true, 0U, leaf),
                  DelegationError::action_widens_scope,
                  "the leaf must not be able to delegate back up a wider scope");
}

void test_a_malformed_request_is_refused() {
    AuthorityRegistry registry = AuthorityRegistry::with_default_deny();
    require_equal(registry.evaluate(make_request("", "fs.read", "read")),
                  AuthorityDecision::invalid_request,
                  "a request with no subject must be refused");
    require_equal(registry.evaluate(make_request("agent-a", "", "read")),
                  AuthorityDecision::invalid_request,
                  "a request with no capability must be refused");
    require_equal(registry.evaluate(make_request("agent-a", "fs.read", "")),
                  AuthorityDecision::invalid_request,
                  "a request with no action must be refused");
}

} // namespace

int main() {
    test_an_ungranted_capability_is_refused();
    test_an_empty_registry_refuses_everything();
    test_a_grant_covers_only_its_capability();
    test_a_grant_covers_only_its_actions();
    test_a_grant_with_no_actions_permits_nothing();
    test_a_grant_is_for_one_subject_only();
    test_an_expired_grant_is_refused();
    test_a_grant_without_an_expiry_never_ends();
    test_a_second_grant_for_the_same_capability_is_refused();
    test_a_malformed_grant_is_refused();
    test_revoking_removes_the_grant();
    test_capabilities_of_lists_what_a_subject_holds();
    test_a_delegation_may_narrow();
    test_a_delegation_cannot_widen_the_actions();
    test_a_delegation_cannot_widen_the_network();
    test_a_delegation_cannot_widen_the_filesystem();
    test_a_delegation_cannot_widen_the_lifetime();
    test_a_delegation_may_shorten_the_lifetime();
    test_a_delegation_cannot_invent_a_capability();
    test_a_delegation_cannot_target_its_own_subject();
    test_a_delegation_with_no_actions_is_refused();
    test_a_delegated_grant_is_enforceable();
    test_a_delegated_grant_does_not_lift_the_parent();
    test_a_second_delegation_to_the_same_child_is_refused();
    test_a_delegation_chain_stays_narrow();
    test_a_malformed_request_is_refused();

    std::cout << g_checks << " checks, 0 failures\n";
    return 0;
}
