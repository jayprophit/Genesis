#include "genesis/agents/tool_execution.hpp"

#include "test_support.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

using genesis::agents::AuthorityGrant;
using genesis::agents::AuthorityRegistry;
using genesis::agents::ToolBudget;
using genesis::agents::ToolCall;
using genesis::agents::ToolError;
using genesis::agents::ToolExecutor;

int& g_checks = genesis::testing::g_checks;

void require(bool condition, const char* message) {
    ++g_checks;
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_equal(ToolError actual, ToolError expected, const char* message) {
    ++g_checks;
    if (actual != expected) {
        std::cerr << "expected " << genesis::agents::to_string(expected) << " but got "
                  << genesis::agents::to_string(actual) << ": " << message << "\n";
        throw std::runtime_error(message);
    }
}

/// The grant an agent needs to run a tool.
AuthorityGrant grant_for(const std::string& agent, const std::string& capability,
                         std::set<std::string, std::less<>> actions,
                         bool network = false, bool filesystem = false,
                         std::uint64_t expires_at = 100'000) {
    AuthorityGrant grant;
    grant.subject_id = agent;
    grant.capability_id = capability;
    grant.allowed_actions = std::move(actions);
    grant.network_allowed = network;
    grant.filesystem_write_allowed = filesystem;
    grant.granted_at = 1'000;
    grant.expires_at = expires_at;
    return grant;
}

ToolCall call_for(const std::string& agent, const std::string& tool, const std::string& action,
                  bool approved = true) {
    ToolCall call;
    call.call_id = "call-1";
    call.agent_id = agent;
    call.tool_id = tool;
    call.action = action;
    call.started_at = 2'000;
    call.approved = approved;
    return call;
}

// ---------------------------------------------------------------------------
// A tool runs, and what it is handed.
// ---------------------------------------------------------------------------

void test_an_authorised_call_runs() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(authority.register_grant(grant_for("agent-a", "documents", {"read", "write"})),
            "the grant should be registered");
    require(executor.register_tool({"documents", "documents", "read",
                                    [](const genesis::agents::ToolSandbox&) {
                                        return "the document";
                                    }}),
            "the tool should be registered");

    const auto outcome = executor.execute(call_for("agent-a", "documents", "read"),
                                           ToolBudget{}, 0, 0,
                                           genesis::agents::ApprovalMode::never);
    require(outcome.executed, "the call should run");
    require_equal(outcome.error, ToolError::none, "the call should not be refused");
    require(outcome.result == "the document", "the result should come back");
}

void test_the_tool_is_handed_only_what_the_grant_permits() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    // The grant permits the network but not a filesystem write.
    require(authority.register_grant(grant_for("agent-a", "network", {"fetch"}, true, false)),
            "the grant should be registered");
    require(executor.register_tool({"network", "network", "fetch",
                                    [](const genesis::agents::ToolSandbox& sandbox) {
                                        return std::string(sandbox.network_allowed ? "net" : "no-net") +
                                               (sandbox.filesystem_write_allowed ? "+write" : "+nowrite");
                                    }}),
            "the tool should be registered");

    const auto outcome = executor.execute(call_for("agent-a", "network", "fetch"),
                                           ToolBudget{}, 0, 0,
                                           genesis::agents::ApprovalMode::never);
    require(outcome.executed, "the call should run");
    require(outcome.result == "net+nowrite",
            "the tool should see the network and not the filesystem");
}

void test_the_tool_is_handed_the_remaining_budget() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(authority.register_grant(grant_for("agent-a", "documents", {"read"})),
            "the grant should be registered");
    require(executor.register_tool({"documents", "documents", "read",
                                    [](const genesis::agents::ToolSandbox& sandbox) {
                                        return std::to_string(sandbox.remaining_steps) + "/" +
                                               std::to_string(sandbox.remaining_tool_calls);
                                    }}),
            "the tool should be registered");

    ToolBudget budget;
    budget.maximum_steps = 10;
    budget.maximum_tool_calls = 4;
    const auto outcome = executor.execute(call_for("agent-a", "documents", "read"), budget, 3, 1,
                                           genesis::agents::ApprovalMode::never);
    require(outcome.executed, "the call should run");
    require(outcome.result == "7/3", "the tool should see what is left of the budget");
}

// ---------------------------------------------------------------------------
// Authority is absolute.
// ---------------------------------------------------------------------------

void test_a_call_without_a_grant_is_refused() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(executor.register_tool({"documents", "documents", "read",
                                    [](const genesis::agents::ToolSandbox&) { return "x"; }}),
            "the tool should be registered");
    const auto outcome = executor.execute(call_for("agent-a", "documents", "read"),
                                           ToolBudget{}, 0, 0,
                                           genesis::agents::ApprovalMode::never);
    require(!outcome.executed, "a call without a grant must be refused");
    require_equal(outcome.error, ToolError::tool_not_granted, "the refusal must say why");
    require(!outcome.diagnostic.empty(), "the refusal must carry a reason");
}

void test_a_call_the_grant_does_not_permit_is_refused() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(authority.register_grant(grant_for("agent-a", "documents", {"read"})),
            "the grant should be registered");
    require(executor.register_tool({"documents", "documents", "read",
                                    [](const genesis::agents::ToolSandbox&) { return "x"; }}),
            "the tool should be registered");
    const auto outcome = executor.execute(call_for("agent-a", "documents", "write"),
                                           ToolBudget{}, 0, 0,
                                           genesis::agents::ApprovalMode::never);
    require(!outcome.executed, "an action the grant does not permit must be refused");
    require_equal(outcome.error, ToolError::action_not_permitted, "the refusal must say why");
}

void test_an_expired_grant_refuses_the_call() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    // The grant ends at 1,500 and the call is made at 2,000.
    require(authority.register_grant(grant_for("agent-a", "documents", {"read"}, false, false, 1'500)),
            "the grant should be registered");
    require(executor.register_tool({"documents", "documents", "read",
                                    [](const genesis::agents::ToolSandbox&) { return "x"; }}),
            "the tool should be registered");
    const auto outcome = executor.execute(call_for("agent-a", "documents", "read"),
                                           ToolBudget{}, 0, 0,
                                           genesis::agents::ApprovalMode::never);
    require(!outcome.executed, "an expired grant must refuse the call");
    require_equal(outcome.error, ToolError::tool_not_granted, "the refusal must say why");
}

void test_an_unknown_tool_is_refused() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    const auto outcome = executor.execute(call_for("agent-a", "ghost", "read"), ToolBudget{}, 0, 0,
                                           genesis::agents::ApprovalMode::never);
    require(!outcome.executed, "an unknown tool must be refused");
    require_equal(outcome.error, ToolError::unknown_tool, "the refusal must say why");
}

void test_an_unqualified_tool_is_refused() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(authority.register_grant(grant_for("agent-a", "documents", {"read"})),
            "the grant should be registered");
    require(executor.register_tool({"documents", "documents", "read",
                                    [](const genesis::agents::ToolSandbox&) { return "x"; },
                                    false}),
            "the tool should be registered");
    const auto outcome = executor.execute(call_for("agent-a", "documents", "read"),
                                           ToolBudget{}, 0, 0,
                                           genesis::agents::ApprovalMode::never);
    require(!outcome.executed, "an unqualified tool must be refused");
    require_equal(outcome.error, ToolError::unqualified_tool, "the refusal must say why");
}

// ---------------------------------------------------------------------------
// Approval is absolute.
// ---------------------------------------------------------------------------

void test_a_risky_call_without_approval_is_refused() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(authority.register_grant(grant_for("agent-a", "documents", {"write"})),
            "the grant should be registered");
    require(executor.register_tool({"documents", "documents", "write",
                                    [](const genesis::agents::ToolSandbox&) { return "x"; },
                                    true, true}),
            "the tool should be registered");
    const auto outcome = executor.execute(call_for("agent-a", "documents", "write", false),
                                           ToolBudget{}, 0, 0,
                                           genesis::agents::ApprovalMode::risky_actions);
    require(!outcome.executed, "a risky call without approval must be refused");
    require_equal(outcome.error, ToolError::approval_required, "the refusal must say why");
}

void test_a_risky_call_with_approval_runs() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(authority.register_grant(grant_for("agent-a", "documents", {"write"})),
            "the grant should be registered");
    require(executor.register_tool({"documents", "documents", "write",
                                    [](const genesis::agents::ToolSandbox&) { return "written"; },
                                    true, true}),
            "the tool should be registered");
    const auto outcome = executor.execute(call_for("agent-a", "documents", "write", true),
                                           ToolBudget{}, 0, 0,
                                           genesis::agents::ApprovalMode::risky_actions);
    require(outcome.executed, "an approved risky call should run");
}

void test_every_call_needs_approval_when_the_mode_says_so() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(authority.register_grant(grant_for("agent-a", "documents", {"read"})),
            "the grant should be registered");
    require(executor.register_tool({"documents", "documents", "read",
                                    [](const genesis::agents::ToolSandbox&) { return "x"; }}),
            "the tool should be registered");
    const auto outcome = executor.execute(call_for("agent-a", "documents", "read", false),
                                           ToolBudget{}, 0, 0,
                                           genesis::agents::ApprovalMode::always);
    require(!outcome.executed, "a call without approval must be refused");
    require_equal(outcome.error, ToolError::approval_required, "the refusal must say why");
}

// ---------------------------------------------------------------------------
// The budget is absolute.
// ---------------------------------------------------------------------------

void test_the_step_budget_is_enforced() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(authority.register_grant(grant_for("agent-a", "documents", {"read"})),
            "the grant should be registered");
    require(executor.register_tool({"documents", "documents", "read",
                                    [](const genesis::agents::ToolSandbox&) { return "x"; }}),
            "the tool should be registered");
    ToolBudget budget;
    budget.maximum_steps = 4;
    budget.maximum_tool_calls = 10;
    const auto outcome = executor.execute(call_for("agent-a", "documents", "read"), budget, 4, 0,
                                           genesis::agents::ApprovalMode::never);
    require(!outcome.executed, "a call with no steps left must be refused");
    require_equal(outcome.error, ToolError::step_budget_exhausted, "the refusal must say why");
}

void test_the_tool_call_budget_is_enforced() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(authority.register_grant(grant_for("agent-a", "documents", {"read"})),
            "the grant should be registered");
    require(executor.register_tool({"documents", "documents", "read",
                                    [](const genesis::agents::ToolSandbox&) { return "x"; }}),
            "the tool should be registered");
    ToolBudget budget;
    budget.maximum_steps = 10;
    budget.maximum_tool_calls = 2;
    const auto outcome = executor.execute(call_for("agent-a", "documents", "read"), budget, 0, 2,
                                           genesis::agents::ApprovalMode::never);
    require(!outcome.executed, "a call with no tool calls left must be refused");
    require_equal(outcome.error, ToolError::tool_call_budget_exhausted,
                  "the refusal must say why");
}

// ---------------------------------------------------------------------------
// The runtime limit is cooperative, and the breach is recorded.
// ---------------------------------------------------------------------------

void test_a_call_that_overruns_is_recorded_as_a_breach() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(authority.register_grant(grant_for("agent-a", "slow", {"wait"})),
            "the grant should be registered");
    require(executor.register_tool({"slow", "slow", "wait",
                                    [](const genesis::agents::ToolSandbox&) {
                                        std::this_thread::sleep_for(std::chrono::milliseconds(40));
                                        return "too late";
                                    },
                                    true, false, 5}),
            "the tool should be registered");
    const auto outcome = executor.execute(call_for("agent-a", "slow", "wait"), ToolBudget{}, 0, 0,
                                           genesis::agents::ApprovalMode::never);
    require(!outcome.executed, "an overrunning call is not a success");
    require(outcome.runtime_breached, "the breach should be recorded");
    require_equal(outcome.error, ToolError::runtime_exceeded, "the refusal must say why");
}

void test_a_tool_that_fails_is_recorded() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(authority.register_grant(grant_for("agent-a", "broken", {"go"})),
            "the grant should be registered");
    require(executor.register_tool({"broken", "broken", "go",
                                    [](const genesis::agents::ToolSandbox&) -> std::string {
                                        throw std::runtime_error("it broke");
                                    }}),
            "the tool should be registered");
    const auto outcome = executor.execute(call_for("agent-a", "broken", "go"), ToolBudget{}, 0, 0,
                                           genesis::agents::ApprovalMode::never);
    require(!outcome.executed, "a failed tool is not a success");
    require_equal(outcome.error, ToolError::tool_failed, "the refusal must say why");
    require(outcome.diagnostic.find("it broke") != std::string::npos,
            "the reason should be carried");
}

// ---------------------------------------------------------------------------
// The record.
// ---------------------------------------------------------------------------

void test_every_call_is_recorded() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(authority.register_grant(grant_for("agent-a", "documents", {"read"})),
            "the grant should be registered");
    require(executor.register_tool({"documents", "documents", "read",
                                    [](const genesis::agents::ToolSandbox&) { return "x"; }}),
            "the tool should be registered");
    require(executor.execute(call_for("agent-a", "documents", "read"), ToolBudget{}, 0, 0,
                              genesis::agents::ApprovalMode::never)
                .executed,
            "the first call should run");
    require(!executor.execute(call_for("agent-b", "documents", "read"), ToolBudget{}, 0, 0,
                               genesis::agents::ApprovalMode::never)
                 .executed,
            "the second call should be refused");
    require(executor.history().size() == 2, "both calls should be recorded");
    require(executor.history()[0].executed, "the first call ran");
    require(!executor.history()[1].executed, "the second call was refused");
    require(!executor.history()[1].diagnostic.empty(),
            "the refusal should carry a reason");
}

void test_a_duplicate_tool_is_refused() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    require(executor.register_tool({"documents", "documents", "read",
                                    [](const genesis::agents::ToolSandbox&) { return "x"; }}),
            "the first tool should be registered");
    require(!executor.register_tool({"documents", "documents", "read",
                                      [](const genesis::agents::ToolSandbox&) { return "y"; }}),
            "a duplicate tool must be refused");
    require(executor.size() == 1, "the executor must not have grown");
}

void test_a_malformed_call_is_refused() {
    AuthorityRegistry authority;
    ToolExecutor executor{authority};
    ToolCall call;
    const auto outcome = executor.execute(call, ToolBudget{}, 0, 0,
                                           genesis::agents::ApprovalMode::never);
    require(!outcome.executed, "a malformed call must be refused");
    require_equal(outcome.error, ToolError::invalid_request, "the refusal must say why");
}

int run_suite() {
    test_an_authorised_call_runs();
    test_the_tool_is_handed_only_what_the_grant_permits();
    test_the_tool_is_handed_the_remaining_budget();
    test_a_call_without_a_grant_is_refused();
    test_a_call_the_grant_does_not_permit_is_refused();
    test_an_expired_grant_refuses_the_call();
    test_an_unknown_tool_is_refused();
    test_an_unqualified_tool_is_refused();
    test_a_risky_call_without_approval_is_refused();
    test_a_risky_call_with_approval_runs();
    test_every_call_needs_approval_when_the_mode_says_so();
    test_the_step_budget_is_enforced();
    test_the_tool_call_budget_is_enforced();
    test_a_call_that_overruns_is_recorded_as_a_breach();
    test_a_tool_that_fails_is_recorded();
    test_every_call_is_recorded();
    test_a_duplicate_tool_is_refused();
    test_a_malformed_call_is_refused();
    return 0;
}

} // namespace

int main() {
    return genesis::testing::run(run_suite, "tool_execution_tests");
}
