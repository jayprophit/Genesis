#include "genesis/agents/planner.hpp"

#include <algorithm>
#include <map>
#include <utility>

namespace genesis::agents {

namespace {

/// The steps of a plan, keyed by identifier.
std::map<std::string, const PlanStep*, std::less<>> index_of(const Plan& plan) {
    std::map<std::string, const PlanStep*, std::less<>> out;
    for (const auto& step : plan.steps) {
        out.emplace(step.step_id, &step);
    }
    return out;
}

} // namespace

std::string_view to_string(PlanError error) noexcept {
    switch (error) {
        case PlanError::none:
            return "none";
        case PlanError::empty_plan:
            return "empty_plan";
        case PlanError::duplicate_step:
            return "duplicate_step";
        case PlanError::unknown_dependency:
            return "unknown_dependency";
        case PlanError::self_dependency:
            return "self_dependency";
        case PlanError::dependency_cycle:
            return "dependency_cycle";
        case PlanError::too_many_steps:
            return "too_many_steps";
        case PlanError::no_decomposition:
            return "no_decomposition";
        case PlanError::unknown_step:
            return "unknown_step";
    }
    return "none";
}

Planner::Planner(Decomposer decomposer, std::size_t maximum_steps)
    : decomposer_(std::move(decomposer)), maximum_steps_(maximum_steps == 0 ? 1 : maximum_steps) {}

std::size_t Planner::maximum_steps() const noexcept {
    return maximum_steps_;
}

bool Planner::validate(const Plan& plan, PlanError* error) {
    const auto fail = [error](PlanError code) {
        if (error) {
            *error = code;
        }
        return false;
    };

    if (plan.steps.empty()) {
        return fail(PlanError::empty_plan);
    }

    std::set<std::string, std::less<>> ids;
    for (const auto& step : plan.steps) {
        if (step.step_id.empty()) {
            return fail(PlanError::empty_plan);
        }
        if (!ids.insert(step.step_id).second) {
            // Two steps with one identifier make every dependency on it
            // ambiguous, and an ambiguous dependency is a plan that
            // cannot be run.
            return fail(PlanError::duplicate_step);
        }
    }

    for (const auto& step : plan.steps) {
        for (const auto& dependency : step.depends_on) {
            if (dependency == step.step_id) {
                return fail(PlanError::self_dependency);
            }
            if (ids.find(dependency) == ids.end()) {
                return fail(PlanError::unknown_dependency);
            }
        }
    }

    // A cycle means no step can ever become ready, so the plan would
    // wait for ever.
    std::map<std::string, std::size_t, std::less<>> remaining;
    for (const auto& step : plan.steps) {
        remaining.emplace(step.step_id, step.depends_on.size());
    }
    std::map<std::string, std::vector<std::string>, std::less<>> dependants;
    for (const auto& step : plan.steps) {
        for (const auto& dependency : step.depends_on) {
            dependants[dependency].push_back(step.step_id);
        }
    }
    std::vector<std::string> ready;
    for (const auto& [id, count] : remaining) {
        if (count == 0) {
            ready.push_back(id);
        }
    }
    std::size_t settled = 0;
    while (!ready.empty()) {
        const auto id = ready.back();
        ready.pop_back();
        ++settled;
        for (const auto& dependant : dependants[id]) {
            if (--remaining[dependant] == 0) {
                ready.push_back(dependant);
            }
        }
    }
    if (settled != plan.steps.size()) {
        return fail(PlanError::dependency_cycle);
    }

    if (error) {
        *error = PlanError::none;
    }
    return true;
}

std::vector<std::string> Planner::topological_order(const Plan& plan, PlanError* error) {
    if (!validate(plan, error)) {
        return {};
    }
    const auto index = index_of(plan);
    std::map<std::string, std::size_t, std::less<>> remaining;
    for (const auto& step : plan.steps) {
        remaining.emplace(step.step_id, step.depends_on.size());
    }
    std::map<std::string, std::vector<std::string>, std::less<>> dependants;
    for (const auto& step : plan.steps) {
        for (const auto& dependency : step.depends_on) {
            dependants[dependency].push_back(step.step_id);
        }
    }

    // Among the steps that are ready, the one proposed first comes first,
    // so the same plan always produces the same order.
    std::vector<std::string> order;
    std::map<std::string, std::size_t, std::less<>> proposed_at;
    for (std::size_t i = 0; i < plan.steps.size(); ++i) {
        proposed_at.emplace(plan.steps[i].step_id, i);
    }
    std::vector<std::string> ready;
    for (const auto& step : plan.steps) {
        if (step.depends_on.empty()) {
            ready.push_back(step.step_id);
        }
    }
    const auto by_proposal = [&proposed_at](const std::string& a, const std::string& b) {
        return proposed_at.at(a) < proposed_at.at(b);
    };
    std::sort(ready.begin(), ready.end(), by_proposal);

    while (!ready.empty()) {
        const auto id = ready.front();
        ready.erase(ready.begin());
        order.push_back(id);
        std::vector<std::string> unlocked;
        for (const auto& dependant : dependants[id]) {
            if (--remaining[dependant] == 0) {
                unlocked.push_back(dependant);
            }
        }
        std::sort(unlocked.begin(), unlocked.end(), by_proposal);
        ready.insert(ready.begin(), unlocked.begin(), unlocked.end());
    }
    return order;
}

std::vector<std::string> Planner::ready_steps(const Plan& plan,
                                             const std::set<std::string, std::less<>>& completed) {
    std::vector<std::string> ready;
    for (const auto& step : plan.steps) {
        if (completed.find(step.step_id) != completed.end()) {
            // A step that has finished is not to be run again.
            continue;
        }
        bool blocked = false;
        for (const auto& dependency : step.depends_on) {
            if (completed.find(dependency) == completed.end()) {
                blocked = true;
                break;
            }
        }
        if (!blocked) {
            ready.push_back(step.step_id);
        }
    }
    return ready;
}

std::vector<std::string> Planner::held_steps(const Plan& plan, const std::string& failed_step) {
    std::vector<std::string> held;
    const auto index = index_of(plan);
    if (index.find(failed_step) == index.end()) {
        return held;
    }
    // The failed step and everything that depends on it, directly or
    // through another step.
    std::map<std::string, std::vector<std::string>, std::less<>> dependants;
    for (const auto& step : plan.steps) {
        for (const auto& dependency : step.depends_on) {
            dependants[dependency].push_back(step.step_id);
        }
    }
    std::vector<std::string> frontier{failed_step};
    std::set<std::string, std::less<>> seen{failed_step};
    // The failed step is held as well: a step that failed is not to be
    // run again as it stands.
    held.push_back(failed_step);
    while (!frontier.empty()) {
        const auto id = frontier.back();
        frontier.pop_back();
        for (const auto& dependant : dependants[id]) {
            if (seen.insert(dependant).second) {
                held.push_back(dependant);
                frontier.push_back(dependant);
            }
        }
    }
    return held;
}

bool Planner::plan(const std::string& objective, Plan& out, PlanError* error) const {
    if (!decomposer_) {
        if (error) {
            *error = PlanError::no_decomposition;
        }
        return false;
    }
    std::vector<PlanStep> steps;
    if (!decomposer_(objective, {}, steps) || steps.empty()) {
        if (error) {
            *error = PlanError::no_decomposition;
        }
        return false;
    }
    if (steps.size() > maximum_steps_) {
        if (error) {
            *error = PlanError::too_many_steps;
        }
        return false;
    }
    Plan candidate;
    candidate.plan_id = objective;
    candidate.objective = objective;
    candidate.steps = std::move(steps);
    if (!validate(candidate, error)) {
        return false;
    }
    out = std::move(candidate);
    return true;
}

bool Planner::replan(const Plan& plan, const std::string& failed_step,
                      const std::string& reason, Plan& out, PlanError* error) const {
    const auto index = index_of(plan);
    if (index.find(failed_step) == index.end()) {
        if (error) {
            *error = PlanError::unknown_step;
        }
        return false;
    }

    // The steps that are held: the failed one and everything that
    // depended on it.
    const auto held = held_steps(plan, failed_step);
    const std::set<std::string, std::less<>> held_set(held.begin(), held.end());

    // The steps that survive: everything that was not held.
    Plan candidate;
    candidate.plan_id = plan.plan_id;
    candidate.objective = plan.objective;
    for (const auto& step : plan.steps) {
        if (held_set.find(step.step_id) == held_set.end()) {
            candidate.steps.push_back(step);
        }
    }

    // The failed part is decomposed again, with the reason, so the plan
    // can continue rather than restart.
    if (decomposer_) {
        std::vector<PlanStep> replacement;
        if (decomposer_(plan.objective, reason, replacement)) {
            for (auto& step : replacement) {
                // A replacement may not depend on a step that is being
                // held, because that step is not going to run.
                for (const auto& dependency : step.depends_on) {
                    if (held_set.find(dependency) != held_set.end()) {
                        step.depends_on.erase(dependency);
                    }
                }
                candidate.steps.push_back(std::move(step));
            }
        }
    }

    if (candidate.steps.size() > maximum_steps_) {
        if (error) {
            *error = PlanError::too_many_steps;
        }
        return false;
    }
    if (!validate(candidate, error)) {
        return false;
    }
    out = std::move(candidate);
    return true;
}

} // namespace genesis::agents
