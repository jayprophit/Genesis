#include "genesis/agents/planner.hpp"

#include "test_support.hpp"

#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using genesis::agents::Plan;
using genesis::agents::PlanError;
using genesis::agents::PlanStep;
using genesis::agents::Planner;

int& g_checks = genesis::testing::g_checks;

void require(bool condition, const char* message) {
    ++g_checks;
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_equal(PlanError actual, PlanError expected, const char* message) {
    ++g_checks;
    if (actual != expected) {
        std::cerr << "expected " << genesis::agents::to_string(expected) << " but got "
                  << genesis::agents::to_string(actual) << ": " << message << "\n";
        throw std::runtime_error(message);
    }
}

PlanStep step(const std::string& id, std::set<std::string, std::less<>> dependencies = {}) {
    PlanStep s;
    s.step_id = id;
    s.title = "do " + id;
    s.tool_id = "tool";
    s.action = "act";
    s.depends_on = std::move(dependencies);
    return s;
}

Plan make_plan(const std::vector<PlanStep>& steps) {
    Plan plan;
    plan.plan_id = "plan-1";
    plan.objective = "the objective";
    plan.steps = steps;
    return plan;
}

// ---------------------------------------------------------------------------
// A plan has to be executable.
// ---------------------------------------------------------------------------

void test_a_sound_plan_is_accepted() {
    PlanError error{PlanError::none};
    const auto plan = make_plan({step("a"), step("b", {"a"}), step("c", {"a", "b"})});
    require(Planner::validate(plan, &error), "the plan should be sound");
    require_equal(error, PlanError::none, "the plan should be accepted");
}

void test_an_empty_plan_is_refused() {
    PlanError error{PlanError::none};
    require(!Planner::validate(make_plan({}), &error), "an empty plan must be refused");
    require_equal(error, PlanError::empty_plan, "the refusal must say why");
}

void test_a_duplicate_step_is_refused() {
    PlanError error{PlanError::none};
    require(!Planner::validate(make_plan({step("a"), step("a")}), &error),
            "two steps with one identifier must be refused");
    require_equal(error, PlanError::duplicate_step, "the refusal must say why");
}

void test_an_unknown_dependency_is_refused() {
    PlanError error{PlanError::none};
    require(!Planner::validate(make_plan({step("a", {"ghost"})}), &error),
            "a dependency on a step that does not exist must be refused");
    require_equal(error, PlanError::unknown_dependency, "the refusal must say why");
}

void test_a_self_dependency_is_refused() {
    PlanError error{PlanError::none};
    require(!Planner::validate(make_plan({step("a", {"a"})}), &error),
            "a step that depends on itself must be refused");
    require_equal(error, PlanError::self_dependency, "the refusal must say why");
}

void test_a_dependency_cycle_is_refused() {
    PlanError error{PlanError::none};
    require(!Planner::validate(make_plan({step("a", {"b"}), step("b", {"a"})}), &error),
            "a cycle must be refused, because no step in it can ever run");
    require_equal(error, PlanError::dependency_cycle, "the refusal must say why");
}

void test_a_longer_cycle_is_refused() {
    PlanError error{PlanError::none};
    require(!Planner::validate(
                make_plan({step("a", {"c"}), step("b", {"a"}), step("c", {"b"})}), &error),
            "a cycle must be refused however long it is");
    require_equal(error, PlanError::dependency_cycle, "the refusal must say why");
}

// ---------------------------------------------------------------------------
// The order respects the dependencies.
// ---------------------------------------------------------------------------

void test_the_order_respects_the_dependencies() {
    const auto plan = make_plan({step("c", {"b"}), step("b", {"a"}), step("a")});
    const auto order = Planner::topological_order(plan);
    require(order.size() == 3, "the order should hold every step");
    require(order[0] == "a", "the first step should come first");
    require(order[1] == "b", "the second step should come second");
    require(order[2] == "c", "the third step should come third");
}

void test_the_order_is_stable() {
    const auto plan = make_plan({step("b"), step("a"), step("c")});
    const auto first = Planner::topological_order(plan);
    const auto second = Planner::topological_order(plan);
    require(first == second, "the same plan must always produce the same order");
    require(first[0] == "b", "the order should follow the proposal, not the alphabet");
}

void test_a_plan_with_a_cycle_has_no_order() {
    PlanError error{PlanError::none};
    const auto order = Planner::topological_order(make_plan({step("a", {"b"}), step("b", {"a"})}), &error);
    require(order.empty(), "a cyclic plan has no order");
    require_equal(error, PlanError::dependency_cycle, "the refusal must say why");
}

// ---------------------------------------------------------------------------
// The steps that may run now.
// ---------------------------------------------------------------------------

void test_the_ready_steps_are_the_ones_whose_dependencies_are_done() {
    const auto plan = make_plan({step("a"), step("b", {"a"}), step("c", {"b"})});
    const auto none = Planner::ready_steps(plan, {});
    require(none.size() == 1 && none.front() == "a", "only the first step is ready");

    const auto one = Planner::ready_steps(plan, {"a"});
    require(one.size() == 1 && one.front() == "b", "the second step becomes ready");

    const auto two = Planner::ready_steps(plan, {"a", "b"});
    require(two.size() == 1 && two.front() == "c", "the third step becomes ready");
}

void test_a_step_with_no_dependencies_is_ready() {
    const auto plan = make_plan({step("a"), step("b")});
    const auto ready = Planner::ready_steps(plan, {});
    require(ready.size() == 2, "both steps should be ready");
}

// ---------------------------------------------------------------------------
// Re-planning after a failure.
// ---------------------------------------------------------------------------

void test_the_failed_step_and_its_dependants_are_held() {
    const auto plan = make_plan({step("a"), step("b", {"a"}), step("c", {"b"}), step("d")});
    const auto held = Planner::held_steps(plan, "a");
    require(held.size() == 3, "the failed step and its two dependants are held");
    require(held[0] == "a", "the failed step is held");
    require(std::find(held.begin(), held.end(), "b") != held.end(),
            "the direct dependant is held");
    require(std::find(held.begin(), held.end(), "c") != held.end(),
            "the indirect dependant is held");
    require(std::find(held.begin(), held.end(), "d") == held.end(),
            "an unrelated step is not held");
}

void test_replanning_replaces_the_failed_part() {
    // The decomposer proposes two steps the first time, and a different
    // pair when it is told the first one failed.
    bool first = true;
    Planner planner([&first](const std::string&, const std::string& failure,
                             std::vector<PlanStep>& out) {
        if (first) {
            out = {step("a"), step("b", {"a"})};
            first = false;
        } else {
            // The reason reaches the decomposer.
            if (failure != "the tool timed out") {
                return false;
            }
            out = {step("a2")};
        }
        return true;
    });

    Plan plan;
    PlanError error{PlanError::none};
    require(planner.plan("the objective", plan, &error), "the plan should be made");
    require(plan.steps.size() == 2, "the plan should hold two steps");

    Plan replanned;
    require(planner.replan(plan, "a", "the tool timed out", replanned, &error),
            "the plan should be re-made");
    require(replanned.steps.size() == 1, "only the replacement should remain");
    require(replanned.steps.front().step_id == "a2", "the replacement should be the new step");
}

void test_replanning_keeps_the_steps_that_did_not_fail() {
    Planner planner([](const std::string&, const std::string& failure,
                       std::vector<PlanStep>& out) {
        if (failure.empty()) {
            out = {step("a"), step("b", {"a"}), step("c")};
        } else {
            out = {step("a2")};
        }
        return true;
    });

    Plan plan;
    require(planner.plan("the objective", plan), "the plan should be made");
    Plan replanned;
    require(planner.replan(plan, "a", "it failed", replanned), "the plan should be re-made");
    require(replanned.steps.size() == 2, "the unrelated step and the replacement remain");
    bool has_c = false;
    bool has_a2 = false;
    for (const auto& s : replanned.steps) {
        has_c = has_c || s.step_id == "c";
        has_a2 = has_a2 || s.step_id == "a2";
    }
    require(has_c, "the step that did not fail is kept");
    require(has_a2, "the replacement is in the plan");
}

void test_replanning_an_unknown_step_is_refused() {
    Planner planner([](const std::string&, const std::string&, std::vector<PlanStep>& out) {
        out = {step("a")};
        return true;
    });
    Plan plan;
    require(planner.plan("the objective", plan), "the plan should be made");
    Plan replanned;
    PlanError error{PlanError::none};
    require(!planner.replan(plan, "ghost", "it failed", replanned, &error),
            "an unknown step must be refused");
    require_equal(error, PlanError::unknown_step, "the refusal must say why");
}

// ---------------------------------------------------------------------------
// Decomposing an objective.
// ---------------------------------------------------------------------------

void test_a_planner_without_a_decomposer_makes_no_plan() {
    Planner planner;
    Plan plan;
    PlanError error{PlanError::none};
    require(!planner.plan("the objective", plan, &error), "there can be no plan");
    require_equal(error, PlanError::no_decomposition, "the refusal must say why");
}

void test_a_decomposer_that_proposes_nothing_makes_no_plan() {
    Planner planner([](const std::string&, const std::string&, std::vector<PlanStep>&) {
        return false;
    });
    Plan plan;
    PlanError error{PlanError::none};
    require(!planner.plan("the objective", plan, &error), "there can be no plan");
    require_equal(error, PlanError::no_decomposition, "the refusal must say why");
}

void test_a_plan_that_is_too_large_is_refused() {
    Planner planner([](const std::string&, const std::string&, std::vector<PlanStep>& out) {
        for (int i = 0; i < 10; ++i) {
            out.push_back(step("s" + std::to_string(i)));
        }
        return true;
    }, 4);
    Plan plan;
    PlanError error{PlanError::none};
    require(!planner.plan("the objective", plan, &error), "an oversized plan must be refused");
    require_equal(error, PlanError::too_many_steps, "the refusal must say why");
}

void test_a_plan_that_is_wrong_is_refused() {
    Planner planner([](const std::string&, const std::string&, std::vector<PlanStep>& out) {
        // A plan that depends on a step that does not exist.
        out = {step("a", {"ghost"})};
        return true;
    });
    Plan plan;
    PlanError error{PlanError::none};
    require(!planner.plan("the objective", plan, &error), "an unsound plan must be refused");
    require_equal(error, PlanError::unknown_dependency, "the refusal must say why");
}

void test_a_sound_decomposition_is_accepted() {
    Planner planner([](const std::string&, const std::string&, std::vector<PlanStep>& out) {
        out = {step("a"), step("b", {"a"})};
        return true;
    });
    Plan plan;
    PlanError error{PlanError::none};
    require(planner.plan("the objective", plan, &error), "the plan should be made");
    require(plan.steps.size() == 2, "the plan should hold both steps");
    require(plan.objective == "the objective", "the plan should keep the objective");
    require_equal(error, PlanError::none, "the plan should be accepted");
}

int run_suite() {
    test_a_sound_plan_is_accepted();
    test_an_empty_plan_is_refused();
    test_a_duplicate_step_is_refused();
    test_an_unknown_dependency_is_refused();
    test_a_self_dependency_is_refused();
    test_a_dependency_cycle_is_refused();
    test_a_longer_cycle_is_refused();
    test_the_order_respects_the_dependencies();
    test_the_order_is_stable();
    test_a_plan_with_a_cycle_has_no_order();
    test_the_ready_steps_are_the_ones_whose_dependencies_are_done();
    test_a_step_with_no_dependencies_is_ready();
    test_the_failed_step_and_its_dependants_are_held();
    test_replanning_replaces_the_failed_part();
    test_replanning_keeps_the_steps_that_did_not_fail();
    test_replanning_an_unknown_step_is_refused();
    test_a_planner_without_a_decomposer_makes_no_plan();
    test_a_decomposer_that_proposes_nothing_makes_no_plan();
    test_a_plan_that_is_too_large_is_refused();
    test_a_plan_that_is_wrong_is_refused();
    test_a_sound_decomposition_is_accepted();
    return 0;
}

} // namespace

int main() {
    return genesis::testing::run(run_suite, "planner_tests");
}
