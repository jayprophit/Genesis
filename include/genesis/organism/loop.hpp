#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "genesis/cognition/capability.hpp"
#include "genesis/cognition/workspace.hpp"
#include "genesis/cognition/world_model.hpp"
#include "genesis/learning/consolidation.hpp"
#include "genesis/memory/graph.hpp"
#include "genesis/organism/systems.hpp"
#include "genesis/perception/pipeline.hpp"
#include "genesis/runtime/runtime.hpp"

namespace genesis::organism {

// One pass of the organism loop over EXISTING modules. Stages without an
// implementation are marked executed=false with a NOT_IMPLEMENTED note;
// the driver never fakes them.
enum class LoopStage {
    world_event,
    sense,
    perceive,
    interoception,
    attention_proxy,
    mental_state_na,
    recall,
    predict,
    drive_na,
    goal_na,
    plan_na,
    safety_gate,
    action_na,
    observe,
    prediction_error,
    learn,
    memory_update,
    homeostasis,
    allostasis_na,
    receipt,
};

struct LoopStageMark final {
    LoopStage stage{LoopStage::world_event};
    bool executed{false};
    std::string note;
};

struct LoopInput final {
    std::string topic;
    std::string payload;
    std::string capability_id;
    cognition::AuthorizationDecision authorization;
};

struct InteroceptiveSnapshot final {
    double memory_fill{0.0};
    HomeostasisDecision memory_pressure;
    HomeostasisDecision error_rate;
    std::uint64_t tick{0};
    [[nodiscard]] std::string digest() const;
};

struct LoopReceipt final {
    bool completed{false};
    bool safety_denied{false};
    std::string event_id;
    runtime::Sequence sequence{0};
    std::size_t recalled{0};
    std::size_t focused{0};
    bool prediction_issued{false};
    HomeostasisDecision memory_pressure;
    HomeostasisDecision error_rate;
    InteroceptiveSnapshot interoception;
    std::vector<LoopStageMark> stages;
    [[nodiscard]] std::string digest() const;
};

// Non-owning driver: the caller owns every module and the dispatcher.
class LoopDriver final {
public:
    LoopDriver(std::string organism_id,
               runtime::DeterministicDispatcher* dispatcher,
               perception::PerceptionPipeline* perception,
               cognition::ConsciousWorkspace* workspace,
               memory::MemoryGraph* memory,
               cognition::WorldDynamics* world,
               cognition::SelfCapabilityModel* capabilities,
               learning::ConsolidationScheduler* scheduler,
               HomeostasisController* homeostasis);

    LoopDriver(const LoopDriver&) = delete;
    LoopDriver& operator=(const LoopDriver&) = delete;

    // Declares + qualifies the loop sense route, configures homeostasis bands.
    [[nodiscard]] bool initialize(std::string* error = nullptr);
    [[nodiscard]] LoopReceipt step(const LoopInput& input, std::string* error);

private:
    std::string organism_id_;
    runtime::DeterministicDispatcher* dispatcher_{nullptr};
    perception::PerceptionPipeline* perception_{nullptr};
    cognition::ConsciousWorkspace* workspace_{nullptr};
    memory::MemoryGraph* memory_{nullptr};
    cognition::WorldDynamics* world_{nullptr};
    cognition::SelfCapabilityModel* capabilities_{nullptr};
    learning::ConsolidationScheduler* scheduler_{nullptr};
    HomeostasisController* homeostasis_{nullptr};
    std::uint64_t step_{0};
};

} // namespace genesis::organism
