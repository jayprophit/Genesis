// P4: organism loop driver integration tests. The driver wires existing
// modules; missing stages must be marked NOT_IMPLEMENTED, never faked.
#include <iostream>
#include "test_support.hpp"
#include <string>

#include "genesis/cognition/capability.hpp"
#include "genesis/cognition/workspace.hpp"
#include "genesis/cognition/world_model.hpp"
#include "genesis/learning/consolidation.hpp"
#include "genesis/memory/graph.hpp"
#include "genesis/organism/loop.hpp"

#include "genesis/agents/planner.hpp"
#include "genesis/organism/systems.hpp"
#include "genesis/perception/pipeline.hpp"
#include "genesis/runtime/runtime.hpp"

// The counter is the harness's, so the summary line reports the
// checks that were really made.
int& g_checks = genesis::testing::g_checks;

namespace {

using namespace genesis;

int g_failures = 0;
void Check(bool condition, const char* what) {
    // The check is counted whether it passed or failed, so that the
    // summary line reports the checks that were really made.
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << what << "\n";
    }
}

struct Fixture {
    explicit Fixture(std::size_t memory_capacity = 64) : memory{"organism-test", memory_capacity, 128} {}
    runtime::DeterministicDispatcher dispatcher{1024, 1, 0};
    perception::PerceptionPipeline perception{"organism-test", 16, 64, 64};
    cognition::ConsciousWorkspace workspace{16};
    memory::MemoryGraph memory;
    cognition::WorldDynamics world{"organism-test", 64, 16, 16};
    cognition::SelfCapabilityModel capabilities{"organism-test", 16};
    learning::ConsolidationScheduler scheduler{64, 16};
    organism::HomeostasisController homeostasis;
    organism::LoopDriver driver{"organism-test", &dispatcher, &perception, &workspace,
                                &memory,     &world,       &capabilities, &scheduler,
                                &homeostasis};
};

bool HasMark(const organism::LoopReceipt& receipt, organism::LoopStage stage, bool executed) {
    for (const auto& mark : receipt.stages) {
        if (mark.stage == stage) return mark.executed == executed;
    }
    return false;
}

bool AllowCapability(Fixture& fx) {
    const std::string digest = runtime::sha256("loop-evidence");
    if (!fx.capabilities.declare_capability("loop.observe", "loop.local/text", digest, 1))
        return false;
    // Qualification policy: 5 successful trials before qualify.
    for (std::uint64_t at = 1; at <= 5; ++at) {
        if (!fx.capabilities.record_outcome("loop.observe", true, digest, at)) return false;
    }
    return fx.capabilities.qualify("loop.observe", digest, 6);
}

bool RegisterHypothesis(Fixture& fx) {
    cognition::CausalHypothesis hypothesis;
    hypothesis.id = "loop-hyp-1";
    hypothesis.cause_property = "loop.stimulus";
    hypothesis.cause_value_digest = runtime::sha256("cause");
    hypothesis.effect_property = "loop.outcome";
    hypothesis.effect_value_digest = runtime::sha256("effect");
    hypothesis.evidence_digest = runtime::sha256("hyp-evidence");
    hypothesis.created_at = 1;
    hypothesis.updated_at = 1;
    hypothesis.prior_confidence = 0.5;
    hypothesis.calibrated_confidence = 0.5;
    return fx.world.register_hypothesis(std::move(hypothesis));
}

organism::LoopInput AllowedInput() {
    organism::LoopInput input;
    input.topic = "organism.event";
    input.payload = "loop integration probe";
    input.capability_id = "loop.observe";
    input.authorization = cognition::AuthorizationDecision{true, true, true,
                                                           runtime::sha256("authz")};
    return input;
}

void TestAllowedPass() {
    Fixture fx;
    std::string error;
    Check(fx.driver.initialize(&error), "driver initialize");
    Check(AllowCapability(fx), "capability qualified");
    Check(RegisterHypothesis(fx), "hypothesis registered");

    const auto first = fx.driver.step(AllowedInput(), &error);
    if (!error.empty()) std::cerr << "step error: " << error << "\n";
    Check(error.empty(), "allowed step error");
    Check(first.completed && !first.safety_denied, "allowed step completes");
    Check(first.prediction_issued, "prediction issued with hypothesis");
    Check(first.recalled == 0, "empty memory recalls nothing");
    Check(fx.memory.find(first.event_id) != nullptr, "event node in memory");
    Check(first.memory_pressure.level != organism::PressureLevel::invalid,
          "homeostasis evaluated");
    Check(HasMark(first, organism::LoopStage::sense, true), "sense executed");
    Check(HasMark(first, organism::LoopStage::safety_gate, true), "gate executed");
    Check(HasMark(first, organism::LoopStage::mental_state, true), "mental state executed");
    Check(first.mental_state.admitted == first.focused, "admitted matches focus");
    Check(first.mental_state.uncertainty >= 0.0 && first.mental_state.uncertainty <= 1.0,
          "uncertainty bounded");
    Check(!first.mental_state.digest().empty(), "mental digest");
    Check(HasMark(first, organism::LoopStage::prediction_error, true),
          "prediction error resolved");
    const auto* hyp = fx.world.find_hypothesis("loop-hyp-1");
    Check(hyp != nullptr, "hypothesis still registered");
    // Observed receipt digest differs from expected "effect": refuted once.
    Check(hyp->support_count + hyp->counterevidence_count == 1, "error evidence recorded");
    Check(hyp->counterevidence_count == 1, "mismatch counted as counterevidence");
    Check(HasMark(first, organism::LoopStage::interoception, true),
          "interoception executed");
    Check(first.interoception.memory_fill == 0.0, "empty memory fill snapshot");
    Check(first.interoception.memory_pressure.level != organism::PressureLevel::invalid,
          "interoceptive pressure evaluated");
    Check(!first.interoception.digest().empty(), "interoception digest");
    Check(HasMark(first, organism::LoopStage::drive, true), "drive marked executed");
    Check(HasMark(first, organism::LoopStage::goal, true), "goal marked executed");
    Check(HasMark(first, organism::LoopStage::plan_na, false),
          "plan marked missing when there is no planner");
    Check(HasMark(first, organism::LoopStage::action_na, false), "action marked");
    Check(!first.digest().empty(), "receipt digest");

    const auto second = fx.driver.step(AllowedInput(), &error);
    Check(second.completed, "second step completes");
    Check(second.recalled >= 1, "second step recalls first event");
    Check(second.sequence == first.sequence + 2, "sequence advances 2 per pass");
}

void TestAllostasisArmsBeforeBreach() {
    Fixture fx(8); // small capacity: fill rises 1/8 per allowed step
    std::string error;
    Check(fx.driver.initialize(&error), "driver initialize (allostasis)");
    Check(AllowCapability(fx), "capability qualified (allostasis)");

    organism::LoopReceipt receipt;
    for (int i = 0; i < 6; ++i) {
        receipt = fx.driver.step(AllowedInput(), &error);
        Check(error.empty(), "allostasis step error");
        Check(receipt.completed, "allostasis step completes");
    }
    Check(HasMark(receipt, organism::LoopStage::allostasis, true), "allostasis executed");
    // Fill 6/8 = 0.75 (nominal) but projection 0.75 + 4/8 = 1.25 breaches:
    // pre-emptive action must arm BEFORE the reactive level turns critical.
    Check(receipt.allostasis_armed, "pre-emptive action armed");
    Check(receipt.preemptive_action != organism::CompensatoryAction::none,
          "pre-emptive action selected");
    Check(receipt.projected_fill > 0.8, "projection exceeds target band");
    Check(!fx.driver.step(AllowedInput(), &error).event_id.empty(), "loop continues");
}

void TestDriveSelection() {
    // Nominal + prediction issued -> consolidate.
    {
        Fixture fx;
        std::string error;
        Check(fx.driver.initialize(&error), "driver initialize (drive)");
        Check(AllowCapability(fx), "capability qualified (drive)");
        Check(RegisterHypothesis(fx), "hypothesis registered (drive)");
        const auto receipt = fx.driver.step(AllowedInput(), &error);
        Check(error.empty(), "drive step error");
        Check(HasMark(receipt, organism::LoopStage::drive, true), "drive executed");
        Check(receipt.drive.name == "consolidate", "nominal drive consolidates");
        Check(receipt.drive.urgency == 0.3, "consolidate urgency");
        Check(HasMark(receipt, organism::LoopStage::goal, true), "goal executed");
        Check(receipt.goal.description == "consolidate recent traces", "goal follows drive");
        Check(receipt.goal.priority == 0.3, "goal priority inherits urgency");
    }
    // High pressure -> conserve (pre-update fill 7/8 on capacity-8 memory).
    {
        Fixture fx(8);
        std::string error;
        Check(fx.driver.initialize(&error), "driver initialize (drive-pressure)");
        Check(AllowCapability(fx), "capability qualified (drive-pressure)");
        Check(RegisterHypothesis(fx), "hypothesis registered (drive-pressure)");
        organism::LoopReceipt receipt;
        for (int i = 0; i < 8; ++i) {
            receipt = fx.driver.step(AllowedInput(), &error);
            Check(error.empty(), "drive-pressure step error");
        }
        Check(receipt.drive.name == "conserve", "pressured drive conserves");
        Check(receipt.drive.urgency == 0.5, "conserve urgency");
        Check(receipt.goal.description == "reduce memory pressure", "pressured goal");
    }
    // No hypothesis -> no prediction -> resolve-uncertainty, even when denied
    // (drive proposes before the authority check disposes).
    {
        Fixture fx;
        std::string error;
        Check(fx.driver.initialize(&error), "driver initialize (drive-denied)");
        organism::LoopInput input;
        input.topic = "organism.event";
        input.payload = "denied probe";
        input.capability_id = "loop.unknown";
        input.authorization = cognition::AuthorizationDecision{false, false, false, ""};
        const auto receipt = fx.driver.step(input, &error);
        Check(receipt.safety_denied, "drive-denied receipt");
        Check(HasMark(receipt, organism::LoopStage::drive, true), "drive precedes gate");
        Check(receipt.drive.name == "resolve-uncertainty", "denied drive seeks evidence");
        Check(receipt.goal.description == "gather evidence", "denied goal");
    }
}

void TestDeniedStopsBeforeMutation() {
    Fixture fx;
    std::string error;
    Check(fx.driver.initialize(&error), "driver initialize (denied)");
    const std::size_t before = fx.memory.size();
    organism::LoopInput input;
    input.topic = "organism.event";
    input.payload = "denied probe";
    input.capability_id = "loop.unknown";
    input.authorization = cognition::AuthorizationDecision{false, false, false, ""};
    const auto receipt = fx.driver.step(input, &error);
    Check(receipt.safety_denied && !receipt.completed, "denied receipt");
    Check(fx.memory.size() == before, "denied step adds no memory");
    bool learn_present = false;
    for (const auto& mark : receipt.stages) {
        if (mark.stage == organism::LoopStage::learn) learn_present = true;
    }
    Check(!learn_present, "learn stage absent after denial");
    bool deny_event = false;
    for (const auto& outcome : fx.dispatcher.history_snapshot()) {
        if (outcome.event.topic() == "organism.loop.deny") deny_event = true;
    }
    Check(deny_event, "denial audit event published");
}

/// The loop asks its planner for a plan and records what came back.
void test_the_loop_plans_when_a_planner_is_given() {
    Fixture fx;
    // A planner that proposes two steps, the second depending on the
    // first, so that the plan carries a dependency rather than a flat
    // list.
    agents::Planner planner(
        [](const std::string& objective, const std::string&,
           std::vector<agents::PlanStep>& out) {
            // The objective reaches the decomposer, and it comes back on
            // the plan, so the caller can see what was planned for.
            if (objective.empty()) {
                return false;
            }
            agents::PlanStep first;
            first.step_id = "observe";
            first.title = "observe";
            first.tool_id = "tool";
            first.action = "read";
            agents::PlanStep second;
            second.step_id = "record";
            second.title = "record";
            second.tool_id = "tool";
            second.action = "write";
            second.depends_on = {"observe"};
            out = {first, second};
            return true;
        });
    // The loop borrows the planner, as it borrows every other module.
    organism::LoopDriver planned{"organism-test", &fx.dispatcher, &fx.perception,
                                 &fx.workspace, &fx.memory,     &fx.world,
                                 &fx.capabilities, &fx.scheduler, &fx.homeostasis,
                                 &planner};

    std::string error;
    Check(planned.initialize(&error), "the planned driver initialises");
    Check(AllowCapability(fx), "capability qualified");
    Check(RegisterHypothesis(fx), "hypothesis registered");

    const auto receipt = planned.step(AllowedInput(), &error);
    if (!error.empty()) std::cerr << "step error: " << error << "\n";
    Check(error.empty(), "the planned step has no error");
    Check(HasMark(receipt, organism::LoopStage::plan_na, true),
          "the plan stage is executed when a planner is given");
    Check(receipt.planned, "the receipt records that a plan was made");
    Check(receipt.planned_steps == 2, "the plan holds two steps");
    Check(receipt.plan.steps.size() == 2, "the receipt carries the plan");
    Check(receipt.plan.steps[1].depends_on.count("observe") == 1,
          "the plan carries the dependency between the steps");
    Check(!receipt.plan.objective.empty(), "the plan carries the objective");
}

/// The stage is still honestly missing when there is no planner.
void test_the_loop_without_a_planner_says_so() {
    Fixture fx;
    std::string error;
    Check(fx.driver.initialize(&error), "the driver without a planner initialises");
    Check(AllowCapability(fx), "capability qualified");
    Check(RegisterHypothesis(fx), "hypothesis registered");
    const auto receipt = fx.driver.step(AllowedInput(), &error);
    Check(HasMark(receipt, organism::LoopStage::plan_na, false),
          "the plan stage is missing without a planner");
    Check(!receipt.planned, "no plan is recorded without a planner");
}

} // namespace

/**
 * The suite, wrapped so that a failing check prints what failed and
 * returns a non-zero exit code. The Microsoft C++ runtime shows a modal
 * dialog on an unhandled exception, and a modal dialog waits for a key
 * press that never comes.
 */
static int run_suite() {
    TestAllowedPass();
    TestDriveSelection();
    TestAllostasisArmsBeforeBreach();
    TestDeniedStopsBeforeMutation();
    test_the_loop_plans_when_a_planner_is_given();
    test_the_loop_without_a_planner_says_so();
    if (g_failures == 0) {
        std::cout << "organism_loop_tests: all assertions passed\n";
    }
    std::cerr << "organism_loop_tests: " << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}

int main() {
    return genesis::testing::run(run_suite, "organism_loop_tests");
}
