#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
namespace genesis::cognition {
enum class CapabilityEvidenceLevel { unavailable, declared, observed, qualified };
struct CompetencyPolicy { std::uint64_t minimum_trials{5};double minimum_success_rate{.8};std::uint64_t maximum_consecutive_failures{2}; };
struct CapabilityRecord { std::string id,route,declaration_evidence_digest,latest_evidence_digest,qualification_evidence_digest;CapabilityEvidenceLevel level{CapabilityEvidenceLevel::unavailable};std::uint64_t trials{},successes{},consecutive_failures{},updated_at{};double calibrated_competence{};bool revoked{}; };
struct AuthorizationDecision { bool identity_authenticated{},policy_allowed{},safety_allowed{};std::string decision_evidence_digest; };
class SelfCapabilityModel final {
public:
 SelfCapabilityModel(std::string organism_id,std::size_t capacity,CompetencyPolicy policy={});
 bool declare_capability(std::string id,std::string route,std::string evidence_digest,std::uint64_t at,std::string* error=nullptr);
 bool record_outcome(std::string_view id,bool success,std::string_view evidence_digest,std::uint64_t at,std::string* error=nullptr);
 bool qualify(std::string_view id,std::string_view qualification_evidence_digest,std::uint64_t at,std::string* error=nullptr);
 bool revoke(std::string_view id,std::string_view evidence_digest,std::uint64_t at,std::string* error=nullptr);
 [[nodiscard]] bool may_execute(std::string_view id,const AuthorizationDecision& authorization)const noexcept;
 [[nodiscard]] const CapabilityRecord* find(std::string_view id)const noexcept;
 [[nodiscard]] bool verify()const;
 [[nodiscard]] const std::string& organism_id()const noexcept{return organism_id_;}
 [[nodiscard]] std::size_t capacity()const noexcept{return capacity_;}
 [[nodiscard]] const CompetencyPolicy& policy()const noexcept{return policy_;}
 [[nodiscard]] const std::map<std::string,CapabilityRecord>& records()const noexcept{return records_;}
private: std::string organism_id_;std::size_t capacity_{};CompetencyPolicy policy_;std::map<std::string,CapabilityRecord> records_;
};
}
