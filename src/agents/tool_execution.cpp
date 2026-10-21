#include "genesis/agents/tool_execution.hpp"

#include "genesis/runtime/runtime.hpp"

#include <chrono>
#include <utility>

namespace genesis::agents {

namespace {

/// The sandbox the tool is handed. It is built from the grant and the
/// budget, and from nothing else.
ToolSandbox build_sandbox(const ToolExecutor::ToolSpec& tool, const ToolCall& call,
                           const AuthorityGrant& grant, const ToolBudget& budget,
                           std::size_t steps_taken, std::size_t tool_calls_made) {
    ToolSandbox sandbox;
    sandbox.agent_id = call.agent_id;
    sandbox.tool_id = tool.tool_id;
    sandbox.action = call.action;
    sandbox.network_allowed = grant.network_allowed;
    sandbox.filesystem_write_allowed = grant.filesystem_write_allowed;
    sandbox.remaining_steps = budget.maximum_steps > steps_taken
                                  ? budget.maximum_steps - steps_taken
                                  : 0;
    sandbox.remaining_tool_calls = budget.maximum_tool_calls > tool_calls_made
                                       ? budget.maximum_tool_calls - tool_calls_made
                                       : 0;
    sandbox.maximum_runtime_ms = tool.maximum_runtime_ms;
    return sandbox;
}

} // namespace

std::string_view to_string(ToolError error) noexcept {
    switch (error) {
        case ToolError::none:
            return "none";
        case ToolError::invalid_request:
            return "invalid_request";
        case ToolError::unknown_tool:
            return "unknown_tool";
        case ToolError::unqualified_tool:
            return "unqualified_tool";
        case ToolError::tool_not_granted:
            return "tool_not_granted";
        case ToolError::action_not_permitted:
            return "action_not_permitted";
        case ToolError::approval_required:
            return "approval_required";
        case ToolError::step_budget_exhausted:
            return "step_budget_exhausted";
        case ToolError::tool_call_budget_exhausted:
            return "tool_call_budget_exhausted";
        case ToolError::runtime_exceeded:
            return "runtime_exceeded";
        case ToolError::tool_failed:
            return "tool_failed";
    }
    return "none";
}

bool ToolExecutor::register_tool(ToolSpec spec) {
    if (spec.tool_id.empty() || !spec.tool) {
        return false;
    }
    if (tools_.find(spec.tool_id) != tools_.end()) {
        // Two tools with one identifier would make a grant ambiguous, and
        // an ambiguous grant is a grant that can be widened by accident.
        return false;
    }
    if (spec.capability_id.empty()) {
        spec.capability_id = spec.tool_id;
    }
    if (spec.action.empty()) {
        spec.action = spec.tool_id;
    }
    tools_.emplace(spec.tool_id, std::move(spec));
    return true;
}

const ToolExecutor::ToolSpec* ToolExecutor::find_tool(std::string_view tool_id) const noexcept {
    const auto found = tools_.find(tool_id);
    if (found == tools_.end()) {
        return nullptr;
    }
    return &found->second;
}

std::size_t ToolExecutor::size() const noexcept {
    return tools_.size();
}

ToolCallOutcome ToolExecutor::execute(const ToolCall& call, const ToolBudget& budget,
                                     std::size_t steps_already_taken,
                                     std::size_t tool_calls_already_made,
                                     ApprovalMode approval_mode) {
    ToolCallOutcome outcome;
    outcome.call_id = call.call_id;
    outcome.tool_id = call.tool_id;
    outcome.action = call.action;
    outcome.agent_id = call.agent_id;

    const auto refuse = [this, &outcome](ToolError error, std::string diagnostic) {
        outcome.executed = false;
        outcome.error = error;
        outcome.diagnostic = std::move(diagnostic);
        history_.push_back(outcome);
        return outcome;
    };

    if (call.call_id.empty() || call.agent_id.empty() || call.tool_id.empty() ||
        call.action.empty() || call.started_at == 0) {
        return refuse(ToolError::invalid_request,
                      "a call needs an identifier, an agent, a tool, an action and a time");
    }

    const auto* tool = find_tool(call.tool_id);
    if (tool == nullptr) {
        return refuse(ToolError::unknown_tool, "there is no such tool");
    }
    if (!tool->qualified) {
        return refuse(ToolError::unqualified_tool, "the tool is not qualified to run");
    }

    // The authority decides. The executor does not restate the rule; it
    // asks the authority engine and obeys the answer.
    const AuthorityGrant* grant = authority_->find_grant(call.agent_id, tool->capability_id);
    if (grant == nullptr) {
        return refuse(ToolError::tool_not_granted,
                      "the agent holds no grant for the tool");
    }
    if (!AuthorityRegistry::is_live(*grant, call.started_at)) {
        return refuse(ToolError::tool_not_granted, "the agent's grant has expired");
    }
    if (!AuthorityRegistry::permits_action(*grant, call.action)) {
        return refuse(ToolError::action_not_permitted,
                      "the agent's grant does not permit the action");
    }

    // Approval. The executor approves nothing.
    const bool needs_approval = approval_mode == ApprovalMode::always ||
                                 (approval_mode == ApprovalMode::risky_actions && tool->risky);
    if (needs_approval && !call.approved) {
        return refuse(ToolError::approval_required,
                      "the call needs a human approval and has not got one");
    }

    // The budget. It is counted here and cannot be exceeded by a tool.
    if (steps_already_taken >= budget.maximum_steps) {
        return refuse(ToolError::step_budget_exhausted, "the run has no steps left");
    }
    if (tool_calls_already_made >= budget.maximum_tool_calls) {
        return refuse(ToolError::tool_call_budget_exhausted,
                      "the run has no tool calls left");
    }

    const auto sandbox = build_sandbox(*tool, call, *grant, budget, steps_already_taken,
                                       tool_calls_already_made);

    const auto started = std::chrono::steady_clock::now();
    std::string result;
    try {
        result = tool->tool(sandbox);
    } catch (const std::exception& exception) {
        const auto finished = std::chrono::steady_clock::now();
        outcome.duration_ms = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count());
        return refuse(ToolError::tool_failed,
                      std::string("the tool failed: ") + exception.what());
    } catch (...) {
        const auto finished = std::chrono::steady_clock::now();
        outcome.duration_ms = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count());
        return refuse(ToolError::tool_failed, "the tool failed");
    }
    const auto finished = std::chrono::steady_clock::now();
    outcome.duration_ms = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count());

    if (sandbox.maximum_runtime_ms != 0 && outcome.duration_ms > sandbox.maximum_runtime_ms) {
        // The limit is cooperative: the tool was told, and the overrun is
        // recorded rather than hidden. The executor cannot preempt a call
        // that ignores it, and saying otherwise would be a lie.
        outcome.runtime_breached = true;
        outcome.executed = false;
        outcome.error = ToolError::runtime_exceeded;
        outcome.diagnostic = "the call overran the time it was allowed";
        history_.push_back(outcome);
        return outcome;
    }

    outcome.executed = true;
    outcome.error = ToolError::none;
    outcome.result = std::move(result);
    outcome.diagnostic = "the call ran";
    history_.push_back(outcome);
    return outcome;
}

const std::vector<ToolCallOutcome>& ToolExecutor::history() const noexcept {
    return history_;
}

} // namespace genesis::agents
