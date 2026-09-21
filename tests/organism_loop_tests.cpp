// P4: organism loop driver integration tests. The driver wires existing
// modules; missing stages must be marked NOT_IMPLEMENTED, never faked.
#include <iostream>
#include <string>

#include "genesis/cognition/capability.hpp"
#include "genesis/cognition/workspace.hpp"
#include "genesis/cognition/world_model.hpp"
#include "genesis/learning/consolidation.hpp"
#include "genesis/memory/graph.hpp"
#include "genesis/organism/loop.hpp"
#include "genesis/organism/systems.hpp"
#include "genesis/perception/pipeline.hpp"
#include "genesis/runtime/runtime.hpp"

namespace {

using namespace genesis;

int g_failures = 0;
void Check(bool condition, const char* what) {
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
    Check(HasMark(first, organism::LoopStage::drive_na, false), "drive marked");
    Check(HasMark(first, organism::LoopStage::goal_na, false), "goal marked");
    Check(HasMark(first, organism::LoopStage::plan_na, false), "plan marked");
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

} // namespace

int main() {
    TestAllowedPass();
    TestAllostasisArmsBeforeBreach();
    TestDeniedStopsBeforeMutation();
    if (g_failures == 0) {
        std::cout << "organism_loop_tests: all assertions passed\n";
        return 0;
    }
    std::cerr << "organism_loop_tests: " << g_failures << " failures\n";
    return 1;
}
