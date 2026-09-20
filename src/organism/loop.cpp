#include "genesis/organism/loop.hpp"

namespace genesis::organism {
namespace {

constexpr std::string_view kLoopRoute = "loop.local/text";

void Mark(std::vector<LoopStageMark>& stages, LoopStage stage, bool executed,
          std::string note = {}) {
    stages.push_back(LoopStageMark{stage, executed, std::move(note)});
}

void MarkMissing(std::vector<LoopStageMark>& stages, LoopStage stage, std::string what) {
    Mark(stages, stage, false, "NOT_IMPLEMENTED: " + std::move(what));
}

} // namespace

LoopDriver::LoopDriver(std::string organism_id,
                       runtime::DeterministicDispatcher* dispatcher,
                       perception::PerceptionPipeline* perception,
                       cognition::ConsciousWorkspace* workspace,
                       memory::MemoryGraph* memory,
                       cognition::WorldDynamics* world,
                       cognition::SelfCapabilityModel* capabilities,
                       learning::ConsolidationScheduler* scheduler,
                       HomeostasisController* homeostasis)
    : organism_id_(std::move(organism_id)),
      dispatcher_(dispatcher),
      perception_(perception),
      workspace_(workspace),
      memory_(memory),
      world_(world),
      capabilities_(capabilities),
      scheduler_(scheduler),
      homeostasis_(homeostasis) {}

bool LoopDriver::initialize(std::string* error) {
    auto fail = [&](const char* message) {
        if (error != nullptr) *error = message;
        return false;
    };
    if (dispatcher_ == nullptr || perception_ == nullptr || workspace_ == nullptr ||
        memory_ == nullptr || world_ == nullptr || capabilities_ == nullptr ||
        scheduler_ == nullptr || homeostasis_ == nullptr || organism_id_.empty()) {
        return fail("loop driver has no module bound");
    }
    perception::AdapterRoute route;
    route.id = std::string(kLoopRoute);
    route.provider_id = "genesis-loop";
    route.artifact_digest = runtime::sha256("loop-route");
    route.license_id = "first-party";
    route.declaration_evidence_digest = runtime::sha256("loop-declare");
    route.modality = perception::Modality::text;
    route.state = perception::RouteEvidenceState::declared;
    std::string step_error;
    if (!perception_->declare_route(route, &step_error)) return fail(step_error.c_str());
    for (int probe = 0; probe < 3; ++probe) {
        if (!perception_->record_probe(std::string(kLoopRoute), true,
                                       runtime::sha256("loop-probe"), 1,
                                       &step_error))
            return fail(step_error.c_str());
    }
    if (!perception_->qualify_route(std::string(kLoopRoute),
                                    runtime::sha256("loop-qualify"), 1, &step_error))
        return fail(step_error.c_str());
    if (!homeostasis_->configure(Metric::memory_pressure, {0.0, 0.2, 0.8, 1.0}))
        return fail("memory_pressure band rejected");
    if (!homeostasis_->configure(Metric::error_rate, {0.0, 0.0, 0.1, 1.0}))
        return fail("error_rate band rejected");
    return true;
}

std::string LoopReceipt::digest() const {
    std::string material{"genesis.loop.receipt.v1"};
    material += event_id + "|" + std::to_string(sequence) + "|";
    material += completed ? "1|" : "0|";
    material += safety_denied ? "1|" : "0|";
    material += std::to_string(recalled) + "|" + std::to_string(focused) + "|";
    material += prediction_issued ? "1|" : "0|";
    for (const auto& mark : stages) {
        material += std::to_string(static_cast<int>(mark.stage)) + (mark.executed ? "1" : "0");
    }
    return runtime::sha256(material);
}

LoopReceipt LoopDriver::step(const LoopInput& input, std::string* error) {
    LoopReceipt receipt;
    auto fail = [&](const char* message) {
        if (error != nullptr) *error = message;
        return receipt;
    };
    if (dispatcher_ == nullptr || input.topic.empty()) return fail("driver not initialized");
    const std::uint64_t tick = ++step_;
    const std::string event_id = "loop-evt-" + std::to_string(tick);

    // WORLD EVENT: deterministic envelope; payload stays outside the boundary.
    runtime::EventDraft draft;
    draft.event_id = event_id;
    draft.source_id = organism_id_;
    draft.topic = input.topic;
    draft.causal_parent_id = std::nullopt;
    draft.payload_digest = runtime::sha256(input.payload);
    const auto outcome = dispatcher_->publish(std::move(draft));
    receipt.event_id = event_id;
    receipt.sequence = outcome.event.sequence();
    Mark(receipt.stages, LoopStage::world_event, true);

    // SENSE: ingest through the qualified loop route.
    const auto now = dispatcher_->logical_time();
    perception::RawObservation observation;
    observation.id = "loop-obs-" + event_id;
    observation.route_id = std::string(kLoopRoute);
    observation.payload_digest = outcome.event.payload_digest();
    observation.provenance_digest = outcome.event.envelope_digest();
    observation.captured_at = now;
    observation.received_at = now;
    observation.confidence = 0.8;
    observation.uncertainty = 0.2;
    std::string step_error;
    if (!perception_->ingest(std::move(observation), &step_error)) return fail(step_error.c_str());
    Mark(receipt.stages, LoopStage::sense, true);

    // PERCEIVE: derive one feature, project workspace/memory candidates.
    perception::DerivedFeature feature;
    feature.id = "loop-feat-" + event_id;
    feature.observation_id = "loop-obs-" + event_id;
    feature.name = "topic";
    feature.value_digest = runtime::sha256(input.topic);
    feature.derivation_digest = runtime::sha256("derive:" + event_id);
    feature.confidence = 0.8;
    feature.uncertainty = 0.2;
    if (!perception_->derive(std::move(feature), &step_error)) return fail(step_error.c_str());
    const auto projection = perception_->project("loop-obs-" + event_id, 8);
    Mark(receipt.stages, LoopStage::perceive, true);
    MarkMissing(receipt.stages, LoopStage::interoception_na, "no unified self-state module");

    // ATTENTION (proxy): bounded workspace focus over the projected candidate.
    if (projection.workspace_candidate.has_value()) {
        if (!workspace_->submit(*projection.workspace_candidate)) return fail("workspace submit");
    }
    receipt.focused = workspace_->focus().size();
    Mark(receipt.stages, LoopStage::attention_proxy, true, "proxy: ConsciousWorkspace focus");
    MarkMissing(receipt.stages, LoopStage::mental_state_na, "no belief/affect integrator");

    // MEMORY RECALL.
    const auto recalled = memory_->activate({input.topic}, input.topic, 8);
    receipt.recalled = recalled.size();
    Mark(receipt.stages, LoopStage::recall, true);

    // PREDICTION over the first registered hypothesis, when one exists.
    if (world_->hypothesis_count() > 0) {
        const auto& hypotheses = world_->hypotheses();
        cognition::WorldPrediction prediction;
        prediction.id = "loop-pred-" + event_id;
        prediction.hypothesis_id = hypotheses.begin()->first;
        prediction.entity_id = organism_id_;
        prediction.expected_value_digest = hypotheses.begin()->second.effect_value_digest;
        prediction.issued_at = now;
        prediction.due_at = now + 100;
        prediction.issued_confidence = 0.5;
        if (!world_->issue_prediction(std::move(prediction), &step_error))
            return fail(step_error.c_str());
        receipt.prediction_issued = true;
        Mark(receipt.stages, LoopStage::predict, true);
    } else {
        Mark(receipt.stages, LoopStage::predict, false, "SKIPPED: no hypothesis registered");
    }
    MarkMissing(receipt.stages, LoopStage::drive_na, "no drive/motivation selector");
    MarkMissing(receipt.stages, LoopStage::goal_na, "no goal type");
    MarkMissing(receipt.stages, LoopStage::plan_na, "no action planner");

    // RIGHTS / SAFETY / AUTHORITY CHECK: fail-closed gate before any mutation.
    if (!capabilities_->may_execute(input.capability_id, input.authorization)) {
        receipt.safety_denied = true;
        Mark(receipt.stages, LoopStage::safety_gate, true, "denied");
        MarkMissing(receipt.stages, LoopStage::action_na, "no actuator; Ready never authorizes");
        runtime::EventDraft deny;
        deny.event_id = "loop-deny-" + event_id;
        deny.source_id = organism_id_;
        deny.topic = "organism.loop.deny";
        deny.payload_digest = runtime::sha256(receipt.digest());
        (void)dispatcher_->publish(std::move(deny));
        Mark(receipt.stages, LoopStage::receipt, true, "denial receipt");
        return receipt;
    }
    Mark(receipt.stages, LoopStage::safety_gate, true, "allowed");
    MarkMissing(receipt.stages, LoopStage::action_na, "no actuator; Ready never authorizes");

    // OBSERVE the loop outcome into the world model.
    cognition::WorldStateObservation world_observation;
    world_observation.id = "loop-wso-" + event_id;
    world_observation.entity_id = organism_id_;
    world_observation.property = "loop.receipt";
    world_observation.value_digest = runtime::sha256(receipt.digest());
    world_observation.evidence_digest = outcome.event.envelope_digest();
    world_observation.observed_at = now;
    world_observation.confidence = 0.8;
    if (!world_->observe_state(std::move(world_observation), &step_error))
        return fail(step_error.c_str());
    Mark(receipt.stages, LoopStage::observe, true);
    MarkMissing(receipt.stages, LoopStage::prediction_error_na,
                "no error broadcast; resolve_prediction is manual");

    // LEARN: register + touch a trace for this event.
    learning::LearningTrace trace;
    trace.id = "loop-trace-" + event_id;
    trace.kind = learning::TraceKind::episodic;
    trace.importance = 0.5;
    trace.uncertainty = 0.5;
    trace.accessibility = 0.5;
    trace.last_used = now;
    trace.estimated_cost = 1;
    if (!scheduler_->register_trace(trace, &step_error)) return fail(step_error.c_str());
    if (!scheduler_->record_use(trace.id, now)) return fail("trace use not recorded");
    Mark(receipt.stages, LoopStage::learn, true);

    // MEMORY UPDATE.
    memory::MemoryNode node;
    node.id = event_id;
    node.owner_id = organism_id_;
    node.content_digest = outcome.event.payload_digest();
    node.provenance_digest = outcome.event.envelope_digest();
    node.context = input.topic;
    node.features = {input.topic};
    if (!memory_->add(std::move(node), &step_error)) return fail(step_error.c_str());
    Mark(receipt.stages, LoopStage::memory_update, true);

    // HOMEOSTASIS: reactive evaluation over live pressures.
    const double pressure = memory_->node_capacity() == 0
        ? 0.0
        : static_cast<double>(memory_->size()) /
              static_cast<double>(memory_->node_capacity());
    receipt.memory_pressure = homeostasis_->evaluate(Metric::memory_pressure, pressure);
    receipt.error_rate = homeostasis_->evaluate(Metric::error_rate, 0.0);
    Mark(receipt.stages, LoopStage::homeostasis, true, "reactive evaluate only");
    MarkMissing(receipt.stages, LoopStage::allostasis_na, "no predictive regulation");

    // RECEIPT: audit event closing the pass.
    runtime::EventDraft receipt_event;
    receipt_event.event_id = "loop-receipt-" + event_id;
    receipt_event.source_id = organism_id_;
    receipt_event.topic = "organism.loop.receipt";
    receipt_event.payload_digest = runtime::sha256(receipt.digest());
    (void)dispatcher_->publish(std::move(receipt_event));
    Mark(receipt.stages, LoopStage::receipt, true);
    receipt.completed = true;
    return receipt;
}

} // namespace genesis::organism
