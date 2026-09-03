#pragma once
#include "genesis/cognition/workspace.hpp"
#include "genesis/memory/graph.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace genesis::perception {
enum class Modality { text,image,audio,video,telemetry,proprioception,custom };
enum class RouteEvidenceState { unavailable,declared,observed,qualified };
struct QualificationPolicy { std::uint64_t minimum_successes{3};double maximum_failure_rate{.2}; };
struct AdapterRoute { std::string id,provider_id,artifact_digest,license_id,declaration_evidence_digest,latest_probe_evidence_digest,qualification_evidence_digest;Modality modality{Modality::custom};RouteEvidenceState state{RouteEvidenceState::unavailable};std::uint64_t probes{},successes{},failures{},last_probe_at{}; };
struct RawObservation { std::string id,route_id,payload_digest,provenance_digest;std::uint64_t captured_at{},received_at{};double confidence{},uncertainty{}; };
struct DerivedFeature { std::string id,observation_id,name,value_digest,derivation_digest;double confidence{},uncertainty{}; };
struct PerceptionProjection { std::optional<cognition::WorkspaceItem> workspace_candidate;std::optional<memory::MemoryNode> memory_candidate;bool belief_committed{false},memory_committed{false},action_authorized{false}; };
class PerceptionPipeline final {
public:
 PerceptionPipeline(std::string organism_id,std::size_t route_capacity,std::size_t observation_capacity,std::size_t feature_capacity,QualificationPolicy policy={});
 bool declare_route(AdapterRoute route,std::string* error=nullptr);
 bool record_probe(std::string_view route_id,bool success,std::string_view evidence_digest,std::uint64_t at,std::string* error=nullptr);
 bool qualify_route(std::string_view route_id,std::string_view evidence_digest,std::uint64_t at,std::string* error=nullptr);
 bool ingest(RawObservation observation,std::string* error=nullptr);
 bool derive(DerivedFeature feature,std::string* error=nullptr);
 [[nodiscard]] PerceptionProjection project(std::string_view observation_id,std::size_t maximum_features)const;
 [[nodiscard]] const AdapterRoute* find_route(std::string_view id)const noexcept;
 [[nodiscard]] const RawObservation* find_observation(std::string_view id)const noexcept;
 [[nodiscard]] bool verify()const;
 [[nodiscard]] const std::map<std::string,AdapterRoute>& routes()const noexcept{return routes_;}
 [[nodiscard]] const std::map<std::string,RawObservation>& observations()const noexcept{return observations_;}
 [[nodiscard]] const std::map<std::string,DerivedFeature>& features()const noexcept{return features_;}
private:std::string organism_id_;std::size_t route_capacity_{},observation_capacity_{},feature_capacity_{};QualificationPolicy policy_;std::map<std::string,AdapterRoute> routes_;std::map<std::string,RawObservation> observations_;std::map<std::string,DerivedFeature> features_;
};
}
