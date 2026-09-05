#include "genesis/genesis.hpp"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

using genesis::identity::EntityAddress;
using genesis::identity::EntityKind;
using genesis::identity::EntityRegistry;
using namespace genesis::security;

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

struct Fixture final {
    std::string namespace_id{"key.test"};
    EntityAddress owner = genesis::identity::make_entity_address(
        namespace_id, EntityKind::organism, "owner", digest("owner provenance"), 1U);
    EntityAddress custodian = genesis::identity::make_entity_address(
        namespace_id, EntityKind::person, "custodian", digest("custodian provenance"), 2U);
    EntityAddress recovery = genesis::identity::make_entity_address(
        namespace_id,
        EntityKind::organization,
        "recovery-authority",
        digest("recovery provenance"),
        2U);
    EntityAddress operator_entity = genesis::identity::make_entity_address(
        namespace_id, EntityKind::service, "operator", digest("operator provenance"), 2U);
    EntityAddress observer = genesis::identity::make_entity_address(
        namespace_id, EntityKind::person, "observer", digest("observer provenance"), 2U);
    EntityAddress qualifier = genesis::identity::make_entity_address(
        namespace_id,
        EntityKind::organization,
        "independent-qualifier",
        digest("qualifier provenance"),
        2U);
    EntityAddress outsider = genesis::identity::make_entity_address(
        namespace_id, EntityKind::person, "outsider", digest("outsider provenance"), 2U);
    EntityRegistry entities{namespace_id, owner.local_key, 64U, 64U};
    CryptoProviderRegistry providers{
        genesis::security::derive_crypto_registry_id(namespace_id, owner.entity_id),
        namespace_id,
        owner.entity_id,
        16U,
        16U,
        16U,
        64U};
    KeyCustodyRegistry keys{
        derive_key_custody_registry_id(namespace_id, owner.entity_id),
        namespace_id,
        owner.entity_id,
        64U,
        256U,
        64U};
    std::string custody_policy_digest{digest("test custody policy v1")};

    Fixture() {
        for (const auto* entity : {&owner,
                                   &custodian,
                                   &recovery,
                                   &operator_entity,
                                   &observer,
                                   &qualifier,
                                   &outsider}) {
            require(entities.register_entity(*entity), "entity registration failed");
        }
        require(entities.verify(), "entity fixture invalid");
    }

    CryptoProviderManifest provider_manifest() const {
        CryptoProviderManifest manifest;
        manifest.implementation_name = "Synthetic Key Test Provider";
        manifest.implementation_version = "1.0.0";
        manifest.platform_id = "test-platform-x64";
        manifest.module_boundary_digest = digest("test module boundary");
        manifest.module_binary_digest = digest("test module binary");
        manifest.source_provenance_digest = digest("test source provenance");
        manifest.license_evidence_digest = digest("test license evidence");
        manifest.build_evidence_digest = digest("test build evidence");
        manifest.capabilities = {{"sign-synthetic-v1",
                                  CryptoFunction::digital_signature,
                                  "synthetic-sign-route",
                                  digest("test sign implementation")}};
        manifest.declared_at = 30U;
        manifest.provider_id = derive_crypto_provider_id(manifest.implementation_name,
                                                         manifest.implementation_version,
                                                         manifest.platform_id,
                                                         manifest.module_binary_digest);
        return manifest;
    }

    CryptoRouteEvidence route(bool qualified,
                              std::string custody_digest = {}) const {
        CryptoRouteEvidence value;
        value.algorithm_id = "sign-synthetic-v1";
        value.function = CryptoFunction::digital_signature;
        value.implementation_route = "synthetic-sign-route";
        value.implementation_digest = digest("test sign implementation");
        value.functional_test_digest = digest("test sign functional evidence");
        value.platform_evidence_digest = digest("test platform evidence");
        if (qualified) {
            value.algorithm_validation_reference = "TEST-ONLY-ALGORITHM-VALIDATION";
            value.algorithm_validation_evidence_digest =
                digest("test algorithm validation evidence");
            value.key_custody_evidence_digest = std::move(custody_digest);
        }
        return value;
    }

    void seed_provider(bool qualify = true,
                       std::string qualification_custody_digest = {}) {
        CryptoThreatModelDraft threat;
        threat.id = "threat-main";
        threat.threats = qualification_threats();
        threat.asset_inventory_digest = digest("test asset inventory");
        threat.trust_boundaries_digest = digest("test trust boundaries");
        threat.mitigation_plan_digest = digest("test mitigation plan");
        threat.residual_risk_digest = digest("test residual risk");
        threat.evidence_digest = digest("test threat evidence");
        threat.effective_from = 10U;
        threat.review_at = 1'000U;
        threat.recorded_at = 10U;
        require(providers.append_threat_model(std::move(threat), entities),
                "threat append failed");

        CryptoAlgorithmRule rule;
        rule.algorithm_id = "sign-synthetic-v1";
        rule.function = CryptoFunction::digital_signature;
        rule.quantum_readiness = QuantumReadiness::hybrid;
        rule.disposition = AlgorithmDisposition::approved;
        rule.security_strength_bits = 128U;
        rule.standard_reference = "TEST-ONLY-SYNTHETIC-POLICY";
        rule.standard_evidence_digest = digest("test policy source");
        CryptoPolicyDraft policy;
        policy.id = "policy-main";
        policy.threat_model_id = "threat-main";
        policy.threat_model_revision = 1U;
        policy.source_set_digest = digest("test source set");
        policy.change_evidence_digest = digest("test policy change");
        policy.rules = {rule};
        policy.effective_from = 20U;
        policy.review_at = 900U;
        policy.recorded_at = 20U;
        require(providers.append_policy(std::move(policy), entities),
                "policy append failed");

        require(providers.register_provider(provider_manifest(), entities),
                "provider registration failed");
        CryptoProviderAssessmentDraft observation;
        observation.id = "observation-40";
        observation.provider_id = provider_manifest().provider_id;
        observation.kind = ProviderAssessmentKind::observation;
        observation.policy_id = "policy-main";
        observation.policy_revision = 1U;
        observation.evaluator_entity_id = observer.entity_id;
        observation.evaluation_evidence_digest = digest("observation evidence");
        observation.routes = {route(false)};
        observation.observed_at = 39U;
        observation.recorded_at = 40U;
        require(providers.record_assessment(std::move(observation), entities),
                "provider observation failed");
        if (!qualify) {
            return;
        }
        if (qualification_custody_digest.empty()) {
            qualification_custody_digest = custody_policy_digest;
        }
        CryptoProviderAssessmentDraft qualification;
        qualification.id = "qualification-50";
        qualification.provider_id = provider_manifest().provider_id;
        qualification.kind = ProviderAssessmentKind::qualification;
        qualification.policy_id = "policy-main";
        qualification.policy_revision = 1U;
        qualification.evaluator_entity_id = qualifier.entity_id;
        qualification.evaluation_evidence_digest = digest("qualification evidence");
        qualification.module_validation_reference = "TEST-ONLY-MODULE-VALIDATION";
        qualification.module_validation_evidence_digest =
            digest("test module validation evidence");
        qualification.routes = {route(true, std::move(qualification_custody_digest))};
        qualification.observed_at = 49U;
        qualification.recorded_at = 50U;
        qualification.valid_until = 800U;
        require(providers.record_assessment(std::move(qualification), entities),
                "provider qualification failed");
    }

    KeyHandleManifest manifest(std::string locator_material = "provider locator one",
                               std::uint64_t generation = 1U,
                               std::string predecessor = {},
                               KeyOrigin origin = KeyOrigin::generated,
                               std::uint64_t registered_at = 60U) const {
        KeyHandleManifest value;
        value.owner_entity_id = owner.entity_id;
        value.provider_id = provider_manifest().provider_id;
        value.algorithm_id = "sign-synthetic-v1";
        value.function = CryptoFunction::digital_signature;
        value.implementation_route = "synthetic-sign-route";
        value.implementation_digest = digest("test sign implementation");
        value.provider_locator_digest = digest(locator_material);
        value.origin = origin;
        value.export_policy = KeyExportPolicy::non_exportable;
        value.permitted_usages = {KeyUsage::sign, KeyUsage::verify};
        value.custodian_entity_id = custodian.entity_id;
        value.recovery_authority_entity_id = recovery.entity_id;
        value.operator_entity_ids = {custodian.entity_id, operator_entity.entity_id};
        std::sort(value.operator_entity_ids.begin(), value.operator_entity_ids.end());
        value.custody_policy_digest = custody_policy_digest;
        value.creation_evidence_digest = digest("test creation evidence " + locator_material);
        value.attestation_evidence_digest =
            digest("test attestation evidence " + locator_material);
        value.predecessor_key_id = std::move(predecessor);
        value.generation = generation;
        value.not_before = 60U;
        value.not_after = 700U;
        value.registered_at = registered_at;
        value.key_id = derive_key_handle_id(value.owner_entity_id,
                                            value.provider_id,
                                            value.algorithm_id,
                                            value.function,
                                            value.implementation_route,
                                            value.implementation_digest,
                                            value.provider_locator_digest,
                                            value.generation);
        return value;
    }

    KeyTransitionDraft transition(std::string key_id,
                                   KeyTransitionKind kind,
                                   std::uint64_t at,
                                   std::string actor = {}) const {
        KeyTransitionDraft value;
        value.id = "transition-" + std::to_string(at) + '-'
                   + std::string(to_string(kind)) + '-'
                   + digest(key_id).substr(0U, 8U);
        value.key_id = std::move(key_id);
        value.kind = kind;
        value.actor_entity_id = actor.empty() ? custodian.entity_id : std::move(actor);
        value.reason_digest = digest("test transition reason " + std::to_string(at));
        value.evidence_digest = digest("test transition evidence " + std::to_string(at));
        value.occurred_at = at;
        value.recorded_at = at;
        return value;
    }

    void activate(const std::string& key_id, std::uint64_t at = 70U) {
        require(keys.record_transition(transition(key_id, KeyTransitionKind::activate, at),
                                       providers,
                                       entities),
                "key activation failed");
    }
};

void test_enum_and_identity_boundaries() {
    KeyOrigin origin{};
    KeyExportPolicy export_policy{};
    KeyUsage usage{};
    KeyLifecycleState state{};
    KeyTransitionKind transition_kind{};
    KeySuccessionKind succession_kind{};
    require(key_origin_from_string("recovered", origin)
                && origin == KeyOrigin::recovered,
            "key origin codec failed");
    require(key_export_policy_from_string("public_only", export_policy)
                && export_policy == KeyExportPolicy::public_only,
            "export policy codec failed");
    require(key_usage_from_string("unwrap", usage) && usage == KeyUsage::unwrap,
            "key usage codec failed");
    require(key_lifecycle_state_from_string("destroyed", state)
                && state == KeyLifecycleState::destroyed,
            "key state codec failed");
    require(key_transition_kind_from_string("compromise", transition_kind)
                && transition_kind == KeyTransitionKind::compromise,
            "transition codec failed");
    require(key_succession_kind_from_string("rotation", succession_kind)
                && succession_kind == KeySuccessionKind::rotation,
            "succession codec failed");
    require(!key_usage_from_string("execute", usage), "unknown usage was accepted");
    require(derive_key_handle_id("owner|provider",
                                 "provider",
                                 "algorithm",
                                 CryptoFunction::digital_signature,
                                 "route",
                                 digest("implementation"),
                                 digest("locator"),
                                 1U)
                != derive_key_handle_id("owner",
                                        "provider|provider",
                                        "algorithm",
                                        CryptoFunction::digital_signature,
                                        "route",
                                        digest("implementation"),
                                        digest("locator"),
                                        1U),
            "length-ambiguous key identity material collided");

    Fixture fixture;
    fixture.seed_provider();
    auto manifest = fixture.manifest();
    require(manifest.key_id
                == derive_key_handle_id(manifest.owner_entity_id,
                                        manifest.provider_id,
                                        manifest.algorithm_id,
                                        manifest.function,
                                        manifest.implementation_route,
                                        manifest.implementation_digest,
                                        manifest.provider_locator_digest,
                                        manifest.generation),
            "derived key identifier is unstable");
    require(fixture.keys.register_key(manifest, fixture.providers, fixture.entities),
            "valid key registration failed");
    const auto key_count = fixture.keys.keys().size();
    KeyCustodyError error;
    require(!fixture.keys.register_key(manifest,
                                       fixture.providers,
                                       fixture.entities,
                                       &error)
                && error.code == KeyCustodyErrorCode::duplicate_entry
                && fixture.keys.keys().size() == key_count,
            "duplicate key registration mutated the registry");

    auto malformed = fixture.manifest("locator malformed");
    malformed.provider_locator_digest = "plaintext-provider-locator";
    malformed.key_id = "key-invalid";
    require(!fixture.keys.register_key(std::move(malformed),
                                       fixture.providers,
                                       fixture.entities,
                                       &error)
                && error.code == KeyCustodyErrorCode::invalid_manifest,
            "plaintext locator-shaped metadata was accepted");

    auto same_roles = fixture.manifest("locator same roles");
    same_roles.recovery_authority_entity_id = same_roles.custodian_entity_id;
    require(!fixture.keys.register_key(std::move(same_roles),
                                       fixture.providers,
                                       fixture.entities,
                                       &error)
                && error.code == KeyCustodyErrorCode::invalid_manifest,
            "custody and recovery role separation was not enforced");

    auto wrong_route = fixture.manifest("locator wrong route");
    wrong_route.implementation_route = "unqualified-route";
    wrong_route.key_id = derive_key_handle_id(wrong_route.owner_entity_id,
                                              wrong_route.provider_id,
                                              wrong_route.algorithm_id,
                                              wrong_route.function,
                                              wrong_route.implementation_route,
                                              wrong_route.implementation_digest,
                                              wrong_route.provider_locator_digest,
                                              wrong_route.generation);
    require(!fixture.keys.register_key(std::move(wrong_route),
                                       fixture.providers,
                                       fixture.entities,
                                       &error)
                && error.code == KeyCustodyErrorCode::provider_binding_mismatch,
            "undeclared provider route was accepted");
    require(fixture.keys.verify(), "key registry invalid after rejected manifests");
}

void test_provider_gate_lifecycle_and_preflight() {
    Fixture unqualified;
    unqualified.seed_provider(false);
    auto unavailable_key = unqualified.manifest();
    require(unqualified.keys.register_key(unavailable_key,
                                          unqualified.providers,
                                          unqualified.entities),
            "unqualified provider key metadata registration failed");
    KeyCustodyError error;
    const auto before = unqualified.keys.transition_count();
    require(!unqualified.keys.record_transition(
                unqualified.transition(unavailable_key.key_id,
                                       KeyTransitionKind::activate,
                                       70U),
                unqualified.providers,
                unqualified.entities,
                &error)
                && error.code == KeyCustodyErrorCode::provider_not_qualified
                && unqualified.keys.transition_count() == before,
            "unqualified provider activated a key or mutated history");

    Fixture mismatch;
    mismatch.seed_provider(true, digest("different custody policy"));
    auto mismatch_key = mismatch.manifest();
    require(mismatch.keys.register_key(mismatch_key,
                                       mismatch.providers,
                                       mismatch.entities),
            "mismatch fixture key registration failed");
    require(!mismatch.keys.record_transition(
                mismatch.transition(mismatch_key.key_id, KeyTransitionKind::activate, 70U),
                mismatch.providers,
                mismatch.entities,
                &error)
                && error.code == KeyCustodyErrorCode::custody_policy_mismatch,
            "mismatched custody policy activated a key");

    Fixture fixture;
    fixture.seed_provider();
    const auto key = fixture.manifest();
    require(fixture.keys.register_key(key, fixture.providers, fixture.entities),
            "key registration failed");
    fixture.activate(key.key_id);
    require(fixture.keys.key_state(key.key_id) == KeyLifecycleState::active,
            "activation state missing");

    KeyOperationRequest request{key.key_id,
                                KeyUsage::sign,
                                fixture.operator_entity.entity_id,
                                digest("test operation context"),
                                80U};
    const auto allowed =
        fixture.keys.preflight(request, fixture.providers, fixture.entities);
    require(allowed.execution_candidate && allowed.registry_valid
                && allowed.entity_registry_valid && allowed.owner_bound
                && allowed.actor_recorded && allowed.actor_role_matched
                && allowed.active && allowed.within_cryptoperiod
                && allowed.usage_permitted && allowed.provider_registry_valid
                && allowed.provider_integration_candidate
                && allowed.provider_route_bound && allowed.custody_policy_bound
                && allowed.operation_context_recorded
                && !allowed.cryptographic_operation_executed
                && !allowed.identity_authenticated
                && !allowed.provenance_authenticated && !allowed.action_authorized,
            "valid preflight did not remain an evidence-only execution candidate");

    auto denied = request;
    denied.actor_entity_id = fixture.outsider.entity_id;
    require(!fixture.keys.preflight(denied, fixture.providers, fixture.entities)
                 .execution_candidate,
            "unrecorded operator role passed preflight");
    denied = request;
    denied.operation_context_digest.clear();
    require(fixture.keys.preflight(denied, fixture.providers, fixture.entities).reason
                == "operation_context_digest_missing",
            "missing operation context was not denied");

    const auto transitions_before = fixture.keys.transition_count();
    require(!fixture.keys.record_transition(
                fixture.transition(key.key_id, KeyTransitionKind::resume, 81U),
                fixture.providers,
                fixture.entities,
                &error)
                && error.code == KeyCustodyErrorCode::transition_conflict
                && fixture.keys.transition_count() == transitions_before,
            "invalid resume mutated an active key");
    require(fixture.keys.record_transition(
                fixture.transition(key.key_id,
                                   KeyTransitionKind::suspend,
                                   90U,
                                   fixture.operator_entity.entity_id),
                fixture.providers,
                fixture.entities),
            "operator suspension failed");
    request.requested_at = 91U;
    require(fixture.keys.preflight(request, fixture.providers, fixture.entities).reason
                == "key_not_active",
            "suspended key passed preflight");
    require(fixture.keys.record_transition(
                fixture.transition(key.key_id, KeyTransitionKind::resume, 100U),
                fixture.providers,
                fixture.entities),
            "key resume failed");
    require(fixture.keys.record_transition(
                fixture.transition(key.key_id, KeyTransitionKind::retire, 110U),
                fixture.providers,
                fixture.entities),
            "key retirement failed");
    require(fixture.keys.record_transition(
                fixture.transition(key.key_id, KeyTransitionKind::destroy, 120U),
                fixture.providers,
                fixture.entities),
            "key destruction failed");
    require(fixture.keys.key_state(key.key_id) == KeyLifecycleState::destroyed,
            "destroyed state missing");
    require(!fixture.keys.record_transition(
                fixture.transition(key.key_id, KeyTransitionKind::activate, 130U),
                fixture.providers,
                fixture.entities),
            "destroyed key was revived");
    require(fixture.keys.verify() && fixture.keys.audit_entities(fixture.entities).clean(),
            "lifecycle registry audit failed");

    Fixture provider_gate;
    provider_gate.seed_provider();
    const auto provider_key = provider_gate.manifest("provider suspension key");
    require(provider_gate.keys.register_key(provider_key,
                                            provider_gate.providers,
                                            provider_gate.entities),
            "provider suspension key registration failed");
    provider_gate.activate(provider_key.key_id, 70U);
    CryptoProviderAssessmentDraft suspension;
    suspension.id = "provider-suspension-90";
    suspension.provider_id = provider_gate.provider_manifest().provider_id;
    suspension.kind = ProviderAssessmentKind::suspension;
    suspension.evaluator_entity_id = provider_gate.observer.entity_id;
    suspension.evaluation_evidence_digest = digest("provider suspension evidence");
    suspension.observed_at = 90U;
    suspension.recorded_at = 90U;
    require(provider_gate.providers.record_assessment(std::move(suspension),
                                                      provider_gate.entities),
            "provider suspension record failed");
    KeyOperationRequest provider_request{provider_key.key_id,
                                         KeyUsage::sign,
                                         provider_gate.custodian.entity_id,
                                         digest("provider-gate operation"),
                                         91U};
    require(provider_gate.keys.preflight(provider_request,
                                         provider_gate.providers,
                                         provider_gate.entities)
                .reason
                == "provider_not_an_integration_candidate",
            "suspended provider remained usable through key preflight");
}

void test_rotation_and_recovery_lineage() {
    Fixture rotation;
    rotation.seed_provider();
    const auto predecessor = rotation.manifest("rotation predecessor");
    require(rotation.keys.register_key(predecessor,
                                       rotation.providers,
                                       rotation.entities),
            "rotation predecessor registration failed");
    rotation.activate(predecessor.key_id, 70U);
    require(rotation.keys.record_transition(
                rotation.transition(predecessor.key_id, KeyTransitionKind::retire, 90U),
                rotation.providers,
                rotation.entities),
            "rotation predecessor retirement failed");
    auto reused_locator = rotation.manifest("rotation predecessor",
                                            2U,
                                            predecessor.key_id,
                                            KeyOrigin::generated,
                                            95U);
    KeyCustodyError error;
    require(!rotation.keys.register_key(std::move(reused_locator),
                                        rotation.providers,
                                        rotation.entities,
                                        &error)
                && error.code == KeyCustodyErrorCode::succession_conflict,
            "successor reused its predecessor's external locator binding");
    const auto successor = rotation.manifest("rotation successor",
                                             2U,
                                             predecessor.key_id,
                                             KeyOrigin::generated,
                                             100U);
    require(rotation.keys.register_key(successor, rotation.providers, rotation.entities),
            "rotation successor registration failed");
    rotation.activate(successor.key_id, 110U);
    KeySuccessionDraft rotation_record;
    rotation_record.id = "rotation-link-1";
    rotation_record.kind = KeySuccessionKind::rotation;
    rotation_record.predecessor_key_id = predecessor.key_id;
    rotation_record.successor_key_id = successor.key_id;
    rotation_record.actor_entity_id = rotation.custodian.entity_id;
    rotation_record.reason_digest = digest("scheduled rotation reason");
    rotation_record.evidence_digest = digest("scheduled rotation evidence");
    rotation_record.occurred_at = 111U;
    rotation_record.recorded_at = 111U;
    require(rotation.keys.record_succession(rotation_record, rotation.entities),
            "rotation lineage append failed");
    require(rotation.keys.succession_from(predecessor.key_id) != nullptr
                && rotation.keys.succession_to(successor.key_id) != nullptr,
            "rotation lineage indexes missing");
    require(!rotation.keys.record_succession(rotation_record,
                                             rotation.entities,
                                             &error)
                && error.code == KeyCustodyErrorCode::duplicate_entry,
            "duplicate rotation link was accepted");

    Fixture recovery;
    recovery.seed_provider();
    const auto compromised = recovery.manifest("recovery predecessor");
    require(recovery.keys.register_key(compromised,
                                       recovery.providers,
                                       recovery.entities),
            "recovery predecessor registration failed");
    recovery.activate(compromised.key_id, 70U);
    require(recovery.keys.record_transition(
                recovery.transition(compromised.key_id,
                                    KeyTransitionKind::compromise,
                                    90U,
                                    recovery.recovery.entity_id),
                recovery.providers,
                recovery.entities),
            "compromise record failed");
    const auto recovered = recovery.manifest("recovered external locator",
                                             2U,
                                             compromised.key_id,
                                             KeyOrigin::recovered,
                                             100U);
    require(recovery.keys.register_key(recovered, recovery.providers, recovery.entities),
            "recovered successor registration failed");
    recovery.activate(recovered.key_id, 110U);
    KeySuccessionDraft recovery_record;
    recovery_record.id = "recovery-link-1";
    recovery_record.kind = KeySuccessionKind::recovery;
    recovery_record.predecessor_key_id = compromised.key_id;
    recovery_record.successor_key_id = recovered.key_id;
    recovery_record.actor_entity_id = recovery.recovery.entity_id;
    recovery_record.reason_digest = digest("compromise recovery reason");
    recovery_record.evidence_digest = digest("compromise recovery evidence");
    recovery_record.occurred_at = 111U;
    recovery_record.recorded_at = 111U;
    require(recovery.keys.record_succession(recovery_record, recovery.entities),
            "recovery lineage append failed");
    KeyOperationRequest old_request{compromised.key_id,
                                    KeyUsage::sign,
                                    recovery.custodian.entity_id,
                                    digest("old-key operation"),
                                    120U};
    require(recovery.keys.preflight(old_request,
                                    recovery.providers,
                                    recovery.entities)
                .reason
                == "key_not_active",
            "compromised predecessor passed preflight");
    require(rotation.keys.verify() && recovery.keys.verify(),
            "rotation or recovery registry failed verification");

    Fixture false_recovery;
    false_recovery.seed_provider();
    const auto merely_retired = false_recovery.manifest("false recovery predecessor");
    require(false_recovery.keys.register_key(merely_retired,
                                             false_recovery.providers,
                                             false_recovery.entities),
            "false recovery predecessor registration failed");
    false_recovery.activate(merely_retired.key_id, 70U);
    require(false_recovery.keys.record_transition(
                false_recovery.transition(merely_retired.key_id,
                                          KeyTransitionKind::retire,
                                          90U),
                false_recovery.providers,
                false_recovery.entities),
            "false recovery predecessor retirement failed");
    const auto false_successor = false_recovery.manifest("false recovery successor",
                                                         2U,
                                                         merely_retired.key_id,
                                                         KeyOrigin::recovered,
                                                         100U);
    require(false_recovery.keys.register_key(false_successor,
                                             false_recovery.providers,
                                             false_recovery.entities),
            "false recovery successor registration failed");
    false_recovery.activate(false_successor.key_id, 110U);
    KeySuccessionDraft unsupported_recovery;
    unsupported_recovery.id = "unsupported-recovery-link";
    unsupported_recovery.kind = KeySuccessionKind::recovery;
    unsupported_recovery.predecessor_key_id = merely_retired.key_id;
    unsupported_recovery.successor_key_id = false_successor.key_id;
    unsupported_recovery.actor_entity_id = false_recovery.recovery.entity_id;
    unsupported_recovery.reason_digest = digest("unsupported recovery reason");
    unsupported_recovery.evidence_digest = digest("unsupported recovery evidence");
    unsupported_recovery.occurred_at = 111U;
    unsupported_recovery.recorded_at = 111U;
    require(!false_recovery.keys.record_succession(std::move(unsupported_recovery),
                                                   false_recovery.entities,
                                                   &error),
            "recovery lineage was accepted without a compromise event");
}

std::filesystem::path test_root(std::string_view name) {
    return std::filesystem::temp_directory_path()
           / ("genesis-key-custody-" + std::string(name) + '-'
              + digest(name).substr(0U, 12U));
}

void test_persistence_and_recovery() {
    Fixture fixture;
    fixture.seed_provider();
    const auto key = fixture.manifest("provider locator must never persist");
    require(fixture.keys.register_key(key, fixture.providers, fixture.entities),
            "persistence key registration failed");
    fixture.activate(key.key_id);
    const auto bytes = KeyCustodyStore::serialize(fixture.keys);
    require(bytes.find("provider locator must never persist") == std::string::npos,
            "plaintext external provider locator entered the snapshot");
    require(bytes.find("private key") == std::string::npos,
            "key-material-shaped test marker entered the snapshot");
    KeyCustodyStoreError error;
    auto restored = KeyCustodyStore::deserialize(bytes, &error);
    require(restored.has_value() && restored->verify()
                && KeyCustodyStore::serialize(*restored) == bytes,
            "key-custody canonical roundtrip failed");
    auto corrupt = bytes;
    corrupt[corrupt.size() / 2U] ^= 0x01;
    require(!KeyCustodyStore::deserialize(corrupt, &error).has_value()
                && error.code == KeyCustodyStoreErrorCode::corrupt_record,
            "corrupt snapshot was accepted");

    auto future_schema = bytes;
    const auto schema_offset = std::string_view("GENESIS-KEY-CUSTODY-REGISTRY").size();
    future_schema[schema_offset] = 2;
    for (std::size_t index = 1U; index < 8U; ++index) {
        future_schema[schema_offset + index] = 0;
    }
    const auto payload_size = future_schema.size() - 64U;
    const auto replacement_checksum =
        digest(std::string_view(future_schema).substr(0U, payload_size));
    future_schema.replace(payload_size, 64U, replacement_checksum);
    require(!KeyCustodyStore::deserialize(future_schema, &error).has_value()
                && error.code == KeyCustodyStoreErrorCode::unsupported_schema,
            "future snapshot schema was accepted");

    const auto root = test_root("persistence");
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    KeyCustodyStore store(root);
    require(store.write(fixture.owner.entity_id, "v1", fixture.keys, &error),
            "immutable key-custody write failed");
    require(store.write(fixture.owner.entity_id, "v1", fixture.keys, &error),
            "idempotent immutable key-custody write failed");
    auto loaded = store.read(fixture.owner.entity_id, "v1", &error);
    require(loaded.has_value() && KeyCustodyStore::serialize(*loaded) == bytes,
            "immutable key-custody read failed");
    require(!store.read(fixture.outsider.entity_id, "v1", &error).has_value()
                && error.code == KeyCustodyStoreErrorCode::not_found,
            "different owner unexpectedly restored a snapshot");
    require(!store.write(fixture.outsider.entity_id,
                         "v2",
                         fixture.keys,
                         &error)
                && error.code == KeyCustodyStoreErrorCode::owner_binding_mismatch,
            "owner-mismatched snapshot write was accepted");
    require(!store.write(fixture.owner.entity_id, "../unsafe", fixture.keys, &error)
                && error.code == KeyCustodyStoreErrorCode::invalid_identifier,
            "path-unsafe snapshot version was accepted");

    Fixture conflict;
    conflict.seed_provider();
    const auto conflict_key = conflict.manifest("conflicting provider locator");
    require(conflict.keys.register_key(conflict_key,
                                       conflict.providers,
                                       conflict.entities),
            "conflict fixture registration failed");
    require(!store.write(fixture.owner.entity_id, "v1", conflict.keys, &error)
                && error.code == KeyCustodyStoreErrorCode::conflicting_version,
            "conflicting immutable snapshot replaced published evidence");

    std::filesystem::remove_all(test_root("tiny"), ignored);
    KeyCustodyStore tiny_store(test_root("tiny"), 1024U);
    require(!tiny_store.write(fixture.owner.entity_id,
                              "v1",
                              fixture.keys,
                              &error),
            "oversized snapshot passed configured limit");

    const auto unsafe_type_root = test_root("unsafe-type");
    std::filesystem::remove_all(unsafe_type_root, ignored);
    std::filesystem::create_directories(
        unsafe_type_root
        / (fixture.owner.entity_id + ".v1.key-custody"));
    KeyCustodyStore unsafe_type_store(unsafe_type_root);
    require(!unsafe_type_store.read(fixture.owner.entity_id, "v1", &error).has_value()
                && error.code == KeyCustodyStoreErrorCode::corrupt_record,
            "non-regular snapshot target was accepted");

    const auto concurrent_root = test_root("concurrent");
    std::filesystem::remove_all(concurrent_root, ignored);
    KeyCustodyStore concurrent_store(concurrent_root);
    std::atomic<unsigned> successes{0U};
    std::vector<std::thread> writers;
    for (unsigned index = 0U; index < 8U; ++index) {
        writers.emplace_back([&] {
            KeyCustodyStoreError thread_error;
            if (concurrent_store.write(fixture.owner.entity_id,
                                       "v1",
                                       fixture.keys,
                                       &thread_error)) {
                successes.fetch_add(1U, std::memory_order_relaxed);
            }
        });
    }
    for (auto& writer : writers) {
        writer.join();
    }
    require(successes.load(std::memory_order_relaxed) == writers.size(),
            "concurrent idempotent writers did not converge");
    require(concurrent_store.read(fixture.owner.entity_id, "v1", &error).has_value(),
            "concurrent immutable publication was not recoverable");
    std::filesystem::remove_all(root, ignored);
    std::filesystem::remove_all(test_root("tiny"), ignored);
    std::filesystem::remove_all(unsafe_type_root, ignored);
    std::filesystem::remove_all(concurrent_root, ignored);
}

} // namespace

int main() {
    try {
        test_enum_and_identity_boundaries();
        test_provider_gate_lifecycle_and_preflight();
        test_rotation_and_recovery_lineage();
        test_persistence_and_recovery();
        std::cout << "key custody tests passed\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "key custody tests failed: " << exception.what() << '\n';
        return 1;
    }
}
