#include "genesis/genesis.hpp"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

using genesis::identity::EntityAddress;
using genesis::identity::EntityKind;
using genesis::identity::EntityRegistry;
using genesis::security::AlgorithmDisposition;
using genesis::security::CryptoAlgorithmCapability;
using genesis::security::CryptoAlgorithmRule;
using genesis::security::CryptoFunction;
using genesis::security::CryptoPolicyDraft;
using genesis::security::CryptoProviderAssessmentDraft;
using genesis::security::CryptoProviderManifest;
using genesis::security::CryptoProviderRegistry;
using genesis::security::CryptoProviderStore;
using genesis::security::CryptoRegistryError;
using genesis::security::CryptoRegistryErrorCode;
using genesis::security::CryptoRouteEvidence;
using genesis::security::CryptoStoreError;
using genesis::security::CryptoStoreErrorCode;
using genesis::security::CryptoThreatCategory;
using genesis::security::CryptoThreatModelDraft;
using genesis::security::ProviderAssessmentKind;
using genesis::security::ProviderState;
using genesis::security::QuantumReadiness;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string digest(std::string_view value) {
    return genesis::runtime::sha256(value);
}

std::vector<CryptoThreatCategory> qualification_threats() {
    return {
        CryptoThreatCategory::quantum_cryptanalysis,
        CryptoThreatCategory::weak_randomness,
        CryptoThreatCategory::rollback_attack,
        CryptoThreatCategory::supply_chain_compromise,
        CryptoThreatCategory::algorithm_break,
        CryptoThreatCategory::key_theft,
        CryptoThreatCategory::binary_tampering,
        CryptoThreatCategory::remote_attacker,
    };
}

CryptoThreatModelDraft threat_draft(std::uint64_t effective = 10U,
                                    std::uint64_t recorded = 10U,
                                    std::uint64_t review = 1'000U) {
    CryptoThreatModelDraft draft;
    draft.id = "threat-main";
    draft.threats = qualification_threats();
    draft.asset_inventory_digest = digest("test asset inventory");
    draft.trust_boundaries_digest = digest("test trust boundaries");
    draft.mitigation_plan_digest = digest("test mitigation plan");
    draft.residual_risk_digest = digest("test residual risk");
    draft.evidence_digest = digest("test threat review evidence");
    draft.effective_from = effective;
    draft.review_at = review;
    draft.recorded_at = recorded;
    return draft;
}

CryptoAlgorithmRule rule(std::string algorithm_id,
                         CryptoFunction function,
                         AlgorithmDisposition disposition =
                             AlgorithmDisposition::approved) {
    CryptoAlgorithmRule value;
    value.algorithm_id = std::move(algorithm_id);
    value.function = function;
    value.quantum_readiness = function == CryptoFunction::digital_signature
                                  ? QuantumReadiness::hybrid
                                  : QuantumReadiness::traditional;
    value.disposition = disposition;
    value.security_strength_bits = 128U;
    value.standard_reference = "TEST-ONLY-SYNTHETIC-POLICY";
    value.standard_evidence_digest = digest("test policy source");
    return value;
}

CryptoPolicyDraft policy_draft(std::uint64_t effective = 20U,
                               std::uint64_t recorded = 20U,
                               std::uint64_t review = 900U) {
    CryptoPolicyDraft draft;
    draft.id = "policy-main";
    draft.threat_model_id = "threat-main";
    draft.threat_model_revision = 1U;
    draft.source_set_digest = digest("test reference set");
    draft.change_evidence_digest = digest("test policy change");
    draft.rules = {
        rule("sign-synthetic-v1", CryptoFunction::digital_signature),
        rule("hash-synthetic-v1", CryptoFunction::secure_hash),
    };
    draft.effective_from = effective;
    draft.review_at = review;
    draft.recorded_at = recorded;
    return draft;
}

CryptoProviderManifest provider_manifest() {
    CryptoProviderManifest manifest;
    manifest.implementation_name = "Synthetic Test Provider";
    manifest.implementation_version = "1.0.0";
    manifest.platform_id = "test-platform-x64";
    manifest.module_boundary_digest = digest("test module boundary");
    manifest.module_binary_digest = digest("test module binary");
    manifest.source_provenance_digest = digest("test source provenance");
    manifest.license_evidence_digest = digest("test license evidence");
    manifest.build_evidence_digest = digest("test build evidence");
    manifest.capabilities = {
        {"sign-synthetic-v1",
         CryptoFunction::digital_signature,
         "synthetic-sign-route",
         digest("test sign implementation")},
        {"hash-synthetic-v1",
         CryptoFunction::secure_hash,
         "synthetic-hash-route",
         digest("test hash implementation")},
    };
    manifest.declared_at = 30U;
    manifest.provider_id = genesis::security::derive_crypto_provider_id(
        manifest.implementation_name,
        manifest.implementation_version,
        manifest.platform_id,
        manifest.module_binary_digest);
    return manifest;
}

CryptoRouteEvidence route(std::string algorithm_id,
                          CryptoFunction function,
                          bool validated,
                          bool custody = false) {
    CryptoRouteEvidence evidence;
    evidence.algorithm_id = std::move(algorithm_id);
    evidence.function = function;
    evidence.implementation_route = function == CryptoFunction::digital_signature
                                        ? "synthetic-sign-route"
                                        : "synthetic-hash-route";
    evidence.implementation_digest =
        digest(function == CryptoFunction::digital_signature
                   ? "test sign implementation"
                   : "test hash implementation");
    evidence.functional_test_digest = digest("test functional " + evidence.algorithm_id);
    evidence.platform_evidence_digest = digest("test platform " + evidence.algorithm_id);
    if (validated) {
        evidence.algorithm_validation_reference = "TEST-ONLY-ALGORITHM-VALIDATION";
        evidence.algorithm_validation_evidence_digest =
            digest("test algorithm validation " + evidence.algorithm_id);
    }
    if (custody) {
        evidence.key_custody_evidence_digest =
            digest("test custody evidence " + evidence.algorithm_id);
    }
    return evidence;
}

struct Fixture final {
    std::string namespace_id{"crypto.test"};
    EntityAddress owner = genesis::identity::make_entity_address(
        namespace_id, EntityKind::organism, "owner", digest("owner provenance"), 1U);
    EntityAddress observer = genesis::identity::make_entity_address(
        namespace_id, EntityKind::person, "observer", digest("observer provenance"), 2U);
    EntityAddress qualifier = genesis::identity::make_entity_address(
        namespace_id,
        EntityKind::organization,
        "qualifier",
        digest("qualifier provenance"),
        2U);
    EntityAddress wrong_evaluator = genesis::identity::make_entity_address(
        namespace_id,
        EntityKind::component,
        "wrong-evaluator",
        digest("component provenance"),
        2U);
    EntityRegistry entities{namespace_id, owner.local_key, 32U, 32U};
    CryptoProviderRegistry registry{
        genesis::security::derive_crypto_registry_id(namespace_id, owner.entity_id),
        namespace_id,
        owner.entity_id,
        32U,
        32U,
        32U,
        128U};

    Fixture() {
        require(entities.register_entity(owner), "owner registration failed");
        require(entities.register_entity(observer), "observer registration failed");
        require(entities.register_entity(qualifier), "qualifier registration failed");
        require(entities.register_entity(wrong_evaluator),
                "wrong evaluator registration failed");
        require(entities.verify(), "fixture entity registry invalid");
    }

    std::string provider_id() const {
        return provider_manifest().provider_id;
    }

    CryptoProviderAssessmentDraft observation(std::uint64_t recorded = 40U) const {
        CryptoProviderAssessmentDraft draft;
        draft.id = "observation-" + std::to_string(recorded);
        draft.provider_id = provider_id();
        draft.kind = ProviderAssessmentKind::observation;
        draft.policy_id = "policy-main";
        draft.policy_revision = 1U;
        draft.evaluator_entity_id = observer.entity_id;
        draft.evaluation_evidence_digest = digest("test observation evidence");
        draft.routes = {
            route("sign-synthetic-v1", CryptoFunction::digital_signature, false),
            route("hash-synthetic-v1", CryptoFunction::secure_hash, false),
        };
        draft.observed_at = recorded - 1U;
        draft.recorded_at = recorded;
        return draft;
    }

    CryptoProviderAssessmentDraft qualification(std::uint64_t recorded = 50U,
                                                std::uint64_t policy_revision = 1U,
                                                bool sign_custody = false) const {
        CryptoProviderAssessmentDraft draft;
        draft.id = "qualification-" + std::to_string(recorded);
        draft.provider_id = provider_id();
        draft.kind = ProviderAssessmentKind::qualification;
        draft.policy_id = "policy-main";
        draft.policy_revision = policy_revision;
        draft.evaluator_entity_id = qualifier.entity_id;
        draft.evaluation_evidence_digest = digest("test qualification evidence");
        draft.module_validation_reference = "TEST-ONLY-MODULE-VALIDATION";
        draft.module_validation_evidence_digest = digest("test module validation");
        draft.routes = {
            route("sign-synthetic-v1",
                  CryptoFunction::digital_signature,
                  true,
                  sign_custody),
            route("hash-synthetic-v1", CryptoFunction::secure_hash, true),
        };
        draft.observed_at = recorded - 1U;
        draft.recorded_at = recorded;
        draft.valid_until = 800U;
        return draft;
    }

    void seed_to_observed() {
        require(registry.append_threat_model(threat_draft(), entities),
                "fixture threat model append failed");
        require(registry.append_policy(policy_draft(), entities),
                "fixture policy append failed");
        require(registry.register_provider(provider_manifest(), entities),
                "fixture provider registration failed");
        require(registry.record_assessment(observation(), entities),
                "fixture observation failed");
    }

    void seed_to_qualified(bool sign_custody = false) {
        seed_to_observed();
        require(registry.record_assessment(qualification(50U, 1U, sign_custody),
                                           entities),
                "fixture qualification failed");
        require(registry.verify(), "qualified fixture registry invalid");
    }
};

void test_enum_codecs_and_identity_derivation() {
    for (const auto value : {CryptoFunction::secure_hash,
                             CryptoFunction::message_authentication,
                             CryptoFunction::authenticated_encryption,
                             CryptoFunction::digital_signature,
                             CryptoFunction::key_establishment,
                             CryptoFunction::key_derivation,
                             CryptoFunction::random_generation,
                             CryptoFunction::key_wrapping}) {
        CryptoFunction parsed{};
        require(genesis::security::crypto_function_from_string(
                    genesis::security::to_string(value), parsed)
                    && parsed == value,
                "crypto function codec failed");
    }
    for (const auto value : {QuantumReadiness::not_applicable,
                             QuantumReadiness::traditional,
                             QuantumReadiness::post_quantum,
                             QuantumReadiness::hybrid}) {
        QuantumReadiness parsed{};
        require(genesis::security::quantum_readiness_from_string(
                    genesis::security::to_string(value), parsed)
                    && parsed == value,
                "quantum-readiness codec failed");
    }
    for (const auto value : {AlgorithmDisposition::candidate,
                             AlgorithmDisposition::approved,
                             AlgorithmDisposition::verify_only,
                             AlgorithmDisposition::prohibited}) {
        AlgorithmDisposition parsed{};
        require(genesis::security::algorithm_disposition_from_string(
                    genesis::security::to_string(value), parsed)
                    && parsed == value,
                "algorithm disposition codec failed");
    }
    for (std::uint8_t index = 0U; index <= 15U; ++index) {
        const auto value = static_cast<CryptoThreatCategory>(index);
        CryptoThreatCategory parsed{};
        require(genesis::security::crypto_threat_category_from_string(
                    genesis::security::to_string(value), parsed)
                    && parsed == value,
                "threat category codec failed");
    }
    for (const auto value : {ProviderAssessmentKind::observation,
                             ProviderAssessmentKind::qualification,
                             ProviderAssessmentKind::suspension,
                             ProviderAssessmentKind::revocation}) {
        ProviderAssessmentKind parsed{};
        require(genesis::security::provider_assessment_kind_from_string(
                    genesis::security::to_string(value), parsed)
                    && parsed == value,
                "assessment kind codec failed");
    }
    for (const auto value : {ProviderState::declared,
                             ProviderState::observed,
                             ProviderState::qualified,
                             ProviderState::suspended,
                             ProviderState::revoked}) {
        ProviderState parsed{};
        require(genesis::security::provider_state_from_string(
                    genesis::security::to_string(value), parsed)
                    && parsed == value,
                "provider state codec failed");
    }
    CryptoFunction untouched = CryptoFunction::secure_hash;
    require(!genesis::security::crypto_function_from_string("unknown", untouched),
            "unknown enum text was accepted");
    require(!genesis::security::crypto_function_requires_key_material(
                CryptoFunction::secure_hash)
                && !genesis::security::crypto_function_requires_key_material(
                    CryptoFunction::random_generation)
                && genesis::security::crypto_function_requires_key_material(
                    CryptoFunction::digital_signature),
            "key-material classification failed");

    Fixture fixture;
    require(fixture.registry.registry_id()
                == genesis::security::derive_crypto_registry_id(
                    fixture.namespace_id, fixture.owner.entity_id),
            "registry identity derivation is unstable");
    require(fixture.provider_id() == provider_manifest().provider_id,
            "provider identity derivation is unstable");
    bool threw = false;
    try {
        static_cast<void>(CryptoProviderRegistry("wrong",
                                                 fixture.namespace_id,
                                                 fixture.owner.entity_id,
                                                 1U,
                                                 1U,
                                                 1U,
                                                 1U));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    require(threw, "registry accepted an unbound identity");
}

void test_threat_policy_and_provider_validation() {
    Fixture fixture;
    CryptoRegistryError error;
    auto duplicate_threat = threat_draft();
    duplicate_threat.threats.push_back(CryptoThreatCategory::key_theft);
    require(!fixture.registry.append_threat_model(
                duplicate_threat, fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::duplicate_entry,
            "duplicate threat category was accepted");
    require(fixture.registry.append_threat_model(
                threat_draft(), fixture.entities, &error),
            "valid threat model was rejected");
    const auto* stored_threat = fixture.registry.threat_model("threat-main", 1U);
    require(stored_threat != nullptr
                && std::is_sorted(stored_threat->threats.begin(),
                                  stored_threat->threats.end(),
                                  [](const auto left, const auto right) {
                                      return static_cast<std::uint8_t>(left)
                                             < static_cast<std::uint8_t>(right);
                                  })
                && stored_threat->revision == 1U
                && !stored_threat->revision_digest.empty(),
            "threat model was not canonicalized");

    auto backwards = threat_draft(9U, 11U, 1'000U);
    require(!fixture.registry.append_threat_model(
                backwards, fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::temporal_conflict,
            "backward threat revision was accepted");

    auto missing_threat = policy_draft();
    missing_threat.threat_model_id = "missing";
    require(!fixture.registry.append_policy(missing_threat, fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::missing_threat_model,
            "policy with missing threat model was accepted");
    auto duplicate_rule = policy_draft();
    duplicate_rule.rules.push_back(duplicate_rule.rules.front());
    require(!fixture.registry.append_policy(duplicate_rule, fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::duplicate_entry,
            "duplicate policy rule was accepted");
    auto early_transition = policy_draft();
    early_transition.rules.front().deprecate_at = 19U;
    require(!fixture.registry.append_policy(early_transition,
                                            fixture.entities,
                                            &error)
                && error.code == CryptoRegistryErrorCode::temporal_conflict,
            "policy accepted a transition before its effective time");
    require(fixture.registry.append_policy(policy_draft(), fixture.entities, &error),
            "valid policy was rejected");

    auto wrong_provider = provider_manifest();
    wrong_provider.provider_id = "provider-wrong";
    require(!fixture.registry.register_provider(
                wrong_provider, fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::invalid_entry,
            "provider with a false derived identity was accepted");
    auto duplicate_capability = provider_manifest();
    duplicate_capability.capabilities.push_back(
        duplicate_capability.capabilities.front());
    require(!fixture.registry.register_provider(
                duplicate_capability, fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::invalid_entry,
            "duplicate provider capability was accepted");
    require(fixture.registry.register_provider(
                provider_manifest(), fixture.entities, &error),
            "valid provider manifest was rejected");
    require(!fixture.registry.register_provider(
                provider_manifest(), fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::duplicate_entry,
            "duplicate provider manifest was accepted");
    require(fixture.registry.provider_state(fixture.provider_id())
                == ProviderState::declared
                && fixture.registry.verify(),
            "declared provider registry did not verify");
}

void test_assessment_gates_and_no_authority() {
    Fixture fixture;
    require(fixture.registry.append_threat_model(threat_draft(), fixture.entities),
            "threat append failed");
    require(fixture.registry.append_policy(policy_draft(), fixture.entities),
            "policy append failed");
    require(fixture.registry.register_provider(provider_manifest(), fixture.entities),
            "provider registration failed");
    CryptoRegistryError error;

    auto premature_qualification = fixture.qualification();
    require(!fixture.registry.record_assessment(premature_qualification,
                                                fixture.entities,
                                                &error)
                && error.code == CryptoRegistryErrorCode::transition_conflict,
            "provider qualified without prior observation");
    require(fixture.registry.assessments().empty() && fixture.registry.verify(),
            "rejected qualification mutated provider history");
    auto missing_evaluator = fixture.observation();
    missing_evaluator.evaluator_entity_id = "missing-evaluator";
    require(!fixture.registry.record_assessment(
                missing_evaluator, fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::missing_evaluator,
            "missing evaluator was accepted");
    auto wrong_evaluator = fixture.observation();
    wrong_evaluator.evaluator_entity_id = fixture.wrong_evaluator.entity_id;
    require(!fixture.registry.record_assessment(
                wrong_evaluator, fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::incompatible_evaluator,
            "incompatible evaluator type was accepted");
    auto duplicate_route = fixture.observation();
    duplicate_route.routes.push_back(duplicate_route.routes.front());
    require(!fixture.registry.record_assessment(
                duplicate_route, fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::duplicate_entry,
            "duplicate assessment route was accepted");
    auto substituted_route = fixture.observation();
    substituted_route.id = "substituted-route";
    substituted_route.routes.front().implementation_digest =
        digest("different implementation");
    require(!fixture.registry.record_assessment(
                substituted_route, fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::route_conflict,
            "assessment evidence was accepted for a different implementation");
    require(fixture.registry.assessments().empty() && fixture.registry.verify(),
            "rejected assessment left a partial history");
    require(fixture.registry.record_assessment(
                fixture.observation(), fixture.entities, &error),
            "valid observation was rejected");

    auto self_qualification = fixture.qualification();
    self_qualification.evaluator_entity_id = fixture.owner.entity_id;
    require(!fixture.registry.record_assessment(
                self_qualification, fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::incompatible_evaluator,
            "owner self-qualification was accepted");
    auto person_qualification = fixture.qualification();
    person_qualification.evaluator_entity_id = fixture.observer.entity_id;
    require(!fixture.registry.record_assessment(
                person_qualification, fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::incompatible_evaluator,
            "person qualification was accepted as independent review");
    require(fixture.registry.record_assessment(
                fixture.qualification(), fixture.entities, &error),
            "valid independent qualification was rejected");

    const auto hash_decision = fixture.registry.evaluate(
        fixture.provider_id(), "hash-synthetic-v1", CryptoFunction::secure_hash, 60U);
    require(hash_decision.state == ProviderState::qualified
                && hash_decision.provider_found
                && hash_decision.capability_declared
                && hash_decision.provider_observed
                && hash_decision.provider_qualified
                && hash_decision.policy_current
                && hash_decision.threat_model_current
                && hash_decision.algorithm_policy_approved
                && hash_decision.validation_evidence_recorded
                && !hash_decision.key_custody_required
                && hash_decision.integration_candidate,
            "qualified hash route was not an integration candidate");
    require(!hash_decision.cryptographic_operation_available
                && !hash_decision.identity_authenticated
                && !hash_decision.provenance_authenticated
                && !hash_decision.action_authorized,
            "registry evidence incorrectly claimed cryptographic authority");

    const auto sign_without_custody = fixture.registry.evaluate(
        fixture.provider_id(),
        "sign-synthetic-v1",
        CryptoFunction::digital_signature,
        60U);
    require(sign_without_custody.key_custody_required
                && !sign_without_custody.key_custody_evidence_recorded
                && !sign_without_custody.integration_candidate
                && sign_without_custody.reason == "key_custody_evidence_missing",
            "keyed route bypassed the custody-evidence gate");

    require(fixture.registry.record_assessment(
                fixture.qualification(65U, 1U, true), fixture.entities, &error),
            "requalification with custody evidence failed");
    const auto sign_with_custody = fixture.registry.evaluate(
        fixture.provider_id(),
        "sign-synthetic-v1",
        CryptoFunction::digital_signature,
        66U);
    require(sign_with_custody.key_custody_evidence_recorded
                && sign_with_custody.integration_candidate
                && !sign_with_custody.cryptographic_operation_available
                && !sign_with_custody.identity_authenticated,
            "custody evidence did not remain an evidence-only candidate gate");
    require(fixture.registry.verify(), "assessment registry failed verification");
}

CryptoProviderAssessmentDraft terminal_assessment(const Fixture& fixture,
                                                   std::string id,
                                                   ProviderAssessmentKind kind,
                                                   std::uint64_t recorded) {
    CryptoProviderAssessmentDraft draft;
    draft.id = std::move(id);
    draft.provider_id = fixture.provider_id();
    draft.kind = kind;
    draft.evaluator_entity_id = fixture.observer.entity_id;
    draft.evaluation_evidence_digest = digest("test state transition evidence");
    draft.observed_at = recorded;
    draft.recorded_at = recorded;
    return draft;
}

void test_policy_agility_and_provider_lifecycle() {
    Fixture fixture;
    fixture.seed_to_qualified(true);

    auto revision = policy_draft(60U, 60U, 900U);
    revision.change_evidence_digest = digest("test policy revision two");
    require(fixture.registry.append_policy(revision, fixture.entities),
            "policy revision two failed");
    const auto stale = fixture.registry.evaluate(
        fixture.provider_id(), "hash-synthetic-v1", CryptoFunction::secure_hash, 70U);
    require(stale.provider_qualified && !stale.policy_current
                && !stale.integration_candidate
                && stale.reason == "qualification_policy_is_not_current",
            "new policy revision did not invalidate stale qualification");

    auto stale_observation = fixture.observation(70U);
    stale_observation.id = "stale-policy-observation";
    require(!fixture.registry.record_assessment(stale_observation, fixture.entities),
            "assessment accepted a superseded policy revision");
    require(fixture.registry.record_assessment(
                fixture.qualification(75U, 2U, true), fixture.entities),
            "qualification against current policy failed");

    require(fixture.registry.record_assessment(
                terminal_assessment(fixture,
                                    "suspension-80",
                                    ProviderAssessmentKind::suspension,
                                    80U),
                fixture.entities),
            "provider suspension failed");
    const auto suspended = fixture.registry.evaluate(
        fixture.provider_id(), "hash-synthetic-v1", CryptoFunction::secure_hash, 81U);
    require(suspended.state == ProviderState::suspended
                && !suspended.integration_candidate,
            "suspended provider remained a candidate");

    auto observation = fixture.observation(85U);
    observation.id = "observation-85-policy-2";
    observation.policy_revision = 2U;
    require(fixture.registry.record_assessment(observation, fixture.entities),
            "suspended provider re-observation failed");
    require(fixture.registry.record_assessment(
                fixture.qualification(90U, 2U, true), fixture.entities),
            "re-observed provider requalification failed");
    require(fixture.registry.record_assessment(
                terminal_assessment(fixture,
                                    "revocation-100",
                                    ProviderAssessmentKind::revocation,
                                    100U),
                fixture.entities),
            "provider revocation failed");
    require(fixture.registry.provider_state(fixture.provider_id())
                == ProviderState::revoked,
            "provider did not enter terminal revoked state");
    auto post_revoke = fixture.observation(110U);
    post_revoke.id = "post-revoke-observation";
    post_revoke.policy_revision = 2U;
    CryptoRegistryError error;
    require(!fixture.registry.record_assessment(post_revoke,
                                                fixture.entities,
                                                &error)
                && error.code == CryptoRegistryErrorCode::transition_conflict,
            "revoked provider was revived");
    const auto revoked = fixture.registry.evaluate(
        fixture.provider_id(), "hash-synthetic-v1", CryptoFunction::secure_hash, 110U);
    require(revoked.state == ProviderState::revoked
                && !revoked.integration_candidate
                && revoked.reason == "provider_revoked",
            "revoked provider evaluation did not fail closed");
    require(fixture.registry.verify(), "provider lifecycle registry invalid");
}

void test_incomplete_threat_model_and_external_audit() {
    Fixture fixture;
    auto incomplete = threat_draft();
    incomplete.threats = {CryptoThreatCategory::remote_attacker};
    require(fixture.registry.append_threat_model(incomplete, fixture.entities),
            "incomplete threat model could not be recorded for review");
    require(fixture.registry.append_policy(policy_draft(), fixture.entities),
            "policy over incomplete threat model could not be recorded");
    require(fixture.registry.register_provider(provider_manifest(), fixture.entities),
            "provider registration failed");
    require(fixture.registry.record_assessment(
                fixture.observation(), fixture.entities),
            "provider observation failed");
    CryptoRegistryError error;
    require(!fixture.registry.record_assessment(
                fixture.qualification(), fixture.entities, &error)
                && error.code == CryptoRegistryErrorCode::policy_conflict,
            "incomplete threat model supported qualification");

    Fixture complete;
    complete.seed_to_qualified();
    EntityRegistry missing_evaluators{
        complete.namespace_id, complete.owner.local_key, 8U, 8U};
    require(missing_evaluators.register_entity(complete.owner),
            "alternate owner registration failed");
    const auto audit = complete.registry.audit_entities(missing_evaluators);
    require(!audit.clean() && audit.missing_evaluators == 2U,
            "external entity audit missed removed evaluators");

    auto other_owner = genesis::identity::make_entity_address(
        "crypto.other",
        EntityKind::organism,
        "owner",
        digest("other owner provenance"),
        1U);
    EntityRegistry wrong_namespace{
        "crypto.other", other_owner.local_key, 8U, 8U};
    require(wrong_namespace.register_entity(other_owner),
            "other namespace entity setup failed");
    require(complete.registry.audit_entities(wrong_namespace).owner_mismatches == 1U,
            "external audit missed namespace/owner mismatch");
}

void rewrite_checksum(std::string& bytes) {
    require(bytes.size() >= 64U, "test snapshot is too small for a checksum");
    const auto payload_size = bytes.size() - 64U;
    bytes.replace(payload_size,
                  64U,
                  digest(std::string_view(bytes.data(), payload_size)));
}

void test_persistence_and_recovery() {
    Fixture fixture;
    fixture.seed_to_qualified(true);
    const auto bytes = CryptoProviderStore::serialize(fixture.registry);
    require(bytes.size() > 1'024U, "security snapshot did not exercise size limits");
    CryptoStoreError error;
    const auto restored = CryptoProviderStore::deserialize(bytes, &error);
    require(restored.has_value() && error.code == CryptoStoreErrorCode::none
                && restored->verify()
                && CryptoProviderStore::serialize(*restored) == bytes,
            "security snapshot exact roundtrip failed");

    auto corrupt = bytes;
    corrupt[corrupt.size() / 2U] ^= 0x01;
    require(!CryptoProviderStore::deserialize(corrupt, &error).has_value()
                && error.code == CryptoStoreErrorCode::corrupt_record,
            "corrupt security snapshot was accepted");

    auto future = bytes;
    const auto schema_offset = std::string_view(
        "GENESIS-CRYPTO-PROVIDER-REGISTRY").size();
    future[schema_offset] = static_cast<char>(2U);
    rewrite_checksum(future);
    require(!CryptoProviderStore::deserialize(future, &error).has_value()
                && error.code == CryptoStoreErrorCode::unsupported_schema,
            "future security schema was accepted");

    auto trailing_payload = bytes.substr(0U, bytes.size() - 64U);
    trailing_payload.push_back('x');
    trailing_payload += digest(trailing_payload);
    require(!CryptoProviderStore::deserialize(trailing_payload, &error).has_value()
                && error.code == CryptoStoreErrorCode::corrupt_record,
            "trailing security snapshot data was accepted");

    const auto root = std::filesystem::temp_directory_path()
                      / "genesis-crypto-provider-tests";
    std::error_code filesystem_error;
    std::filesystem::remove_all(root, filesystem_error);
    CryptoProviderStore store(root);
    require(store.write(fixture.registry, "1.0.0", &error)
                && store.write(fixture.registry, "1.0.0", &error),
            "immutable security snapshot write was not idempotent");
    const auto disk = store.read(fixture.registry.registry_id(),
                                 fixture.namespace_id,
                                 fixture.owner.entity_id,
                                 "1.0.0",
                                 &error);
    require(disk.has_value() && CryptoProviderStore::serialize(*disk) == bytes,
            "immutable security snapshot recovery failed");

    auto changed = *disk;
    auto revision = policy_draft(60U, 60U, 900U);
    revision.change_evidence_digest = digest("conflicting policy revision");
    require(changed.append_policy(revision, fixture.entities),
            "conflict fixture mutation failed");
    require(!store.write(changed, "1.0.0", &error)
                && error.code == CryptoStoreErrorCode::conflicting_version,
            "conflicting immutable security version was overwritten");
    require(!store.read(fixture.registry.registry_id(),
                        fixture.namespace_id,
                        fixture.owner.entity_id,
                        "../unsafe",
                        &error)
                .has_value()
                && error.code == CryptoStoreErrorCode::invalid_identifier,
            "unsafe security snapshot version was accepted");
    require(!store.read(fixture.registry.registry_id(),
                        fixture.namespace_id,
                        fixture.owner.entity_id,
                        "missing",
                        &error)
                .has_value()
                && error.code == CryptoStoreErrorCode::not_found,
            "missing security snapshot was not reported");

    CryptoProviderStore bounded(root / "bounded", 1'024U);
    require(!bounded.write(fixture.registry, "1.0.0", &error)
                && error.code == CryptoStoreErrorCode::invalid_registry,
            "oversized security snapshot bypassed the write limit");

    const auto file_type_root = root / "file-type";
    const auto target = file_type_root
                        / (fixture.registry.registry_id() + ".1.0.0.crypto");
    std::filesystem::create_directories(target, filesystem_error);
    require(!filesystem_error, "could not create unsafe file-type fixture");
    CryptoProviderStore file_type_store(file_type_root);
    require(!file_type_store.read(fixture.registry.registry_id(),
                                  fixture.namespace_id,
                                  fixture.owner.entity_id,
                                  "1.0.0",
                                  &error)
                .has_value()
                && error.code == CryptoStoreErrorCode::corrupt_record,
            "non-regular security snapshot target was accepted");

    const auto binding_root = root / "binding";
    const auto other_owner = genesis::identity::derive_entity_id(
        fixture.namespace_id, EntityKind::organization, "other-owner");
    const auto other_registry_id = genesis::security::derive_crypto_registry_id(
        fixture.namespace_id, other_owner);
    genesis::storage::ImmutableSnapshotFiles binding_files(binding_root);
    genesis::storage::ImmutableFileError file_error;
    require(binding_files.write(other_registry_id,
                                "1.0.0",
                                "crypto",
                                bytes,
                                &file_error),
            "binding mismatch fixture write failed");
    CryptoProviderStore binding_store(binding_root);
    require(!binding_store.read(other_registry_id,
                                fixture.namespace_id,
                                other_owner,
                                "1.0.0",
                                &error)
                .has_value()
                && error.code == CryptoStoreErrorCode::corrupt_record,
            "security snapshot owner binding mismatch was accepted");

    const auto race_root = root / "race";
    CryptoProviderStore race_store(race_root);
    std::atomic<std::size_t> successes{0U};
    std::vector<std::thread> writers;
    for (std::size_t index = 0U; index < 6U; ++index) {
        writers.emplace_back([&] {
            CryptoStoreError thread_error;
            if (race_store.write(fixture.registry, "1.0.0", &thread_error)) {
                ++successes;
            }
        });
    }
    for (auto& writer : writers) {
        writer.join();
    }
    require(successes == writers.size(),
            "concurrent identical security snapshot writers diverged");
    const auto raced = race_store.read(fixture.registry.registry_id(),
                                       fixture.namespace_id,
                                       fixture.owner.entity_id,
                                       "1.0.0",
                                       &error);
    require(raced.has_value() && CryptoProviderStore::serialize(*raced) == bytes,
            "concurrent security snapshot recovery diverged");
    std::filesystem::remove_all(root, filesystem_error);
}

std::vector<std::string> split_tabs(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t begin = 0U;
    while (begin <= line.size()) {
        const auto end = line.find('\t', begin);
        fields.push_back(line.substr(begin,
                                    end == std::string::npos ? std::string::npos
                                                             : end - begin));
        if (end == std::string::npos) {
            break;
        }
        begin = end + 1U;
    }
    return fields;
}

void test_reference_baseline(const std::filesystem::path& path) {
    std::ifstream input(path);
    require(input.good(), "could not open cryptographic reference baseline");
    std::string line;
    require(static_cast<bool>(std::getline(input, line)),
            "cryptographic reference baseline is empty");
    require(line
                == "reference_id\ttitle\tpublisher\tstatus\tretrieved_at\turl\tusage_boundary",
            "cryptographic reference baseline header drifted");
    const std::set<std::string> allowed_statuses{
        "FINAL",
        "LIVE_PROGRAM",
        "DRAFT_MONITOR_ONLY",
        "CURRENT_GUIDANCE",
        "PLATFORM_DOCUMENTATION",
        "PLATFORM_API_DOCUMENTATION"};
    std::set<std::string> ids;
    std::size_t count = 0U;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const auto fields = split_tabs(line);
        require(fields.size() == 7U, "cryptographic reference row shape invalid");
        require(ids.insert(fields[0]).second,
                "duplicate cryptographic reference ID");
        require(!fields[1].empty() && !fields[2].empty(),
                "cryptographic reference metadata missing");
        require(allowed_statuses.contains(fields[3]),
                "cryptographic reference status is not evidence-gated");
        require(fields[4] == "2026-09-04" || fields[4] == "2026-09-05",
                "cryptographic reference retrieval date drifted");
        require(fields[5].starts_with("https://csrc.nist.gov/")
                    || fields[5].starts_with("https://www.ncsc.gov.uk/")
                    || fields[5].starts_with("https://learn.microsoft.com/"),
                "cryptographic reference is not an approved primary-source domain");
        require(fields[6] == "REFERENCE_ONLY_NO_AUTO_APPROVAL",
                "external reference was granted automatic approval authority");
        ++count;
    }
    require(count >= 10U, "cryptographic reference baseline is incomplete");
}

} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "security tests require the reference baseline path");
        test_enum_codecs_and_identity_derivation();
        test_threat_policy_and_provider_validation();
        test_assessment_gates_and_no_authority();
        test_policy_agility_and_provider_lifecycle();
        test_incomplete_threat_model_and_external_audit();
        test_persistence_and_recovery();
        test_reference_baseline(argv[1]);
        std::cout << "security tests passed\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "security tests failed: " << exception.what() << '\n';
        return 1;
    }
}
