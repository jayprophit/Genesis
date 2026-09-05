#include "genesis/genesis.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using namespace genesis::security;

std::string digest(std::string_view value) {
    return genesis::runtime::sha256(value);
}

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct BenchmarkFixture final {
    std::string namespace_id{"key.benchmark"};
    genesis::identity::EntityAddress owner = genesis::identity::make_entity_address(
        namespace_id,
        genesis::identity::EntityKind::organism,
        "owner",
        digest("benchmark owner"),
        1U);
    genesis::identity::EntityAddress custodian =
        genesis::identity::make_entity_address(namespace_id,
                                               genesis::identity::EntityKind::person,
                                               "custodian",
                                               digest("benchmark custodian"),
                                               2U);
    genesis::identity::EntityAddress recovery =
        genesis::identity::make_entity_address(
            namespace_id,
            genesis::identity::EntityKind::organization,
            "recovery",
            digest("benchmark recovery"),
            2U);
    genesis::identity::EntityAddress qualifier =
        genesis::identity::make_entity_address(
            namespace_id,
            genesis::identity::EntityKind::organization,
            "qualifier",
            digest("benchmark qualifier"),
            2U);
    genesis::identity::EntityAddress observer =
        genesis::identity::make_entity_address(namespace_id,
                                               genesis::identity::EntityKind::person,
                                               "observer",
                                               digest("benchmark observer"),
                                               2U);
    genesis::identity::EntityRegistry entities{namespace_id, owner.local_key, 16U, 16U};
    CryptoProviderRegistry providers{
        derive_crypto_registry_id(namespace_id, owner.entity_id),
        namespace_id,
        owner.entity_id,
        8U,
        8U,
        8U,
        16U};
    std::string custody_policy_digest{digest("benchmark custody policy")};
    CryptoProviderManifest provider;

    BenchmarkFixture() {
        for (const auto* entity : {&owner, &custodian, &recovery, &qualifier, &observer}) {
            require(entities.register_entity(*entity), "benchmark entity registration failed");
        }
        provider.implementation_name = "Synthetic Key Benchmark Provider";
        provider.implementation_version = "1.0.0";
        provider.platform_id = "benchmark-platform-x64";
        provider.module_boundary_digest = digest("benchmark module boundary");
        provider.module_binary_digest = digest("benchmark module binary");
        provider.source_provenance_digest = digest("benchmark source provenance");
        provider.license_evidence_digest = digest("benchmark license evidence");
        provider.build_evidence_digest = digest("benchmark build evidence");
        provider.capabilities = {{"sign-benchmark-v1",
                                  CryptoFunction::digital_signature,
                                  "synthetic-benchmark-sign-route",
                                  digest("benchmark sign implementation")}};
        provider.declared_at = 30U;
        provider.provider_id = derive_crypto_provider_id(provider.implementation_name,
                                                         provider.implementation_version,
                                                         provider.platform_id,
                                                         provider.module_binary_digest);

        CryptoThreatModelDraft threat;
        threat.id = "benchmark-threat";
        threat.threats = {CryptoThreatCategory::remote_attacker,
                          CryptoThreatCategory::supply_chain_compromise,
                          CryptoThreatCategory::binary_tampering,
                          CryptoThreatCategory::rollback_attack,
                          CryptoThreatCategory::key_theft,
                          CryptoThreatCategory::weak_randomness,
                          CryptoThreatCategory::algorithm_break,
                          CryptoThreatCategory::quantum_cryptanalysis};
        threat.asset_inventory_digest = digest("benchmark assets");
        threat.trust_boundaries_digest = digest("benchmark boundaries");
        threat.mitigation_plan_digest = digest("benchmark mitigations");
        threat.residual_risk_digest = digest("benchmark residual risk");
        threat.evidence_digest = digest("benchmark threat evidence");
        threat.effective_from = 10U;
        threat.review_at = 10'000'000U;
        threat.recorded_at = 10U;
        require(providers.append_threat_model(std::move(threat), entities),
                "benchmark threat append failed");

        CryptoAlgorithmRule rule;
        rule.algorithm_id = "sign-benchmark-v1";
        rule.function = CryptoFunction::digital_signature;
        rule.quantum_readiness = QuantumReadiness::hybrid;
        rule.disposition = AlgorithmDisposition::approved;
        rule.security_strength_bits = 128U;
        rule.standard_reference = "SYNTHETIC-BENCHMARK-ONLY";
        rule.standard_evidence_digest = digest("benchmark rule evidence");
        CryptoPolicyDraft policy;
        policy.id = "benchmark-policy";
        policy.threat_model_id = "benchmark-threat";
        policy.threat_model_revision = 1U;
        policy.source_set_digest = digest("benchmark source set");
        policy.change_evidence_digest = digest("benchmark policy change");
        policy.rules = {rule};
        policy.effective_from = 20U;
        policy.review_at = 9'000'000U;
        policy.recorded_at = 20U;
        require(providers.append_policy(std::move(policy), entities),
                "benchmark policy append failed");
        require(providers.register_provider(provider, entities),
                "benchmark provider registration failed");

        CryptoRouteEvidence route;
        route.algorithm_id = "sign-benchmark-v1";
        route.function = CryptoFunction::digital_signature;
        route.implementation_route = "synthetic-benchmark-sign-route";
        route.implementation_digest = digest("benchmark sign implementation");
        route.functional_test_digest = digest("benchmark functional evidence");
        route.platform_evidence_digest = digest("benchmark platform evidence");
        CryptoProviderAssessmentDraft observation;
        observation.id = "benchmark-observation";
        observation.provider_id = provider.provider_id;
        observation.kind = ProviderAssessmentKind::observation;
        observation.policy_id = "benchmark-policy";
        observation.policy_revision = 1U;
        observation.evaluator_entity_id = observer.entity_id;
        observation.evaluation_evidence_digest = digest("benchmark observation evidence");
        observation.routes = {route};
        observation.observed_at = 39U;
        observation.recorded_at = 40U;
        require(providers.record_assessment(std::move(observation), entities),
                "benchmark provider observation failed");

        route.algorithm_validation_reference = "SYNTHETIC-BENCHMARK-VALIDATION";
        route.algorithm_validation_evidence_digest = digest("benchmark validation evidence");
        route.key_custody_evidence_digest = custody_policy_digest;
        CryptoProviderAssessmentDraft qualification;
        qualification.id = "benchmark-qualification";
        qualification.provider_id = provider.provider_id;
        qualification.kind = ProviderAssessmentKind::qualification;
        qualification.policy_id = "benchmark-policy";
        qualification.policy_revision = 1U;
        qualification.evaluator_entity_id = qualifier.entity_id;
        qualification.evaluation_evidence_digest = digest("benchmark qualification evidence");
        qualification.module_validation_reference = "SYNTHETIC-BENCHMARK-MODULE";
        qualification.module_validation_evidence_digest = digest("benchmark module evidence");
        qualification.routes = {route};
        qualification.observed_at = 49U;
        qualification.recorded_at = 50U;
        qualification.valid_until = 8'000'000U;
        require(providers.record_assessment(std::move(qualification), entities),
                "benchmark provider qualification failed");
    }
};

} // namespace

int main(int argc, char** argv) {
    try {
        const auto requested = argc > 1 ? std::stoull(argv[1]) : 10'000ULL;
        if (requested == 0U || requested > 250'000U) {
            throw std::invalid_argument("key count must be between 1 and 250000");
        }
        const auto key_count = static_cast<std::size_t>(requested);
        BenchmarkFixture fixture;
        KeyCustodyRegistry registry(
            derive_key_custody_registry_id(fixture.namespace_id, fixture.owner.entity_id),
            fixture.namespace_id,
            fixture.owner.entity_id,
            key_count,
            key_count * 3U,
            1U);

        const auto build_start = std::chrono::steady_clock::now();
        for (std::size_t index = 0U; index < key_count; ++index) {
            const auto registered_at = static_cast<std::uint64_t>(60U + index * 4U);
            KeyHandleManifest key;
            key.owner_entity_id = fixture.owner.entity_id;
            key.provider_id = fixture.provider.provider_id;
            key.algorithm_id = "sign-benchmark-v1";
            key.function = CryptoFunction::digital_signature;
            key.implementation_route = "synthetic-benchmark-sign-route";
            key.implementation_digest = digest("benchmark sign implementation");
            key.provider_locator_digest =
                digest("synthetic external locator " + std::to_string(index));
            key.origin = KeyOrigin::generated;
            key.export_policy = KeyExportPolicy::non_exportable;
            key.permitted_usages = {KeyUsage::sign, KeyUsage::verify};
            key.custodian_entity_id = fixture.custodian.entity_id;
            key.recovery_authority_entity_id = fixture.recovery.entity_id;
            key.operator_entity_ids = {fixture.custodian.entity_id};
            key.custody_policy_digest = fixture.custody_policy_digest;
            key.creation_evidence_digest =
                digest("synthetic creation evidence " + std::to_string(index));
            key.attestation_evidence_digest =
                digest("synthetic attestation evidence " + std::to_string(index));
            key.generation = 1U;
            key.not_before = registered_at;
            key.not_after = 7'000'000U;
            key.registered_at = registered_at;
            key.key_id = derive_key_handle_id(key.owner_entity_id,
                                              key.provider_id,
                                              key.algorithm_id,
                                              key.function,
                                              key.implementation_route,
                                              key.implementation_digest,
                                              key.provider_locator_digest,
                                              key.generation);
            const auto key_id = key.key_id;
            require(registry.register_key(std::move(key),
                                          fixture.providers,
                                          fixture.entities),
                    "benchmark key registration failed");

            const auto append_transition = [&](KeyTransitionKind kind,
                                               std::uint64_t at) {
                KeyTransitionDraft transition;
                transition.id = "event-" + std::to_string(index) + '-'
                                + std::string(to_string(kind));
                transition.key_id = key_id;
                transition.kind = kind;
                transition.actor_entity_id = fixture.custodian.entity_id;
                transition.reason_digest =
                    digest("synthetic reason " + transition.id);
                transition.evidence_digest =
                    digest("synthetic evidence " + transition.id);
                transition.occurred_at = at;
                transition.recorded_at = at;
                require(registry.record_transition(std::move(transition),
                                                   fixture.providers,
                                                   fixture.entities),
                        "benchmark transition append failed");
            };
            append_transition(KeyTransitionKind::activate, registered_at + 1U);
            append_transition(KeyTransitionKind::suspend, registered_at + 2U);
            append_transition(KeyTransitionKind::resume, registered_at + 3U);
        }
        const auto build_end = std::chrono::steady_clock::now();
        require(registry.verify(), "benchmark registry verification failed");

        const auto roundtrip_start = std::chrono::steady_clock::now();
        const auto bytes = KeyCustodyStore::serialize(registry);
        auto restored = KeyCustodyStore::deserialize(bytes);
        require(restored.has_value() && restored->verify(),
                "benchmark registry roundtrip failed");
        const auto roundtrip_end = std::chrono::steady_clock::now();

        const auto build_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  build_end - build_start)
                                  .count();
        const auto roundtrip_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      roundtrip_end - roundtrip_start)
                                      .count();
        std::cout << "key_handles=" << key_count
                  << " transition_records=" << registry.transition_count()
                  << " snapshot_bytes=" << bytes.size() << " build_ms=" << build_ms
                  << " roundtrip_ms=" << roundtrip_ms
                  << " snapshot_digest=" << digest(bytes)
                  << " synthetic_only=1 key_material_present=0 operation_executed=0"
                  << " verified=1\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "key custody benchmark failed: " << exception.what() << '\n';
        return 1;
    }
}
