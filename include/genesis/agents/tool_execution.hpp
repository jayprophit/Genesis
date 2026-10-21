#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "genesis/agents/authority.hpp"
#include "genesis/agents/platform.hpp"

namespace genesis::agents {

/**
 * The tool executor: the part that actually runs a tool.
 *
 * The gap analysis says what was missing, and it says it precisely:
 * "Tools are declared with grants and a sandbox requirement, and
 * workflows are described, but nothing executes a tool. The runtime
 * prepares a run and stops at the approval gate."
 *
 * So there was a gate and nothing behind it. A gate that nothing passes
 * is not a gate, it is a wall.
 *
 * What the executor enforces, and what it honestly does not:
 *
 *  1. AUTHORITY IS ABSOLUTE. The agent must hold a live grant for the
 *     tool, and the grant must permit the action. This is the authority
 *     engine's rule, applied through it, and nothing in the executor can
 *     widen it.
 *
 *  2. APPROVAL IS ABSOLUTE. Where the agent's approval mode requires
 *     it, a call without an approval is refused and recorded. The
 *     executor does not approve anything on the agent's behalf.
 *
 *  3. THE BUDGET IS ABSOLUTE. The step count and the tool-call count
 *     are counted by the executor and cannot be exceeded by a tool.
 *
 *  4. THE SANDBOX IS A DECLARATION. The tool is handed the permissions
 *     its grant permits, and nothing else: no network unless the grant
 *     says so, no filesystem write unless the grant says so, and the
 *     remaining budget. A tool is a registered operation supplied by the
 *     caller, not arbitrary code the executor loads, so the boundary is
 *     the declaration and the gate rather than an operating-system jail.
 *
 *  5. THE RUNTIME LIMIT IS COOPERATIVE. The executor tells the tool
 *     how long it may run and measures how long it did. A call that
 *     overran is recorded as a breach and is not counted as a success,
 *     but the executor cannot preempt a call that ignores it. Pretending
 *     otherwise would be a lie in the code.
 *
 * Every refusal is recorded with its reason, because a refusal that
 * cannot be explained cannot be audited.
 */

/// Why a tool call was refused.
enum class ToolError {
    /// The call was executed.
    none,
    /// The request was malformed.
    invalid_request,
    /// There is no such tool.
    unknown_tool,
    /// The tool is not qualified to run.
    unqualified_tool,
    /// The agent holds no grant for the tool.
    tool_not_granted,
    /// The agent's grant does not permit the action.
    action_not_permitted,
    /// The call needs a human approval it does not have.
    approval_required,
    /// The run has no steps left.
    step_budget_exhausted,
    /// The run has no tool calls left.
    tool_call_budget_exhausted,
    /// The call overran the time it was allowed.
    runtime_exceeded,
    /// The tool itself failed.
    tool_failed,
};

[[nodiscard]] std::string_view to_string(ToolError error) noexcept;

/// What a tool is permitted to do, as its grant permits it.
struct ToolSandbox final {
    /// The agent making the call.
    std::string agent_id;
    /// The tool being called.
    std::string tool_id;
    /// The action within the tool.
    std::string action;
    /// Whether the network may be used.
    bool network_allowed{false};
    /// Whether the filesystem may be written.
    bool filesystem_write_allowed{false};
    /// How many steps the run has left.
    std::size_t remaining_steps{0};
    /// How many tool calls the run has left.
    std::size_t remaining_tool_calls{0};
    /// How long the call may run, in milliseconds. Zero means no limit.
    std::uint64_t maximum_runtime_ms{0};
};

/// A request to run a tool.
struct ToolCall final {
    /// The call's own identifier.
    std::string call_id;
    /// The agent making the call.
    std::string agent_id;
    /// The tool to run.
    std::string tool_id;
    /// The action within the tool.
    std::string action;
    /// When the call was made, in the caller's clock.
    std::uint64_t started_at{0};
    /// Whether a human has approved this call.
    bool approved{false};
};

/// What happened to a tool call.
struct ToolCallOutcome final {
    /// The call's own identifier.
    std::string call_id;
    /// The tool that was called.
    std::string tool_id;
    /// The action that was called.
    std::string action;
    /// The agent that made the call.
    std::string agent_id;
    /// Whether the tool ran and succeeded.
    bool executed{false};
    /// Why it was refused, when it was.
    ToolError error{ToolError::none};
    /// What the tool returned.
    std::string result;
    /// The explanation, for the record.
    std::string diagnostic;
    /// How long the call took.
    std::uint64_t duration_ms{0};
    /// Whether the call overran the time it was allowed.
    bool runtime_breached{false};
};

/// How a tool run is bounded.
struct ToolBudget final {
    /// How many steps the run may take.
    std::size_t maximum_steps{32};
    /// How many tool calls the run may make.
    std::size_t maximum_tool_calls{16};
};

/// The executor.
class ToolExecutor final {
public:
    /// A tool is a registered operation the caller supplies.
    using Tool = std::function<std::string(const ToolSandbox&)>;

    /// What a tool is and whether it may run.
    struct ToolSpec final {
        /// The tool's own identifier.
        std::string tool_id;
        /// The capability the tool exercises. It is the grant's capability.
        std::string capability_id;
        /// The action the tool performs by default.
        std::string action;
        /// The operation itself.
        Tool tool;
        /// Whether the tool has been qualified to run.
        bool qualified{true};
        /// Whether the action is risky, and so needs review.
        bool risky{false};
        /// How long a call may run, in milliseconds. Zero means no limit.
        std::uint64_t maximum_runtime_ms{0};
    };

    /// An executor with an authority registry it does not own.
    explicit ToolExecutor(AuthorityRegistry& authority) noexcept : authority_(&authority) {}

    /// Register a tool. Returns false when it is malformed or already
    /// registered.
    [[nodiscard]] bool register_tool(ToolSpec spec);

    /// The tool, or null.
    [[nodiscard]] const ToolSpec* find_tool(std::string_view tool_id) const noexcept;

    /// How many tools are registered.
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * Run a tool.
     *
     * The call is authorised, approved and counted before the tool runs.
     * A refused call runs nothing and is still recorded.
     */
    [[nodiscard]] ToolCallOutcome execute(const ToolCall& call,
                                          const ToolBudget& budget,
                                          std::size_t steps_already_taken,
                                          std::size_t tool_calls_already_made,
                                          ApprovalMode approval_mode);

    /// Every call that was made, in order, refused or not.
    [[nodiscard]] const std::vector<ToolCallOutcome>& history() const noexcept;

private:
    /// The tools, by identifier.
    std::map<std::string, ToolSpec, std::less<>> tools_;
    /// Every call, in order.
    std::vector<ToolCallOutcome> history_;
    /// The authority registry, borrowed.
    AuthorityRegistry* authority_;
};

} // namespace genesis::agents
