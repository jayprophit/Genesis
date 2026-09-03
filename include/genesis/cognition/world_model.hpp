#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace genesis::cognition {
enum class PredictionState { pending, confirmed, refuted, expired };
struct WorldStateObservation { std::string id,entity_id,property,value_digest,evidence_digest;std::uint64_t observed_at{};double confidence{}; };
struct CausalHypothesis { std::string id,cause_property,cause_value_digest,effect_property,effect_value_digest,evidence_digest;std::uint64_t created_at{},updated_at{},support_count{},counterevidence_count{};double prior_confidence{},calibrated_confidence{}; };
struct WorldPrediction { std::string id,hypothesis_id,entity_id,expected_value_digest,outcome_evidence_digest;std::uint64_t issued_at{},due_at{},resolved_at{};double issued_confidence{};PredictionState state{PredictionState::pending}; };
struct CounterfactualResult { std::string hypothesis_id,effect_property,effect_value_digest;double confidence{};bool simulated{true}; };
class WorldDynamics final {
public:
 WorldDynamics(std::string organism_id,std::size_t state_capacity,std::size_t hypothesis_capacity,std::size_t prediction_capacity);
 bool observe_state(WorldStateObservation observation,std::string* error=nullptr);
 bool register_hypothesis(CausalHypothesis hypothesis,std::string* error=nullptr);
 bool issue_prediction(WorldPrediction prediction,std::string* error=nullptr);
 bool resolve_prediction(std::string_view prediction_id,std::string_view observed_value_digest,std::string_view evidence_digest,std::uint64_t observed_at,std::string* error=nullptr);
 std::size_t expire_predictions(std::uint64_t now);
 [[nodiscard]] std::vector<CounterfactualResult> simulate(std::string_view cause_property,std::string_view cause_value_digest,std::size_t maximum)const;
 [[nodiscard]] const WorldPrediction* find_prediction(std::string_view id)const noexcept;
 [[nodiscard]] const CausalHypothesis* find_hypothesis(std::string_view id)const noexcept;
 [[nodiscard]] bool verify()const;
 [[nodiscard]] const std::string& organism_id()const noexcept;
 [[nodiscard]] std::size_t state_count()const noexcept;
 [[nodiscard]] std::size_t hypothesis_count()const noexcept;
 [[nodiscard]] std::size_t prediction_count()const noexcept;
private:
 std::string organism_id_;std::size_t state_capacity_{},hypothesis_capacity_{},prediction_capacity_{};
 std::vector<WorldStateObservation> states_;std::map<std::string,CausalHypothesis> hypotheses_;std::map<std::string,WorldPrediction> predictions_;
};
}
