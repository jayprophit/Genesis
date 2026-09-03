#pragma once
#include "genesis/cognition/workspace.hpp"
#include "genesis/organism/systems.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
namespace genesis::cognition {
class AffectStore;
enum class AffectSource { perception,memory,homeostasis,learning,social,operator_input };
enum class RegulationKind { reappraisal,calming,grounding,operator_support };
struct AffectSignal { std::string id,source_id,evidence_digest;AffectSource source{AffectSource::perception};double valence{},arousal{},stress{},confidence{};std::uint64_t observed_at{}; };
struct AffectState { double valence{},arousal{},stress{},regulation_load{};std::uint64_t updated_at{}; };
struct RegulationRecord { std::string id,evidence_digest;RegulationKind kind{RegulationKind::grounding};double intensity{};std::uint64_t applied_at{};AffectState before,after; };
struct AffectInfluence { WorkspaceItem item;double salience_multiplier{1};bool action_authorized{false}; };
class AffectRegulator final {
public:
 AffectRegulator(std::string organism_id,std::size_t signal_capacity,std::size_t regulation_capacity);
 bool ingest(AffectSignal signal,std::string* error=nullptr);
 bool ingest_homeostasis(std::string id,organism::Metric metric,organism::HomeostasisDecision decision,std::string evidence_digest,std::uint64_t at,std::string* error=nullptr);
 bool decay(double amount,std::uint64_t at,std::string* error=nullptr);
 bool regulate(std::string id,RegulationKind kind,double intensity,std::string evidence_digest,std::uint64_t at,std::string* error=nullptr);
 [[nodiscard]] AffectInfluence influence(const WorkspaceItem& item,double memory_valence)const;
 [[nodiscard]] bool verify()const;
 [[nodiscard]] const std::string& organism_id()const noexcept{return organism_id_;}
 [[nodiscard]] const AffectState& state()const noexcept{return state_;}
 [[nodiscard]] const std::vector<AffectSignal>& signals()const noexcept{return signals_;}
 [[nodiscard]] const std::vector<RegulationRecord>& regulations()const noexcept{return regulations_;}
private:friend class AffectStore;std::string organism_id_;std::size_t signal_capacity_{},regulation_capacity_{};AffectState state_;std::vector<AffectSignal> signals_;std::vector<RegulationRecord> regulations_;
};
}
