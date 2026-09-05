#include "genesis/genesis.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {

std::string digest(std::string_view value) {
    return genesis::runtime::sha256(value);
}

genesis::security::CryptoThreatModelDraft threat_model() {
    using genesis::security::CryptoThreatCategory;
    genesis::security::CryptoThreatModelDraft draft;
    draft.id = "benchmark-threat-model";
    draft.threats = {
        CryptoThreatCategory::supply_chain_compromise,
        CryptoThreatCategory::binary_tampering,
        CryptoThreatCategory::rollback_attack,
        CryptoThreatCategory::key_theft,
        CryptoThreatCategory::weak_randomness,
        CryptoThreatCategory::algorithm_break,
        CryptoThreatCategory::quantum_cryptanalysis,
    };
    draft.asset_inventory_digest = digest("benchmark-only asset inventory");
    draft.trust_boundaries_digest = digest("benchmark-only trust boundaries");
    draft.mitigation_plan_digest = digest("benchmark-only mitigation plan");
    draft.residual_risk_digest = digest("benchmark-only residual risk");
    draft.evidence_digest = digest("benchmark-only threat evidence");
    draft.effective_from = 10U;
    draft.review_at = 1'000'000U;
    draft.recorded_at = 10U;
    return draft;
}

genesis::security::CryptoPolicyDraft policy() {
    genesis::security::CryptoAlgorithmRule rule;
    rule.algorithm_id = "benchmark-hash-v1";
    rule.function = genesis::security::CryptoFunction::secure_hash;
    rule.quantum_readiness = genesis::security::QuantumReadiness::traditional;
    rule.disposition = genesis::security::AlgorithmDisposition::approved;
    rule.security_strength_bits = 128U;
    rule.standard_reference = "BENCHMARK-ONLY-SYNTHETIC-POLICY";
    rule.standard_evidence_digest = digest("benchmark-only policy reference");

    genesis::security::CryptoPolicyDraft draft;
    draft.id = "benchmark-policy";
    draft.threat_model_id = "benchmark-threat-model";
    draft.threat_model_revision = 1U;
    draft.source_set_digest = digest("benchmark-only source set");
    draft.change_evidence_digest = digest("benchmark-only policy evidence");
    draft.rules = {std::move(rule)};
    draft.effective_from = 20U;
    draft.review_at = 900'000U;
    draft.recorded_at = 20U;
    return draft;
}

genesis::security::CryptoRouteEvidence route(bool validated,
                                             std::string_view provider_suffix) {
    genesis::security::CryptoRouteEvidence evidence;
    evidence.algorithm_id = "benchmark-hash-v1";
    evidence.function = genesis::security::CryptoFunction::secure_hash;
    evidence.implementation_route = "benchmark-route";
    evidence.implementation_digest =
        digest("implementation-" + std::string(provider_suffix));
    evidence.functional_test_digest = digest("benchmark-only functional evidence");
    evidence.platform_evidence_digest = digest("benchmark-only platform evidence");
    if (validated) {
        evidence.algorithm_validation_reference =
            "BENCHMARK-ONLY-NOT-A-VALIDATION";
        evidence.algorithm_validation_evidence_digest =
            digest("benchmark-only algorithm validation placeholder");
    }
    return evidence;
}

} // namespace

int main(int argc, char** argv) {
    const auto provider_total = argc > 1
                                    ? static_cast<std::size_t>(std::stoull(argv[1]))
                                    : 10'000U;
    if (provider_total == 0U
        || provider_total > std::numeric_limits<std::size_t>::max() / 2U) {
        std::cerr << "invalid crypto benchmark provider count\n";
        return 1;
    }

    constexpr std::string_view namespace_id = "crypto.benchmark";
    auto owner = genesis::identity::make_entity_address(
        std::string(namespace_id),
        genesis::identity::EntityKind::organism,
        "benchmark-owner",
        digest("benchmark owner provenance"),
        1U);
    auto observer = genesis::identity::make_entity_address(
        std::string(namespace_id),
        genesis::identity::EntityKind::person,
        "benchmark-observer",
        digest("benchmark observer provenance"),
        2U);
    auto qualifier = genesis::identity::make_entity_address(
        std::string(namespace_id),
        genesis::identity::EntityKind::organization,
        "benchmark-qualifier",
        digest("benchmark qualifier provenance"),
        2U);
    genesis::identity::EntityRegistry entities{
        std::string(namespace_id), owner.local_key, 8U, 8U};
    if (!entities.register_entity(owner) || !entities.register_entity(observer)
        || !entities.register_entity(qualifier) || !entities.verify()) {
        std::cerr << "crypto benchmark entity setup failed\n";
        return 1;
    }

    const auto registry_id = genesis::security::derive_crypto_registry_id(
        namespace_id, owner.entity_id);
    genesis::security::CryptoProviderRegistry registry(registry_id,
                                                        std::string(namespace_id),
                                                        owner.entity_id,
                                                        2U,
                                                        2U,
                                                        provider_total,
                                                        provider_total * 2U);
    if (!registry.append_threat_model(threat_model(), entities)
        || !registry.append_policy(policy(), entities)) {
        std::cerr << "crypto benchmark policy setup failed\n";
        return 1;
    }

    const auto build_start = std::chrono::steady_clock::now();
    for (std::size_t index = 0U; index < provider_total; ++index) {
        const auto suffix = std::to_string(index);
        genesis::security::CryptoProviderManifest provider;
        provider.implementation_name = "Synthetic Benchmark Provider " + suffix;
        provider.implementation_version = "1.0." + suffix;
        provider.platform_id = "benchmark-platform";
        provider.module_boundary_digest = digest("boundary-" + suffix);
        provider.module_binary_digest = digest("binary-" + suffix);
        provider.source_provenance_digest = digest("source-" + suffix);
        provider.license_evidence_digest = digest("license-" + suffix);
        provider.build_evidence_digest = digest("build-" + suffix);
        provider.capabilities = {{"benchmark-hash-v1",
                                  genesis::security::CryptoFunction::secure_hash,
                                  "benchmark-route",
                                  digest("implementation-" + suffix)}};
        provider.declared_at = 30U;
        provider.provider_id = genesis::security::derive_crypto_provider_id(
            provider.implementation_name,
            provider.implementation_version,
            provider.platform_id,
            provider.module_binary_digest);
        const auto provider_id = provider.provider_id;
        if (!registry.register_provider(std::move(provider), entities)) {
            std::cerr << "crypto benchmark provider registration failed at "
                      << index << '\n';
            return 1;
        }

        genesis::security::CryptoProviderAssessmentDraft observation;
        observation.id = "observation-" + suffix;
        observation.provider_id = provider_id;
        observation.kind = genesis::security::ProviderAssessmentKind::observation;
        observation.policy_id = "benchmark-policy";
        observation.policy_revision = 1U;
        observation.evaluator_entity_id = observer.entity_id;
        observation.evaluation_evidence_digest =
            digest("observation-evidence-" + suffix);
        observation.routes = {route(false, suffix)};
        observation.observed_at = 39U;
        observation.recorded_at = 40U;
        if (!registry.record_assessment(std::move(observation), entities)) {
            std::cerr << "crypto benchmark observation failed at " << index << '\n';
            return 1;
        }

        genesis::security::CryptoProviderAssessmentDraft qualification;
        qualification.id = "qualification-" + suffix;
        qualification.provider_id = provider_id;
        qualification.kind =
            genesis::security::ProviderAssessmentKind::qualification;
        qualification.policy_id = "benchmark-policy";
        qualification.policy_revision = 1U;
        qualification.evaluator_entity_id = qualifier.entity_id;
        qualification.evaluation_evidence_digest =
            digest("qualification-evidence-" + suffix);
        qualification.module_validation_reference =
            "BENCHMARK-ONLY-NOT-A-MODULE-VALIDATION";
        qualification.module_validation_evidence_digest =
            digest("benchmark-only module validation placeholder-" + suffix);
        qualification.routes = {route(true, suffix)};
        qualification.observed_at = 49U;
        qualification.recorded_at = 50U;
        qualification.valid_until = 800'000U;
        if (!registry.record_assessment(std::move(qualification), entities)) {
            std::cerr << "crypto benchmark qualification failed at " << index << '\n';
            return 1;
        }
    }
    const auto build_end = std::chrono::steady_clock::now();

    const auto snapshot = genesis::security::CryptoProviderStore::serialize(registry);
    const auto root = std::filesystem::temp_directory_path()
                      / "genesis-crypto-provider-benchmark";
    std::error_code filesystem_error;
    std::filesystem::remove_all(root, filesystem_error);
    genesis::security::CryptoProviderStore store(root);
    genesis::security::CryptoStoreError error;
    const auto roundtrip_start = std::chrono::steady_clock::now();
    const auto written = store.write(registry, "1.0.0", &error);
    const auto restored = store.read(registry_id,
                                     namespace_id,
                                     owner.entity_id,
                                     "1.0.0",
                                     &error);
    const auto roundtrip_end = std::chrono::steady_clock::now();
    const auto sample_id = genesis::security::derive_crypto_provider_id(
        "Synthetic Benchmark Provider 0",
        "1.0.0",
        "benchmark-platform",
        digest("binary-0"));
    const auto sample = restored.has_value()
                            ? restored->evaluate(sample_id,
                                                 "benchmark-hash-v1",
                                                 genesis::security::CryptoFunction::secure_hash,
                                                 60U)
                            : genesis::security::CryptoProviderDecision{};
    const auto verified = written && restored.has_value() && restored->verify()
                          && restored->providers().size() == provider_total
                          && restored->assessment_count() == provider_total * 2U
                          && restored->audit_entities(entities).clean()
                          && genesis::security::CryptoProviderStore::serialize(*restored)
                                 == snapshot
                          && sample.integration_candidate
                          && !sample.cryptographic_operation_available
                          && !sample.identity_authenticated
                          && !sample.provenance_authenticated
                          && !sample.action_authorized;
    std::cout << "crypto_providers=" << provider_total
              << " assessment_versions=" << provider_total * 2U
              << " snapshot_bytes=" << snapshot.size()
              << " build_ms="
              << std::chrono::duration_cast<std::chrono::milliseconds>(build_end
                                                                        - build_start)
                     .count()
              << " roundtrip_ms="
              << std::chrono::duration_cast<std::chrono::milliseconds>(roundtrip_end
                                                                        - roundtrip_start)
                     .count()
              << " snapshot_digest=" << genesis::runtime::sha256(snapshot)
              << " synthetic_only=1 operation_enabled=0 verified="
              << (verified ? 1 : 0) << '\n';
    std::filesystem::remove_all(root, filesystem_error);
    return verified ? 0 : 1;
}
