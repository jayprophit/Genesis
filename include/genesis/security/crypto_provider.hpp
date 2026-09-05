#pragma once

#include "genesis/identity/entity_registry.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::security {

class CryptoProviderStore;

// Serialized enums are append-only migration boundaries. Existing numeric
// values must never be reordered or repurposed.
enum class CryptoFunction : std::uint8_t {
    secure_hash,
    message_authentication,
    authenticated_encryption,
    digital_signature,
    key_establishment,
    key_derivation,
    random_generation,
    key_wrapping,
};

enum class QuantumReadiness : std::uint8_t {
    not_applicable,
    traditional,
    post_quantum,
    hybrid,
};

enum class AlgorithmDisposition : std::uint8_t {
    candidate,
    approved,
    verify_only,
    prohibited,
};

enum class CryptoThreatCategory : std::uint8_t {
    remote_attacker,
    local_untrusted_user,
    supply_chain_compromise,
    binary_tampering,
    rollback_attack,
    replay_attack,
    key_theft,
    key_loss,
    weak_randomness,
    side_channel,
    algorithm_break,
    quantum_cryptanalysis,
    misconfiguration,
    compromised_device,
    malicious_dependency,
    implementation_flaw,
};

enum class ProviderAssessmentKind : std::uint8_t {
    observation,
    qualification,
    suspension,
    revocation,
};

enum class ProviderState : std::uint8_t {
    declared,
    observed,
    qualified,
    suspended,
    revoked,
};

[[nodiscard]] std::string_view to_string(CryptoFunction value) noexcept;
[[nodiscard]] std::string_view to_string(QuantumReadiness value) noexcept;
[[nodiscard]] std::string_view to_string(AlgorithmDisposition value) noexcept;
[[nodiscard]] std::string_view to_string(CryptoThreatCategory value) noexcept;
[[nodiscard]] std::string_view to_string(ProviderAssessmentKind value) noexcept;
[[nodiscard]] std::string_view to_string(ProviderState value) noexcept;
[[nodiscard]] bool crypto_function_from_string(std::string_view text,
                                               CryptoFunction& value) noexcept;
[[nodiscard]] bool quantum_readiness_from_string(std::string_view text,
                                                 QuantumReadiness& value) noexcept;
[[nodiscard]] bool algorithm_disposition_from_string(
    std::string_view text,
    AlgorithmDisposition& value) noexcept;
[[nodiscard]] bool crypto_threat_category_from_string(
    std::string_view text,
    CryptoThreatCategory& value) noexcept;
[[nodiscard]] bool provider_assessment_kind_from_string(
    std::string_view text,
    ProviderAssessmentKind& value) noexcept;
[[nodiscard]] bool provider_state_from_string(std::string_view text,
                                              ProviderState& value) noexcept;
[[nodiscard]] bool crypto_function_requires_key_material(
    CryptoFunction function) noexcept;

struct CryptoThreatModelDraft final {
    std::string id;
    std::vector<CryptoThreatCategory> threats;
    std::string asset_inventory_digest;
    std::string trust_boundaries_digest;
    std::string mitigation_plan_digest;
    std::string residual_risk_digest;
    std::string evidence_digest;
    std::uint64_t effective_from{};
    std::uint64_t review_at{};
    std::uint64_t recorded_at{};
};

struct CryptoThreatModelRevision final {
    std::string id;
    std::vector<CryptoThreatCategory> threats;
    std::string asset_inventory_digest;
    std::string trust_boundaries_digest;
    std::string mitigation_plan_digest;
    std::string residual_risk_digest;
    std::string evidence_digest;
    std::uint64_t revision{};
    std::uint64_t effective_from{};
    std::uint64_t review_at{};
    std::uint64_t recorded_at{};
    std::string previous_revision_digest;
    std::string revision_digest;

    [[nodiscard]] bool operator==(const CryptoThreatModelRevision&) const = default;
};

struct CryptoAlgorithmRule final {
    std::string algorithm_id;
    CryptoFunction function{CryptoFunction::secure_hash};
    QuantumReadiness quantum_readiness{QuantumReadiness::not_applicable};
    AlgorithmDisposition disposition{AlgorithmDisposition::candidate};
    std::uint16_t security_strength_bits{};
    std::string standard_reference;
    std::string standard_evidence_digest;
    std::optional<std::uint64_t> deprecate_at;
    std::optional<std::uint64_t> prohibit_at;

    [[nodiscard]] bool operator==(const CryptoAlgorithmRule&) const = default;
};

struct CryptoPolicyDraft final {
    std::string id;
    std::string threat_model_id;
    std::uint64_t threat_model_revision{};
    std::string source_set_digest;
    std::string change_evidence_digest;
    std::vector<CryptoAlgorithmRule> rules;
    std::uint64_t effective_from{};
    std::uint64_t review_at{};
    std::uint64_t recorded_at{};
};

struct CryptoPolicyRevision final {
    std::string id;
    std::string threat_model_id;
    std::uint64_t threat_model_revision{};
    std::string threat_model_digest;
    std::string source_set_digest;
    std::string change_evidence_digest;
    std::vector<CryptoAlgorithmRule> rules;
    std::uint64_t revision{};
    std::uint64_t effective_from{};
    std::uint64_t review_at{};
    std::uint64_t recorded_at{};
    std::string previous_revision_digest;
    std::string revision_digest;

    [[nodiscard]] bool operator==(const CryptoPolicyRevision&) const = default;
};

struct CryptoAlgorithmCapability final {
    std::string algorithm_id;
    CryptoFunction function{CryptoFunction::secure_hash};
    std::string implementation_route;
    std::string implementation_digest;

    [[nodiscard]] bool operator==(const CryptoAlgorithmCapability&) const = default;
};

struct CryptoProviderManifest final {
    std::string provider_id;
    std::string implementation_name;
    std::string implementation_version;
    std::string platform_id;
    std::string module_boundary_digest;
    std::string module_binary_digest;
    std::string source_provenance_digest;
    std::string license_evidence_digest;
    std::string build_evidence_digest;
    std::vector<CryptoAlgorithmCapability> capabilities;
    std::uint64_t declared_at{};

    [[nodiscard]] bool operator==(const CryptoProviderManifest&) const = default;
};

struct CryptoRouteEvidence final {
    std::string algorithm_id;
    CryptoFunction function{CryptoFunction::secure_hash};
    std::string implementation_route;
    std::string implementation_digest;
    std::string functional_test_digest;
    std::string platform_evidence_digest;
    std::string algorithm_validation_reference;
    std::string algorithm_validation_evidence_digest;
    std::string key_custody_evidence_digest;

    [[nodiscard]] bool operator==(const CryptoRouteEvidence&) const = default;
};

struct CryptoProviderAssessmentDraft final {
    std::string id;
    std::string provider_id;
    ProviderAssessmentKind kind{ProviderAssessmentKind::observation};
    std::string policy_id;
    std::uint64_t policy_revision{};
    std::string evaluator_entity_id;
    std::string evaluation_evidence_digest;
    std::string module_validation_reference;
    std::string module_validation_evidence_digest;
    std::vector<CryptoRouteEvidence> routes;
    std::uint64_t observed_at{};
    std::uint64_t recorded_at{};
    std::optional<std::uint64_t> valid_until;
};

struct CryptoProviderAssessment final {
    std::string id;
    std::string provider_id;
    ProviderAssessmentKind kind{ProviderAssessmentKind::observation};
    std::string policy_id;
    std::uint64_t policy_revision{};
    std::string evaluator_entity_id;
    std::string evaluation_evidence_digest;
    std::string module_validation_reference;
    std::string module_validation_evidence_digest;
    std::vector<CryptoRouteEvidence> routes;
    std::uint64_t sequence{};
    std::uint64_t observed_at{};
    std::uint64_t recorded_at{};
    std::optional<std::uint64_t> valid_until;
    std::string previous_assessment_digest;
    std::string assessment_digest;

    [[nodiscard]] bool operator==(const CryptoProviderAssessment&) const = default;
};

struct CryptoProviderDecision final {
    ProviderState state{ProviderState::declared};
    bool provider_found{false};
    bool capability_declared{false};
    bool provider_observed{false};
    bool provider_qualified{false};
    bool policy_current{false};
    bool threat_model_current{false};
    bool algorithm_policy_approved{false};
    bool validation_evidence_recorded{false};
    bool key_custody_required{false};
    bool key_custody_evidence_recorded{false};
    bool integration_candidate{false};
    bool cryptographic_operation_available{false};
    bool identity_authenticated{false};
    bool provenance_authenticated{false};
    bool action_authorized{false};
    std::string reason;
};

struct CryptoEntityAudit final {
    std::size_t registry_errors{};
    std::size_t owner_mismatches{};
    std::size_t missing_evaluators{};
    std::size_t incompatible_evaluators{};
    std::size_t temporal_mismatches{};

    [[nodiscard]] bool clean() const noexcept;
};

enum class CryptoRegistryErrorCode : std::uint8_t {
    none,
    invalid_entry,
    capacity_exceeded,
    duplicate_entry,
    missing_threat_model,
    missing_policy,
    missing_provider,
    missing_evaluator,
    incompatible_evaluator,
    owner_binding_mismatch,
    temporal_conflict,
    revision_conflict,
    transition_conflict,
    policy_conflict,
    route_conflict,
};

struct CryptoRegistryError final {
    CryptoRegistryErrorCode code{CryptoRegistryErrorCode::none};
    std::string message;
};

[[nodiscard]] std::string derive_crypto_registry_id(
    std::string_view entity_namespace_id,
    std::string_view owner_entity_id);
[[nodiscard]] std::string derive_crypto_provider_id(
    std::string_view implementation_name,
    std::string_view implementation_version,
    std::string_view platform_id,
    std::string_view module_binary_digest);

class CryptoProviderRegistry final {
public:
    CryptoProviderRegistry(std::string registry_id,
                           std::string entity_namespace_id,
                           std::string owner_entity_id,
                           std::size_t threat_revision_capacity,
                           std::size_t policy_revision_capacity,
                           std::size_t provider_capacity,
                           std::size_t assessment_capacity);

    [[nodiscard]] bool append_threat_model(
        CryptoThreatModelDraft draft,
        const identity::EntityRegistry& entities,
        CryptoRegistryError* error = nullptr);
    [[nodiscard]] bool append_policy(CryptoPolicyDraft draft,
                                     const identity::EntityRegistry& entities,
                                     CryptoRegistryError* error = nullptr);
    [[nodiscard]] bool register_provider(
        CryptoProviderManifest manifest,
        const identity::EntityRegistry& entities,
        CryptoRegistryError* error = nullptr);
    [[nodiscard]] bool record_assessment(
        CryptoProviderAssessmentDraft draft,
        const identity::EntityRegistry& entities,
        CryptoRegistryError* error = nullptr);

    [[nodiscard]] const CryptoThreatModelRevision* threat_model(
        std::string_view id,
        std::uint64_t revision) const;
    [[nodiscard]] const CryptoPolicyRevision* policy(std::string_view id,
                                                     std::uint64_t revision) const;
    [[nodiscard]] const CryptoProviderManifest* provider(
        std::string_view provider_id) const;
    [[nodiscard]] const std::vector<CryptoProviderAssessment>* assessment_history(
        std::string_view provider_id) const;
    [[nodiscard]] ProviderState provider_state(std::string_view provider_id) const;
    [[nodiscard]] CryptoProviderDecision evaluate(
        std::string_view provider_id,
        std::string_view algorithm_id,
        CryptoFunction function,
        std::uint64_t at) const;
    [[nodiscard]] CryptoEntityAudit audit_entities(
        const identity::EntityRegistry& entities) const;
    [[nodiscard]] bool verify() const;

    [[nodiscard]] const std::string& registry_id() const noexcept;
    [[nodiscard]] const std::string& entity_namespace_id() const noexcept;
    [[nodiscard]] const std::string& owner_entity_id() const noexcept;
    [[nodiscard]] std::size_t threat_revision_capacity() const noexcept;
    [[nodiscard]] std::size_t policy_revision_capacity() const noexcept;
    [[nodiscard]] std::size_t provider_capacity() const noexcept;
    [[nodiscard]] std::size_t assessment_capacity() const noexcept;
    [[nodiscard]] std::size_t threat_revision_count() const noexcept;
    [[nodiscard]] std::size_t policy_revision_count() const noexcept;
    [[nodiscard]] std::size_t assessment_count() const noexcept;
    [[nodiscard]] const std::map<std::string,
                                 std::vector<CryptoThreatModelRevision>,
                                 std::less<>>&
    threat_models() const noexcept;
    [[nodiscard]] const std::map<std::string,
                                 std::vector<CryptoPolicyRevision>,
                                 std::less<>>&
    policies() const noexcept;
    [[nodiscard]] const std::map<std::string,
                                 CryptoProviderManifest,
                                 std::less<>>&
    providers() const noexcept;
    [[nodiscard]] const std::map<std::string,
                                 std::vector<CryptoProviderAssessment>,
                                 std::less<>>&
    assessments() const noexcept;

private:
    friend class CryptoProviderStore;

    [[nodiscard]] bool rebuild_indexes();

    std::string registry_id_;
    std::string entity_namespace_id_;
    std::string owner_entity_id_;
    std::size_t threat_revision_capacity_{};
    std::size_t policy_revision_capacity_{};
    std::size_t provider_capacity_{};
    std::size_t assessment_capacity_{};
    std::size_t threat_revision_count_{};
    std::size_t policy_revision_count_{};
    std::size_t assessment_count_{};
    std::map<std::string, std::vector<CryptoThreatModelRevision>, std::less<>>
        threat_models_;
    std::map<std::string, std::vector<CryptoPolicyRevision>, std::less<>> policies_;
    std::map<std::string, CryptoProviderManifest, std::less<>> providers_;
    std::map<std::string, std::vector<CryptoProviderAssessment>, std::less<>>
        assessments_;
    std::map<std::string, std::string, std::less<>> assessment_index_;
};

} // namespace genesis::security
