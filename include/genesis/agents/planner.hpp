#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::agents {

/**
 * The planner: how an objective becomes a plan.
 *
 * The gap analysis says what was missing, and it says it in the
 * organism's own words: "Workflows are flat ordered steps with one tool
 * call each. There is no decomposition, no dependency graph, no
 * re-planning when a step fails, and the organism loop is missing its
 * plan stage entirely."
 *
 * Three things are wrong with a flat list of steps, and each has an
 * answer here:
 *
 *  1. A FLAT LIST HAS NO DEPENDENCIES. Steps that must wait for
 *     another step cannot say so. A plan here carries its dependencies,
 *     and the steps that can run now are computed from them rather than
 *     guessed.
 *
 *  2. A LIST CANNOT BE WRONG QUIETLY. A dependency on a step that
 *     does not exist, a dependency on itself, and a dependency cycle
 *     are all refused, because a plan that cannot be run is worse than
 *     no plan.
 *
 *  3. A FAILED PLAN STAYS FAILED. When a step fails, the steps that
 *     depended on it are held rather than attempted, and the objective
 *     is decomposed again with the reason, so the plan can continue
 *     rather than restart.
 *
 * The planner does not decide what the steps are. A caller supplies a
 * decomposer, which is where the model's judgement enters, and the
 * planner structures and checks what comes back. Genesis proposes; the
 * planner refuses a proposal it cannot execute.
 */

/// One step of a plan.
struct PlanStep final {
    /// The step's own identifier, unique within the plan.
    std::string step_id;
    /// What the step is for.
    std::string title;
    /// The tool the step uses.
    std::string tool_id;
    /// The action within that tool.
    std::string action;
    /// The steps that must finish before this one may run.
    std::set<std::string, std::less<>> depends_on;
    /// The steps this one was decomposed into, when it was.
    std::vector<std::string> subtasks;
    /// Whether a person must approve the step before it runs.
    bool requires_approval{false};
    /// What the step is expected to cost, in units the caller defines.
    std::size_t estimated_cost{1};
};

/// A plan: steps, and the dependencies between them.
struct Plan final {
    /// The plan's own identifier.
    std::string plan_id;
    /// What the plan is for.
    std::string objective;
    /// The steps, in the order they were proposed.
    std::vector<PlanStep> steps;
};

/// Why a plan was refused.
enum class PlanError {
    /// The plan is sound.
    none,
    /// The plan has no steps.
    empty_plan,
    /// Two steps share an identifier.
    duplicate_step,
    /// A step depends on a step that does not exist.
    unknown_dependency,
    /// A step depends on itself.
    self_dependency,
    /// The dependencies form a cycle, so no step can ever run.
    dependency_cycle,
    /// The plan has more steps than the planner allows.
    too_many_steps,
    /// The decomposer produced nothing.
    no_decomposition,
    /// The named step is not in the plan.
    unknown_step,
};

[[nodiscard]] std::string_view to_string(PlanError error) noexcept;

/// The planner.
class Planner final {
public:
    /**
     * How an objective is turned into steps.
     *
     * The decomposer is where the judgement lives: it is the model's
     * proposal for what the work is. It is given the objective, and on a
     * re-plan, the reason the last attempt failed. It returns false when
     * it has nothing to propose, and the planner then has no plan,
     * which is a different thing from a plan with a step that fails.
     */
    using Decomposer =
        std::function<bool(const std::string& objective, const std::string& failure,
                           std::vector<PlanStep>& out)>;

    /**
     * A planner with no decomposer. It can still check and order a plan
     * that a caller builds itself, which is what the organism loop does
     * when the caller supplies the steps directly.
     */
    Planner() = default;

    explicit Planner(Decomposer decomposer, std::size_t maximum_steps = 64);

    /**
     * Decompose an objective into a plan.
     *
     * The proposal is checked before it is returned: a plan that cannot
     * be executed is refused, and the reason is given.
     */
    [[nodiscard]] bool plan(const std::string& objective, Plan& out,
                             PlanError* error = nullptr) const;

    /**
     * The steps that may run now: those whose dependencies are all
     * finished. A step with no dependencies is ready.
     */
    [[nodiscard]] static std::vector<std::string> ready_steps(
        const Plan& plan, const std::set<std::string, std::less<>>& completed);

    /**
     * The steps that are held because a step they depend on failed: the
     * failed step itself and everything that depends on it, directly or
     * through other steps.
     */
    [[nodiscard]] static std::vector<std::string> held_steps(
        const Plan& plan, const std::string& failed_step);

    /**
     * Re-plan after a failure.
     *
     * The failed step and the steps that depended on it are held, and the
     * objective is decomposed again with the reason, so the plan can
     * continue from what is left rather than starting again.
     */
    [[nodiscard]] bool replan(const Plan& plan, const std::string& failed_step,
                               const std::string& reason, Plan& out,
                               PlanError* error = nullptr) const;

    /// Whether a plan is sound, and why it is not when it is not.
    [[nodiscard]] static bool validate(const Plan& plan, PlanError* error = nullptr);

    /**
     * The steps in an order that respects the dependencies.
     *
     * The order is stable: among the steps that are ready, the one
     * proposed first comes first, so the same plan always produces the
     * same order.
     */
    [[nodiscard]] static std::vector<std::string> topological_order(
        const Plan& plan, PlanError* error = nullptr);

    /// The most steps a plan may hold.
    [[nodiscard]] std::size_t maximum_steps() const noexcept;

private:
    Decomposer decomposer_;
    std::size_t maximum_steps_{64};
};

} // namespace genesis::agents
