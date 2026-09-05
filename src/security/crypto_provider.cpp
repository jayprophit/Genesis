#include "genesis/security/crypto_provider.hpp"

#include "genesis/runtime/runtime.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace genesis::security {
namespace {

constexpr std::size_t kMaximumRegistryItems = 1'000'000U;
constexpr std::size_t kMaximumIdLength = 256U;
constexpr std::size_t kMaximumReferenceLength = 2048U;
constexpr std::size_t kMaximumAlgorithmsPerRecord = 256U;

constexpr std::array<std::string_view, 8> kFunctionNames{
    "secure_hash",
    "message_authentication",
    "authenticated_encryption",
    "digital_signature",
    "key_establishment",
    "key_derivation",
    "random_generation",
    "key_wrapping",
};

constexpr std::array<std::string_view, 4> kQuantumNames{
    "not_applicable",
    "traditional",
    "post_quantum",
    "hybrid",
};

constexpr std::array<std::string_view, 4> kDispositionNames{
    "candidate",
    "approved",
    "verify_only",
    "prohibited",
};

constexpr std::array<std::string_view, 16> kThreatNames{
    "remote_attacker",
    "local_untrusted_user",
    "supply_chain_compromise",
    "binary_tampering",
    "rollback_attack",
    "replay_attack",
    "key_theft",
    "key_loss",
    "weak_randomness",
    "side_channel",
    "algorithm_break",
    "quantum_cryptanalysis",
    "misconfiguration",
    "compromised_device",
    "malicious_dependency",
    "implementation_flaw",
};

constexpr std::array<std::string_view, 4> kAssessmentKindNames{
    "observation",
    "qualification",
    "suspension",
    "revocation",
};

constexpr std::array<std::string_view, 5> kProviderStateNames{
    "declared",
    "observed",
    "qualified",
    "suspended",
    "revoked",
};

template <typename Enum, std::size_t Size>
bool parse_enum(std::string_view text,
                const std::array<std::string_view, Size>& names,
                Enum& output) noexcept {
    const auto found = std::find(names.begin(), names.end(), text);
    if (found == names.end()) {
        return false;
    }
    output = static_cast<Enum>(std::distance(names.begin(), found));
    return true;
}

template <typename Enum>
bool enum_in_range(Enum value, Enum maximum) noexcept {
    return static_cast<std::uint64_t>(value) <= static_cast<std::uint64_t>(maximum);
}

bool bounded_text(std::string_view value, std::size_t maximum) {
    return !value.empty() && value.size() <= maximum
           && std::all_of(value.begin(), value.end(), [](unsigned char character) {
                  return character >= 0x20U && character != 0x7fU;
              });
}

bool optional_bounded_text(std::string_view value, std::size_t maximum) {
    return value.empty() || bounded_text(value, maximum);
}

bool portable_identifier(std::string_view value, std::size_t maximum) {
    if (!bounded_text(value, maximum) || value == "." || value == "..") {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '_' || character == '-'
               || character == '.';
    });
}

bool digest(std::string_view value) {
    return value.size() == 64U
           && std::all_of(value.begin(), value.end(), [](unsigned char character) {
                  return (character >= '0' && character <= '9')
                         || (character >= 'a' && character <= 'f');
              });
}

bool optional_digest(std::string_view value) {
    return value.empty() || digest(value);
}

void append_material(std::string& output, std::string_view value) {
    output.append(std::to_string(value.size()));
    output.push_back(':');
    output.append(value);
}

void append_optional_time(std::string& output,
                          const std::optional<std::uint64_t>& value) {
    output.push_back(value.has_value() ? '1' : '0');
    append_material(output, std::to_string(value.value_or(0U)));
}

void set_error(CryptoRegistryError* error,
               CryptoRegistryErrorCode code,
               std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->message = std::move(message);
    }
}

bool valid_owner(const CryptoProviderRegistry& registry,
                 const identity::EntityRegistry& entities,
                 std::uint64_t recorded_at) {
    if (!entities.verify() || entities.namespace_id() != registry.entity_namespace_id()) {
        return false;
    }
    const auto* owner = entities.find_entity(registry.owner_entity_id());
    return owner != nullptr
           && (owner->kind == identity::EntityKind::organism
               || owner->kind == identity::EntityKind::organization)
           && recorded_at >= owner->registered_at;
}

bool evaluator_kind_permitted(identity::EntityKind kind) noexcept {
    return kind == identity::EntityKind::organism || kind == identity::EntityKind::person
           || kind == identity::EntityKind::organization;
}

bool threat_sort_less(CryptoThreatCategory left, CryptoThreatCategory right) {
    return static_cast<std::uint8_t>(left) < static_cast<std::uint8_t>(right);
}

bool rule_sort_less(const CryptoAlgorithmRule& left,
                    const CryptoAlgorithmRule& right) {
    return std::tie(left.function, left.algorithm_id)
           < std::tie(right.function, right.algorithm_id);
}

bool capability_sort_less(const CryptoAlgorithmCapability& left,
                          const CryptoAlgorithmCapability& right) {
    return std::tie(left.function, left.algorithm_id)
           < std::tie(right.function, right.algorithm_id);
}

bool route_sort_less(const CryptoRouteEvidence& left,
                     const CryptoRouteEvidence& right) {
    return std::tie(left.function, left.algorithm_id)
           < std::tie(right.function, right.algorithm_id);
}

bool sorted_unique_threats(const std::vector<CryptoThreatCategory>& threats) {
    return std::is_sorted(threats.begin(), threats.end(), threat_sort_less)
           && std::adjacent_find(threats.begin(), threats.end()) == threats.end();
}

template <typename Item, typename Less>
bool sorted_unique_by(const std::vector<Item>& items, Less less) {
    if (!std::is_sorted(items.begin(), items.end(), less)) {
        return false;
    }
    return std::adjacent_find(items.begin(), items.end(), [&](const Item& left,
                                                              const Item& right) {
               return !less(left, right) && !less(right, left);
           }) == items.end();
}

bool valid_threat_draft_shape(const CryptoThreatModelDraft& draft) {
    if (!portable_identifier(draft.id, kMaximumIdLength) || draft.threats.empty()
        || draft.threats.size() > kThreatNames.size()
        || !digest(draft.asset_inventory_digest)
        || !digest(draft.trust_boundaries_digest)
        || !digest(draft.mitigation_plan_digest) || !digest(draft.residual_risk_digest)
        || !digest(draft.evidence_digest) || draft.review_at <= draft.effective_from
        || draft.review_at <= draft.recorded_at) {
        return false;
    }
    return std::all_of(draft.threats.begin(), draft.threats.end(), [](const auto threat) {
        return enum_in_range(threat, CryptoThreatCategory::implementation_flaw);
    });
}

CryptoThreatModelDraft draft_from_threat(const CryptoThreatModelRevision& revision) {
    return {revision.id,
            revision.threats,
            revision.asset_inventory_digest,
            revision.trust_boundaries_digest,
            revision.mitigation_plan_digest,
            revision.residual_risk_digest,
            revision.evidence_digest,
            revision.effective_from,
            revision.review_at,
            revision.recorded_at};
}

std::string threat_revision_digest(std::string_view registry_id,
                                   const CryptoThreatModelRevision& revision) {
    std::string material{"genesis.crypto.threat-model.v1"};
    append_material(material, registry_id);
    append_material(material, revision.id);
    append_material(material, std::to_string(revision.revision));
    append_material(material, std::to_string(revision.threats.size()));
    for (const auto threat : revision.threats) {
        append_material(material, to_string(threat));
    }
    append_material(material, revision.asset_inventory_digest);
    append_material(material, revision.trust_boundaries_digest);
    append_material(material, revision.mitigation_plan_digest);
    append_material(material, revision.residual_risk_digest);
    append_material(material, revision.evidence_digest);
    append_material(material, std::to_string(revision.effective_from));
    append_material(material, std::to_string(revision.review_at));
    append_material(material, std::to_string(revision.recorded_at));
    append_material(material, revision.previous_revision_digest);
    return runtime::sha256(material);
}

bool threat_qualification_ready(const CryptoThreatModelRevision& revision) {
    constexpr std::array<CryptoThreatCategory, 7> required{
        CryptoThreatCategory::supply_chain_compromise,
        CryptoThreatCategory::binary_tampering,
        CryptoThreatCategory::rollback_attack,
        CryptoThreatCategory::key_theft,
        CryptoThreatCategory::weak_randomness,
        CryptoThreatCategory::algorithm_break,
        CryptoThreatCategory::quantum_cryptanalysis,
    };
    return std::all_of(required.begin(), required.end(), [&](const auto threat) {
        return std::binary_search(
            revision.threats.begin(), revision.threats.end(), threat, threat_sort_less);
    });
}

bool valid_rule_shape(const CryptoAlgorithmRule& rule) {
    if (!portable_identifier(rule.algorithm_id, kMaximumIdLength)
        || !enum_in_range(rule.function, CryptoFunction::key_wrapping)
        || !enum_in_range(rule.quantum_readiness, QuantumReadiness::hybrid)
        || !enum_in_range(rule.disposition, AlgorithmDisposition::prohibited)
        || rule.security_strength_bits == 0U || rule.security_strength_bits > 1024U
        || !bounded_text(rule.standard_reference, kMaximumReferenceLength)
        || !digest(rule.standard_evidence_digest)) {
        return false;
    }
    if (rule.deprecate_at.has_value() && rule.prohibit_at.has_value()
        && *rule.prohibit_at <= *rule.deprecate_at) {
        return false;
    }
    return true;
}

bool valid_policy_draft_shape(const CryptoPolicyDraft& draft) {
    return portable_identifier(draft.id, kMaximumIdLength)
           && portable_identifier(draft.threat_model_id, kMaximumIdLength)
           && draft.threat_model_revision > 0U && digest(draft.source_set_digest)
           && digest(draft.change_evidence_digest) && !draft.rules.empty()
           && draft.rules.size() <= kMaximumAlgorithmsPerRecord
           && draft.review_at > draft.effective_from
           && draft.review_at > draft.recorded_at
           && std::all_of(draft.rules.begin(), draft.rules.end(), valid_rule_shape);
}

CryptoPolicyDraft draft_from_policy(const CryptoPolicyRevision& revision) {
    return {revision.id,
            revision.threat_model_id,
            revision.threat_model_revision,
            revision.source_set_digest,
            revision.change_evidence_digest,
            revision.rules,
            revision.effective_from,
            revision.review_at,
            revision.recorded_at};
}

void append_rule_material(std::string& material, const CryptoAlgorithmRule& rule) {
    append_material(material, rule.algorithm_id);
    append_material(material, to_string(rule.function));
    append_material(material, to_string(rule.quantum_readiness));
    append_material(material, to_string(rule.disposition));
    append_material(material, std::to_string(rule.security_strength_bits));
    append_material(material, rule.standard_reference);
    append_material(material, rule.standard_evidence_digest);
    append_optional_time(material, rule.deprecate_at);
    append_optional_time(material, rule.prohibit_at);
}

std::string policy_revision_digest(std::string_view registry_id,
                                   const CryptoPolicyRevision& revision) {
    std::string material{"genesis.crypto.policy.v1"};
    append_material(material, registry_id);
    append_material(material, revision.id);
    append_material(material, std::to_string(revision.revision));
    append_material(material, revision.threat_model_id);
    append_material(material, std::to_string(revision.threat_model_revision));
    append_material(material, revision.threat_model_digest);
    append_material(material, revision.source_set_digest);
    append_material(material, revision.change_evidence_digest);
    append_material(material, std::to_string(revision.rules.size()));
    for (const auto& rule : revision.rules) {
        append_rule_material(material, rule);
    }
    append_material(material, std::to_string(revision.effective_from));
    append_material(material, std::to_string(revision.review_at));
    append_material(material, std::to_string(revision.recorded_at));
    append_material(material, revision.previous_revision_digest);
    return runtime::sha256(material);
}

bool valid_capability_shape(const CryptoAlgorithmCapability& capability) {
    return portable_identifier(capability.algorithm_id, kMaximumIdLength)
           && enum_in_range(capability.function, CryptoFunction::key_wrapping)
           && portable_identifier(capability.implementation_route, kMaximumIdLength)
           && digest(capability.implementation_digest);
}

bool valid_provider_shape(const CryptoProviderManifest& provider) {
    if (!portable_identifier(provider.provider_id, kMaximumIdLength)
        || !bounded_text(provider.implementation_name, kMaximumIdLength)
        || !portable_identifier(provider.implementation_version, kMaximumIdLength)
        || !portable_identifier(provider.platform_id, kMaximumIdLength)
        || !digest(provider.module_boundary_digest)
        || !digest(provider.module_binary_digest)
        || !digest(provider.source_provenance_digest)
        || !digest(provider.license_evidence_digest)
        || !digest(provider.build_evidence_digest) || provider.capabilities.empty()
        || provider.capabilities.size() > kMaximumAlgorithmsPerRecord
        || !std::all_of(provider.capabilities.begin(),
                        provider.capabilities.end(),
                        valid_capability_shape)) {
        return false;
    }
    try {
        return provider.provider_id
               == derive_crypto_provider_id(provider.implementation_name,
                                            provider.implementation_version,
                                            provider.platform_id,
                                            provider.module_binary_digest);
    } catch (const std::exception&) {
        return false;
    }
}

void append_capability_material(std::string& material,
                                const CryptoAlgorithmCapability& capability) {
    append_material(material, capability.algorithm_id);
    append_material(material, to_string(capability.function));
    append_material(material, capability.implementation_route);
    append_material(material, capability.implementation_digest);
}

std::string provider_manifest_digest(std::string_view registry_id,
                                     const CryptoProviderManifest& provider) {
    std::string material{"genesis.crypto.provider-manifest.v1"};
    append_material(material, registry_id);
    append_material(material, provider.provider_id);
    append_material(material, provider.implementation_name);
    append_material(material, provider.implementation_version);
    append_material(material, provider.platform_id);
    append_material(material, provider.module_boundary_digest);
    append_material(material, provider.module_binary_digest);
    append_material(material, provider.source_provenance_digest);
    append_material(material, provider.license_evidence_digest);
    append_material(material, provider.build_evidence_digest);
    append_material(material, std::to_string(provider.capabilities.size()));
    for (const auto& capability : provider.capabilities) {
        append_capability_material(material, capability);
    }
    append_material(material, std::to_string(provider.declared_at));
    return runtime::sha256(material);
}

bool valid_route_shape(const CryptoRouteEvidence& route) {
    return portable_identifier(route.algorithm_id, kMaximumIdLength)
           && enum_in_range(route.function, CryptoFunction::key_wrapping)
           && portable_identifier(route.implementation_route, kMaximumIdLength)
           && digest(route.implementation_digest)
           && digest(route.functional_test_digest)
           && digest(route.platform_evidence_digest)
           && optional_bounded_text(route.algorithm_validation_reference,
                                    kMaximumReferenceLength)
           && optional_digest(route.algorithm_validation_evidence_digest)
           && optional_digest(route.key_custody_evidence_digest)
           && (route.algorithm_validation_reference.empty()
               == route.algorithm_validation_evidence_digest.empty());
}

bool valid_assessment_draft_shape(const CryptoProviderAssessmentDraft& draft) {
    return portable_identifier(draft.id, kMaximumIdLength)
           && portable_identifier(draft.provider_id, kMaximumIdLength)
           && enum_in_range(draft.kind, ProviderAssessmentKind::revocation)
           && portable_identifier(draft.evaluator_entity_id, kMaximumIdLength)
           && digest(draft.evaluation_evidence_digest)
           && optional_bounded_text(draft.module_validation_reference,
                                    kMaximumReferenceLength)
           && optional_digest(draft.module_validation_evidence_digest)
           && (draft.module_validation_reference.empty()
               == draft.module_validation_evidence_digest.empty())
           && draft.routes.size() <= kMaximumAlgorithmsPerRecord
           && std::all_of(
               draft.routes.begin(), draft.routes.end(), valid_route_shape)
           && draft.observed_at <= draft.recorded_at
           && (!draft.valid_until.has_value()
               || *draft.valid_until > draft.recorded_at);
}

CryptoProviderAssessmentDraft draft_from_assessment(
    const CryptoProviderAssessment& assessment) {
    return {assessment.id,
            assessment.provider_id,
            assessment.kind,
            assessment.policy_id,
            assessment.policy_revision,
            assessment.evaluator_entity_id,
            assessment.evaluation_evidence_digest,
            assessment.module_validation_reference,
            assessment.module_validation_evidence_digest,
            assessment.routes,
            assessment.observed_at,
            assessment.recorded_at,
            assessment.valid_until};
}

void append_route_material(std::string& material, const CryptoRouteEvidence& route) {
    append_material(material, route.algorithm_id);
    append_material(material, to_string(route.function));
    append_material(material, route.implementation_route);
    append_material(material, route.implementation_digest);
    append_material(material, route.functional_test_digest);
    append_material(material, route.platform_evidence_digest);
    append_material(material, route.algorithm_validation_reference);
    append_material(material, route.algorithm_validation_evidence_digest);
    append_material(material, route.key_custody_evidence_digest);
}

std::string assessment_digest(std::string_view registry_id,
                              const CryptoProviderAssessment& assessment) {
    std::string material{"genesis.crypto.provider-assessment.v1"};
    append_material(material, registry_id);
    append_material(material, assessment.id);
    append_material(material, assessment.provider_id);
    append_material(material, to_string(assessment.kind));
    append_material(material, assessment.policy_id);
    append_material(material, std::to_string(assessment.policy_revision));
    append_material(material, assessment.evaluator_entity_id);
    append_material(material, assessment.evaluation_evidence_digest);
    append_material(material, assessment.module_validation_reference);
    append_material(material, assessment.module_validation_evidence_digest);
    append_material(material, std::to_string(assessment.routes.size()));
    for (const auto& route : assessment.routes) {
        append_route_material(material, route);
    }
    append_material(material, std::to_string(assessment.sequence));
    append_material(material, std::to_string(assessment.observed_at));
    append_material(material, std::to_string(assessment.recorded_at));
    append_optional_time(material, assessment.valid_until);
    append_material(material, assessment.previous_assessment_digest);
    return runtime::sha256(material);
}

ProviderState transitioned_state(ProviderState current,
                                 ProviderAssessmentKind kind,
                                 bool* valid = nullptr) {
    bool accepted = false;
    auto next = current;
    switch (current) {
    case ProviderState::declared:
        accepted = kind == ProviderAssessmentKind::observation
                   || kind == ProviderAssessmentKind::revocation;
        next = kind == ProviderAssessmentKind::observation ? ProviderState::observed
                                                           : ProviderState::revoked;
        break;
    case ProviderState::observed:
        accepted = true;
        switch (kind) {
        case ProviderAssessmentKind::observation:
            next = ProviderState::observed;
            break;
        case ProviderAssessmentKind::qualification:
            next = ProviderState::qualified;
            break;
        case ProviderAssessmentKind::suspension:
            next = ProviderState::suspended;
            break;
        case ProviderAssessmentKind::revocation:
            next = ProviderState::revoked;
            break;
        }
        break;
    case ProviderState::qualified:
        accepted = kind == ProviderAssessmentKind::qualification
                   || kind == ProviderAssessmentKind::suspension
                   || kind == ProviderAssessmentKind::revocation;
        if (kind == ProviderAssessmentKind::suspension) {
            next = ProviderState::suspended;
        } else if (kind == ProviderAssessmentKind::revocation) {
            next = ProviderState::revoked;
        }
        break;
    case ProviderState::suspended:
        accepted = kind == ProviderAssessmentKind::observation
                   || kind == ProviderAssessmentKind::revocation;
        next = kind == ProviderAssessmentKind::observation ? ProviderState::observed
                                                           : ProviderState::revoked;
        break;
    case ProviderState::revoked:
        accepted = false;
        break;
    }
    if (valid != nullptr) {
        *valid = accepted;
    }
    return next;
}

const CryptoAlgorithmCapability* find_capability(const CryptoProviderManifest& provider,
                                                 std::string_view algorithm_id,
                                                 CryptoFunction function) {
    const auto found = std::find_if(
        provider.capabilities.begin(), provider.capabilities.end(), [&](const auto& item) {
            return item.algorithm_id == algorithm_id && item.function == function;
        });
    return found == provider.capabilities.end() ? nullptr : &*found;
}

bool route_matches_capability(const CryptoProviderManifest& provider,
                              const CryptoRouteEvidence& route) {
    const auto* capability = find_capability(
        provider, route.algorithm_id, route.function);
    return capability != nullptr
           && capability->implementation_route == route.implementation_route
           && capability->implementation_digest == route.implementation_digest;
}

const CryptoAlgorithmRule* find_rule(const CryptoPolicyRevision& policy,
                                     std::string_view algorithm_id,
                                     CryptoFunction function) {
    const auto found = std::find_if(
        policy.rules.begin(), policy.rules.end(), [&](const auto& item) {
            return item.algorithm_id == algorithm_id && item.function == function;
        });
    return found == policy.rules.end() ? nullptr : &*found;
}

const CryptoRouteEvidence* find_route(const CryptoProviderAssessment& assessment,
                                      std::string_view algorithm_id,
                                      CryptoFunction function) {
    const auto found = std::find_if(
        assessment.routes.begin(), assessment.routes.end(), [&](const auto& item) {
            return item.algorithm_id == algorithm_id && item.function == function;
        });
    return found == assessment.routes.end() ? nullptr : &*found;
}

const CryptoPolicyRevision* policy_at(
    const std::map<std::string,
                   std::vector<CryptoPolicyRevision>,
                   std::less<>>& policies,
    std::string_view id,
    std::uint64_t at) {
    const auto family = policies.find(id);
    if (family == policies.end()) {
        return nullptr;
    }
    const CryptoPolicyRevision* candidate = nullptr;
    for (const auto& revision : family->second) {
        if (revision.effective_from <= at) {
            candidate = &revision;
        }
    }
    return candidate != nullptr && at < candidate->review_at ? candidate : nullptr;
}

const CryptoThreatModelRevision* threat_at(
    const std::map<std::string,
                   std::vector<CryptoThreatModelRevision>,
                   std::less<>>& models,
    std::string_view id,
    std::uint64_t revision_number,
    std::uint64_t at) {
    const auto family = models.find(id);
    if (family == models.end() || revision_number == 0U
        || revision_number > family->second.size()) {
        return nullptr;
    }
    const auto& revision = family->second[static_cast<std::size_t>(revision_number - 1U)];
    return revision.effective_from <= at && at < revision.review_at ? &revision : nullptr;
}

bool route_was_observed(const std::vector<CryptoProviderAssessment>& history,
                        const CryptoRouteEvidence& route,
                        std::size_t end_index) {
    const auto bounded_end = std::min(end_index, history.size());
    return std::any_of(history.begin(),
                       history.begin() + static_cast<std::ptrdiff_t>(bounded_end),
                       [&](const auto& prior) {
                           const auto* prior_route =
                               prior.kind == ProviderAssessmentKind::observation
                                   ? find_route(prior,
                                                route.algorithm_id,
                                                route.function)
                                   : nullptr;
                           return prior_route != nullptr
                                  && prior_route->implementation_route
                                         == route.implementation_route
                                  && prior_route->implementation_digest
                                         == route.implementation_digest
                                  && prior_route->functional_test_digest
                                         == route.functional_test_digest
                                  && prior_route->platform_evidence_digest
                                         == route.platform_evidence_digest;
                       });
}

bool qualification_route_allowed(const CryptoPolicyRevision& policy,
                                 const CryptoRouteEvidence& route,
                                 std::uint64_t at) {
    const auto* rule = find_rule(policy, route.algorithm_id, route.function);
    return rule != nullptr && rule->disposition == AlgorithmDisposition::approved
           && (!rule->prohibit_at.has_value() || at < *rule->prohibit_at);
}

} // namespace

std::string_view to_string(CryptoFunction value) noexcept {
    const auto index = static_cast<std::size_t>(value);
    return index < kFunctionNames.size() ? kFunctionNames[index] : "unknown";
}

std::string_view to_string(QuantumReadiness value) noexcept {
    const auto index = static_cast<std::size_t>(value);
    return index < kQuantumNames.size() ? kQuantumNames[index] : "unknown";
}

std::string_view to_string(AlgorithmDisposition value) noexcept {
    const auto index = static_cast<std::size_t>(value);
    return index < kDispositionNames.size() ? kDispositionNames[index] : "unknown";
}

std::string_view to_string(CryptoThreatCategory value) noexcept {
    const auto index = static_cast<std::size_t>(value);
    return index < kThreatNames.size() ? kThreatNames[index] : "unknown";
}

std::string_view to_string(ProviderAssessmentKind value) noexcept {
    const auto index = static_cast<std::size_t>(value);
    return index < kAssessmentKindNames.size() ? kAssessmentKindNames[index] : "unknown";
}

std::string_view to_string(ProviderState value) noexcept {
    const auto index = static_cast<std::size_t>(value);
    return index < kProviderStateNames.size() ? kProviderStateNames[index] : "unknown";
}

bool crypto_function_from_string(std::string_view text,
                                 CryptoFunction& value) noexcept {
    return parse_enum(text, kFunctionNames, value);
}

bool quantum_readiness_from_string(std::string_view text,
                                   QuantumReadiness& value) noexcept {
    return parse_enum(text, kQuantumNames, value);
}

bool algorithm_disposition_from_string(std::string_view text,
                                       AlgorithmDisposition& value) noexcept {
    return parse_enum(text, kDispositionNames, value);
}

bool crypto_threat_category_from_string(std::string_view text,
                                        CryptoThreatCategory& value) noexcept {
    return parse_enum(text, kThreatNames, value);
}

bool provider_assessment_kind_from_string(std::string_view text,
                                          ProviderAssessmentKind& value) noexcept {
    return parse_enum(text, kAssessmentKindNames, value);
}

bool provider_state_from_string(std::string_view text,
                                ProviderState& value) noexcept {
    return parse_enum(text, kProviderStateNames, value);
}

bool crypto_function_requires_key_material(CryptoFunction function) noexcept {
    switch (function) {
    case CryptoFunction::secure_hash:
    case CryptoFunction::random_generation:
        return false;
    case CryptoFunction::message_authentication:
    case CryptoFunction::authenticated_encryption:
    case CryptoFunction::digital_signature:
    case CryptoFunction::key_establishment:
    case CryptoFunction::key_derivation:
    case CryptoFunction::key_wrapping:
        return true;
    }
    return true;
}

bool CryptoEntityAudit::clean() const noexcept {
    return registry_errors == 0U && owner_mismatches == 0U
           && missing_evaluators == 0U && incompatible_evaluators == 0U
           && temporal_mismatches == 0U;
}

std::string derive_crypto_registry_id(std::string_view entity_namespace_id,
                                      std::string_view owner_entity_id) {
    if (!portable_identifier(entity_namespace_id, 128U)
        || !portable_identifier(owner_entity_id, kMaximumIdLength)) {
        throw std::invalid_argument("invalid cryptographic registry identity material");
    }
    std::string material{"genesis.crypto.registry.v1"};
    append_material(material, entity_namespace_id);
    append_material(material, owner_entity_id);
    return "genesis.crypto.registry.v1." + runtime::sha256(material);
}

std::string derive_crypto_provider_id(std::string_view implementation_name,
                                      std::string_view implementation_version,
                                      std::string_view platform_id,
                                      std::string_view module_binary_digest) {
    if (!bounded_text(implementation_name, kMaximumIdLength)
        || !portable_identifier(implementation_version, kMaximumIdLength)
        || !portable_identifier(platform_id, kMaximumIdLength)
        || !digest(module_binary_digest)) {
        throw std::invalid_argument("invalid cryptographic provider identity material");
    }
    std::string material{"genesis.crypto.provider.v1"};
    append_material(material, implementation_name);
    append_material(material, implementation_version);
    append_material(material, platform_id);
    append_material(material, module_binary_digest);
    return "genesis.crypto.provider.v1." + runtime::sha256(material);
}

CryptoProviderRegistry::CryptoProviderRegistry(
    std::string registry_id,
    std::string entity_namespace_id,
    std::string owner_entity_id,
    std::size_t threat_revision_capacity,
    std::size_t policy_revision_capacity,
    std::size_t provider_capacity,
    std::size_t assessment_capacity)
    : registry_id_(std::move(registry_id)),
      entity_namespace_id_(std::move(entity_namespace_id)),
      owner_entity_id_(std::move(owner_entity_id)),
      threat_revision_capacity_(threat_revision_capacity),
      policy_revision_capacity_(policy_revision_capacity),
      provider_capacity_(provider_capacity),
      assessment_capacity_(assessment_capacity) {
    if (registry_id_
            != derive_crypto_registry_id(entity_namespace_id_, owner_entity_id_)
        || threat_revision_capacity_ == 0U || policy_revision_capacity_ == 0U
        || provider_capacity_ == 0U || assessment_capacity_ == 0U
        || threat_revision_capacity_ > kMaximumRegistryItems
        || policy_revision_capacity_ > kMaximumRegistryItems
        || provider_capacity_ > kMaximumRegistryItems
        || assessment_capacity_ > kMaximumRegistryItems) {
        throw std::invalid_argument("invalid cryptographic provider registry configuration");
    }
}

bool CryptoProviderRegistry::append_threat_model(
    CryptoThreatModelDraft draft,
    const identity::EntityRegistry& entities,
    CryptoRegistryError* error) {
    set_error(error, CryptoRegistryErrorCode::none, {});
    if (!valid_owner(*this, entities, draft.recorded_at)) {
        set_error(error,
                  CryptoRegistryErrorCode::owner_binding_mismatch,
                  "cryptographic registry owner is not valid at the record time");
        return false;
    }
    if (!valid_threat_draft_shape(draft)) {
        set_error(error,
                  CryptoRegistryErrorCode::invalid_entry,
                  "invalid cryptographic threat-model revision");
        return false;
    }
    if (threat_revision_count_ >= threat_revision_capacity_) {
        set_error(error,
                  CryptoRegistryErrorCode::capacity_exceeded,
                  "cryptographic threat-model capacity exceeded");
        return false;
    }
    std::sort(draft.threats.begin(), draft.threats.end(), threat_sort_less);
    if (std::adjacent_find(draft.threats.begin(), draft.threats.end())
        != draft.threats.end()) {
        set_error(error,
                  CryptoRegistryErrorCode::duplicate_entry,
                  "duplicate threat category in one model revision");
        return false;
    }

    auto& history = threat_models_[draft.id];
    if (!history.empty()
        && (draft.recorded_at < history.back().recorded_at
            || draft.effective_from < history.back().effective_from)) {
        set_error(error,
                  CryptoRegistryErrorCode::temporal_conflict,
                  "threat-model revision moves time backward");
        return false;
    }
    if (history.size() == std::numeric_limits<std::uint64_t>::max()) {
        set_error(error,
                  CryptoRegistryErrorCode::revision_conflict,
                  "threat-model revision sequence exhausted");
        return false;
    }

    CryptoThreatModelRevision revision{std::move(draft.id),
                                       std::move(draft.threats),
                                       std::move(draft.asset_inventory_digest),
                                       std::move(draft.trust_boundaries_digest),
                                       std::move(draft.mitigation_plan_digest),
                                       std::move(draft.residual_risk_digest),
                                       std::move(draft.evidence_digest),
                                       static_cast<std::uint64_t>(history.size() + 1U),
                                       draft.effective_from,
                                       draft.review_at,
                                       draft.recorded_at,
                                       history.empty() ? registry_id_
                                                       : history.back().revision_digest,
                                       {}};
    revision.revision_digest = threat_revision_digest(registry_id_, revision);
    history.push_back(std::move(revision));
    ++threat_revision_count_;
    return true;
}

bool CryptoProviderRegistry::append_policy(CryptoPolicyDraft draft,
                                           const identity::EntityRegistry& entities,
                                           CryptoRegistryError* error) {
    set_error(error, CryptoRegistryErrorCode::none, {});
    if (!valid_owner(*this, entities, draft.recorded_at)) {
        set_error(error,
                  CryptoRegistryErrorCode::owner_binding_mismatch,
                  "cryptographic registry owner is not valid at the policy record time");
        return false;
    }
    if (!valid_policy_draft_shape(draft)) {
        set_error(error,
                  CryptoRegistryErrorCode::invalid_entry,
                  "invalid cryptographic algorithm-policy revision");
        return false;
    }
    if (policy_revision_count_ >= policy_revision_capacity_) {
        set_error(error,
                  CryptoRegistryErrorCode::capacity_exceeded,
                  "cryptographic policy capacity exceeded");
        return false;
    }
    std::sort(draft.rules.begin(), draft.rules.end(), rule_sort_less);
    if (!sorted_unique_by(draft.rules, rule_sort_less)) {
        set_error(error,
                  CryptoRegistryErrorCode::duplicate_entry,
                  "duplicate algorithm/function rule in one policy revision");
        return false;
    }
    const auto* threat = threat_model(draft.threat_model_id, draft.threat_model_revision);
    if (threat == nullptr) {
        set_error(error,
                  CryptoRegistryErrorCode::missing_threat_model,
                  "cryptographic policy references an unknown threat-model revision");
        return false;
    }
    if (draft.recorded_at < threat->recorded_at
        || draft.effective_from < threat->effective_from
        || draft.review_at > threat->review_at) {
        set_error(error,
                  CryptoRegistryErrorCode::temporal_conflict,
                  "cryptographic policy outlives or predates its threat model");
        return false;
    }
    for (const auto& rule : draft.rules) {
        if ((rule.deprecate_at.has_value()
             && *rule.deprecate_at < draft.effective_from)
            || (rule.prohibit_at.has_value()
                && *rule.prohibit_at < draft.effective_from)) {
            set_error(error,
                      CryptoRegistryErrorCode::temporal_conflict,
                      "algorithm transition predates its policy revision");
            return false;
        }
    }

    auto& history = policies_[draft.id];
    if (!history.empty()
        && (draft.recorded_at < history.back().recorded_at
            || draft.effective_from < history.back().effective_from)) {
        set_error(error,
                  CryptoRegistryErrorCode::temporal_conflict,
                  "cryptographic policy revision moves time backward");
        return false;
    }
    if (history.size() == std::numeric_limits<std::uint64_t>::max()) {
        set_error(error,
                  CryptoRegistryErrorCode::revision_conflict,
                  "cryptographic policy revision sequence exhausted");
        return false;
    }

    CryptoPolicyRevision revision{std::move(draft.id),
                                  draft.threat_model_id,
                                  draft.threat_model_revision,
                                  threat->revision_digest,
                                  std::move(draft.source_set_digest),
                                  std::move(draft.change_evidence_digest),
                                  std::move(draft.rules),
                                  static_cast<std::uint64_t>(history.size() + 1U),
                                  draft.effective_from,
                                  draft.review_at,
                                  draft.recorded_at,
                                  history.empty() ? registry_id_
                                                  : history.back().revision_digest,
                                  {}};
    revision.revision_digest = policy_revision_digest(registry_id_, revision);
    history.push_back(std::move(revision));
    ++policy_revision_count_;
    return true;
}

bool CryptoProviderRegistry::register_provider(
    CryptoProviderManifest manifest,
    const identity::EntityRegistry& entities,
    CryptoRegistryError* error) {
    set_error(error, CryptoRegistryErrorCode::none, {});
    if (!valid_owner(*this, entities, manifest.declared_at)) {
        set_error(error,
                  CryptoRegistryErrorCode::owner_binding_mismatch,
                  "cryptographic registry owner is not valid at provider declaration");
        return false;
    }
    std::sort(manifest.capabilities.begin(),
              manifest.capabilities.end(),
              capability_sort_less);
    if (!valid_provider_shape(manifest)
        || !sorted_unique_by(manifest.capabilities, capability_sort_less)) {
        set_error(error,
                  CryptoRegistryErrorCode::invalid_entry,
                  "invalid cryptographic provider manifest");
        return false;
    }
    if (providers_.size() >= provider_capacity_) {
        set_error(error,
                  CryptoRegistryErrorCode::capacity_exceeded,
                  "cryptographic provider capacity exceeded");
        return false;
    }
    const auto provider_id = manifest.provider_id;
    if (!providers_.emplace(provider_id, std::move(manifest)).second) {
        set_error(error,
                  CryptoRegistryErrorCode::duplicate_entry,
                  "duplicate cryptographic provider manifest");
        return false;
    }
    return true;
}

bool CryptoProviderRegistry::record_assessment(
    CryptoProviderAssessmentDraft draft,
    const identity::EntityRegistry& entities,
    CryptoRegistryError* error) {
    set_error(error, CryptoRegistryErrorCode::none, {});
    if (!valid_owner(*this, entities, draft.recorded_at)) {
        set_error(error,
                  CryptoRegistryErrorCode::owner_binding_mismatch,
                  "cryptographic registry owner is not valid at assessment time");
        return false;
    }
    if (!valid_assessment_draft_shape(draft)) {
        set_error(error,
                  CryptoRegistryErrorCode::invalid_entry,
                  "invalid cryptographic provider assessment");
        return false;
    }
    if (assessment_count_ >= assessment_capacity_) {
        set_error(error,
                  CryptoRegistryErrorCode::capacity_exceeded,
                  "cryptographic provider assessment capacity exceeded");
        return false;
    }
    if (assessment_index_.contains(draft.id)) {
        set_error(error,
                  CryptoRegistryErrorCode::duplicate_entry,
                  "duplicate cryptographic provider assessment ID");
        return false;
    }
    const auto provider_found = providers_.find(draft.provider_id);
    if (provider_found == providers_.end()) {
        set_error(error,
                  CryptoRegistryErrorCode::missing_provider,
                  "cryptographic provider assessment references an unknown provider");
        return false;
    }
    if (draft.recorded_at < provider_found->second.declared_at) {
        set_error(error,
                  CryptoRegistryErrorCode::temporal_conflict,
                  "cryptographic provider assessment predates the provider declaration");
        return false;
    }
    const auto* evaluator = entities.find_entity(draft.evaluator_entity_id);
    if (evaluator == nullptr) {
        set_error(error,
                  CryptoRegistryErrorCode::missing_evaluator,
                  "cryptographic provider evaluator is not registered");
        return false;
    }
    if (!evaluator_kind_permitted(evaluator->kind)) {
        set_error(error,
                  CryptoRegistryErrorCode::incompatible_evaluator,
                  "cryptographic provider evaluator has an incompatible entity type");
        return false;
    }
    if (draft.recorded_at < evaluator->registered_at) {
        set_error(error,
                  CryptoRegistryErrorCode::temporal_conflict,
                  "cryptographic provider assessment predates evaluator registration");
        return false;
    }

    const auto history_found = assessments_.find(draft.provider_id);
    static const std::vector<CryptoProviderAssessment> kEmptyHistory;
    const auto& history = history_found == assessments_.end()
                              ? kEmptyHistory
                              : history_found->second;
    if (!history.empty() && draft.recorded_at < history.back().recorded_at) {
        set_error(error,
                  CryptoRegistryErrorCode::temporal_conflict,
                  "cryptographic provider assessment history moves time backward");
        return false;
    }
    bool transition_valid = false;
    static_cast<void>(transitioned_state(
        provider_state(draft.provider_id), draft.kind, &transition_valid));
    if (!transition_valid) {
        set_error(error,
                  CryptoRegistryErrorCode::transition_conflict,
                  "invalid cryptographic provider evidence-state transition");
        return false;
    }

    std::sort(draft.routes.begin(), draft.routes.end(), route_sort_less);
    if (!sorted_unique_by(draft.routes, route_sort_less)) {
        set_error(error,
                  CryptoRegistryErrorCode::duplicate_entry,
                  "duplicate algorithm/function route in one assessment");
        return false;
    }
    const bool evaluates_routes =
        draft.kind == ProviderAssessmentKind::observation
        || draft.kind == ProviderAssessmentKind::qualification;
    if (evaluates_routes != !draft.routes.empty()) {
        set_error(error,
                  CryptoRegistryErrorCode::route_conflict,
                  "assessment kind and route-evidence shape disagree");
        return false;
    }

    const CryptoPolicyRevision* referenced_policy = nullptr;
    if (evaluates_routes) {
        referenced_policy = policy(draft.policy_id, draft.policy_revision);
        const auto* current_policy = policy_at(policies_, draft.policy_id, draft.recorded_at);
        if (referenced_policy == nullptr || current_policy != referenced_policy) {
            set_error(error,
                      CryptoRegistryErrorCode::missing_policy,
                      "assessment does not reference the current policy revision");
            return false;
        }
        for (const auto& route : draft.routes) {
            if (!route_matches_capability(provider_found->second, route)
                || find_rule(*referenced_policy, route.algorithm_id, route.function)
                       == nullptr) {
                set_error(error,
                          CryptoRegistryErrorCode::route_conflict,
                          "assessment route is not declared by provider and policy");
                return false;
            }
        }
    } else if (!draft.policy_id.empty() || draft.policy_revision != 0U
               || !draft.module_validation_reference.empty()
               || !draft.module_validation_evidence_digest.empty()
               || draft.valid_until.has_value()) {
        set_error(error,
                  CryptoRegistryErrorCode::invalid_entry,
                  "suspension and revocation carry reason evidence, not qualification data");
        return false;
    }

    if (draft.kind == ProviderAssessmentKind::qualification) {
        const auto* threat = threat_model(referenced_policy->threat_model_id,
                                          referenced_policy->threat_model_revision);
        if (threat == nullptr
            || threat->revision_digest != referenced_policy->threat_model_digest
            || !threat_qualification_ready(*threat)
            || threat_at(threat_models_,
                         threat->id,
                         threat->revision,
                         draft.recorded_at)
                   == nullptr) {
            set_error(error,
                      CryptoRegistryErrorCode::policy_conflict,
                      "qualification requires a current, complete threat model");
            return false;
        }
        if (evaluator->kind != identity::EntityKind::organization
            || evaluator->entity_id == owner_entity_id_) {
            set_error(error,
                      CryptoRegistryErrorCode::incompatible_evaluator,
                      "qualification requires a separately registered organization evaluator");
            return false;
        }
        if (draft.module_validation_reference.empty()
            || draft.module_validation_evidence_digest.empty()
            || !draft.valid_until.has_value()
            || *draft.valid_until > referenced_policy->review_at) {
            set_error(error,
                      CryptoRegistryErrorCode::policy_conflict,
                      "qualification lacks bounded module-validation evidence");
            return false;
        }
        for (const auto& route : draft.routes) {
            if (!route_was_observed(history, route, history.size())
                || route.algorithm_validation_reference.empty()
                || route.algorithm_validation_evidence_digest.empty()
                || !qualification_route_allowed(
                    *referenced_policy, route, draft.recorded_at)) {
                set_error(error,
                          CryptoRegistryErrorCode::policy_conflict,
                          "qualification route lacks observation, validation, or approval");
                return false;
            }
        }
    } else if (draft.kind == ProviderAssessmentKind::observation) {
        if (!draft.module_validation_reference.empty()
            && draft.module_validation_evidence_digest.empty()) {
            set_error(error,
                      CryptoRegistryErrorCode::invalid_entry,
                      "module validation reference has no evidence digest");
            return false;
        }
    }

    CryptoProviderAssessment assessment{std::move(draft.id),
                                        std::move(draft.provider_id),
                                        draft.kind,
                                        std::move(draft.policy_id),
                                        draft.policy_revision,
                                        std::move(draft.evaluator_entity_id),
                                        std::move(draft.evaluation_evidence_digest),
                                        std::move(draft.module_validation_reference),
                                        std::move(draft.module_validation_evidence_digest),
                                        std::move(draft.routes),
                                        static_cast<std::uint64_t>(history.size() + 1U),
                                        draft.observed_at,
                                        draft.recorded_at,
                                        draft.valid_until,
                                        history.empty()
                                            ? provider_manifest_digest(
                                                  registry_id_, provider_found->second)
                                            : history.back().assessment_digest,
                                        {}};
    assessment.assessment_digest = assessment_digest(registry_id_, assessment);
    const auto assessment_id = assessment.id;
    const auto provider_id = assessment.provider_id;
    assessments_[provider_id].push_back(std::move(assessment));
    assessment_index_.emplace(assessment_id, provider_id);
    ++assessment_count_;
    return true;
}

const CryptoThreatModelRevision* CryptoProviderRegistry::threat_model(
    std::string_view id,
    std::uint64_t revision) const {
    const auto found = threat_models_.find(id);
    if (found == threat_models_.end() || revision == 0U
        || revision > found->second.size()) {
        return nullptr;
    }
    return &found->second[static_cast<std::size_t>(revision - 1U)];
}

const CryptoPolicyRevision* CryptoProviderRegistry::policy(
    std::string_view id,
    std::uint64_t revision) const {
    const auto found = policies_.find(id);
    if (found == policies_.end() || revision == 0U
        || revision > found->second.size()) {
        return nullptr;
    }
    return &found->second[static_cast<std::size_t>(revision - 1U)];
}

const CryptoProviderManifest* CryptoProviderRegistry::provider(
    std::string_view provider_id) const {
    const auto found = providers_.find(provider_id);
    return found == providers_.end() ? nullptr : &found->second;
}

const std::vector<CryptoProviderAssessment>*
CryptoProviderRegistry::assessment_history(std::string_view provider_id) const {
    const auto found = assessments_.find(provider_id);
    return found == assessments_.end() ? nullptr : &found->second;
}

ProviderState CryptoProviderRegistry::provider_state(std::string_view provider_id) const {
    auto state = ProviderState::declared;
    const auto* history = assessment_history(provider_id);
    if (history == nullptr) {
        return state;
    }
    for (const auto& assessment : *history) {
        state = transitioned_state(state, assessment.kind);
    }
    return state;
}

CryptoProviderDecision CryptoProviderRegistry::evaluate(
    std::string_view provider_id,
    std::string_view algorithm_id,
    CryptoFunction function,
    std::uint64_t at) const {
    CryptoProviderDecision decision;
    const auto* manifest = provider(provider_id);
    if (manifest == nullptr) {
        decision.reason = "provider_not_registered";
        return decision;
    }
    decision.provider_found = true;
    decision.capability_declared =
        find_capability(*manifest, algorithm_id, function) != nullptr;
    decision.key_custody_required = crypto_function_requires_key_material(function);
    if (!decision.capability_declared) {
        decision.reason = "capability_not_declared";
        return decision;
    }

    auto state = ProviderState::declared;
    const CryptoProviderAssessment* qualification = nullptr;
    const auto* history = assessment_history(provider_id);
    if (history != nullptr) {
        for (const auto& assessment : *history) {
            if (assessment.recorded_at > at) {
                break;
            }
            state = transitioned_state(state, assessment.kind);
            if (assessment.kind == ProviderAssessmentKind::observation) {
                decision.provider_observed = true;
            } else if (assessment.kind == ProviderAssessmentKind::qualification) {
                qualification = &assessment;
            } else {
                qualification = nullptr;
            }
        }
    }
    decision.state = state;
    decision.provider_qualified =
        state == ProviderState::qualified && qualification != nullptr
        && qualification->valid_until.has_value() && at < *qualification->valid_until;
    if (!decision.provider_qualified) {
        decision.reason = state == ProviderState::revoked
                              ? "provider_revoked"
                              : state == ProviderState::suspended
                                    ? "provider_suspended"
                                    : "provider_not_currently_qualified";
        return decision;
    }

    const auto* current_policy = policy_at(policies_, qualification->policy_id, at);
    decision.policy_current = current_policy != nullptr
                              && current_policy->revision
                                     == qualification->policy_revision;
    if (!decision.policy_current) {
        decision.reason = "qualification_policy_is_not_current";
        return decision;
    }
    const auto* current_threat = threat_at(threat_models_,
                                           current_policy->threat_model_id,
                                           current_policy->threat_model_revision,
                                           at);
    decision.threat_model_current =
        current_threat != nullptr
        && current_threat->revision_digest == current_policy->threat_model_digest;
    if (!decision.threat_model_current) {
        decision.reason = "qualification_threat_model_is_not_current";
        return decision;
    }
    const auto* rule = find_rule(*current_policy, algorithm_id, function);
    decision.algorithm_policy_approved =
        rule != nullptr && rule->disposition == AlgorithmDisposition::approved
        && (!rule->prohibit_at.has_value() || at < *rule->prohibit_at);
    if (!decision.algorithm_policy_approved) {
        decision.reason = "algorithm_not_approved_by_current_policy";
        return decision;
    }
    const auto* route = find_route(*qualification, algorithm_id, function);
    decision.validation_evidence_recorded =
        route != nullptr && !route->algorithm_validation_reference.empty()
        && digest(route->algorithm_validation_evidence_digest)
        && digest(qualification->module_validation_evidence_digest);
    decision.key_custody_evidence_recorded =
        route != nullptr && digest(route->key_custody_evidence_digest);
    decision.integration_candidate =
        decision.validation_evidence_recorded
        && (!decision.key_custody_required
            || decision.key_custody_evidence_recorded);
    decision.reason = decision.integration_candidate
                          ? "evidence_gated_integration_candidate_only"
                          : decision.key_custody_required
                                && !decision.key_custody_evidence_recorded
                              ? "key_custody_evidence_missing"
                              : "validation_evidence_missing";
    return decision;
}

CryptoEntityAudit CryptoProviderRegistry::audit_entities(
    const identity::EntityRegistry& entities) const {
    CryptoEntityAudit audit;
    if (!entities.verify()) {
        ++audit.registry_errors;
    }
    const auto* owner = entities.find_entity(owner_entity_id_);
    if (entities.namespace_id() != entity_namespace_id_ || owner == nullptr
        || (owner->kind != identity::EntityKind::organism
            && owner->kind != identity::EntityKind::organization)) {
        ++audit.owner_mismatches;
    }
    auto check_owner_time = [&](std::uint64_t recorded_at) {
        if (owner != nullptr && recorded_at < owner->registered_at) {
            ++audit.temporal_mismatches;
        }
    };
    for (const auto& [unused_id, history] : threat_models_) {
        static_cast<void>(unused_id);
        for (const auto& revision : history) {
            check_owner_time(revision.recorded_at);
        }
    }
    for (const auto& [unused_id, history] : policies_) {
        static_cast<void>(unused_id);
        for (const auto& revision : history) {
            check_owner_time(revision.recorded_at);
        }
    }
    for (const auto& [unused_id, manifest] : providers_) {
        static_cast<void>(unused_id);
        check_owner_time(manifest.declared_at);
    }
    for (const auto& [unused_id, history] : assessments_) {
        static_cast<void>(unused_id);
        for (const auto& assessment : history) {
            check_owner_time(assessment.recorded_at);
            const auto* evaluator = entities.find_entity(assessment.evaluator_entity_id);
            if (evaluator == nullptr) {
                ++audit.missing_evaluators;
                continue;
            }
            if (!evaluator_kind_permitted(evaluator->kind)
                || (assessment.kind == ProviderAssessmentKind::qualification
                    && (evaluator->kind != identity::EntityKind::organization
                        || evaluator->entity_id == owner_entity_id_))) {
                ++audit.incompatible_evaluators;
            }
            if (assessment.recorded_at < evaluator->registered_at) {
                ++audit.temporal_mismatches;
            }
        }
    }
    return audit;
}

bool CryptoProviderRegistry::verify() const {
    try {
        if (registry_id_
                != derive_crypto_registry_id(entity_namespace_id_, owner_entity_id_)
            || threat_revision_capacity_ == 0U || policy_revision_capacity_ == 0U
            || provider_capacity_ == 0U || assessment_capacity_ == 0U
            || threat_revision_capacity_ > kMaximumRegistryItems
            || policy_revision_capacity_ > kMaximumRegistryItems
            || provider_capacity_ > kMaximumRegistryItems
            || assessment_capacity_ > kMaximumRegistryItems
            || threat_revision_count_ > threat_revision_capacity_
            || policy_revision_count_ > policy_revision_capacity_
            || providers_.size() > provider_capacity_
            || assessment_count_ > assessment_capacity_) {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    std::size_t expected_threat_count = 0U;
    for (const auto& [id, history] : threat_models_) {
        std::string previous = registry_id_;
        std::uint64_t previous_recorded = 0U;
        std::uint64_t previous_effective = 0U;
        for (std::size_t index = 0U; index < history.size(); ++index) {
            const auto& revision = history[index];
            const auto draft = draft_from_threat(revision);
            if (revision.id != id || !valid_threat_draft_shape(draft)
                || !sorted_unique_threats(revision.threats)
                || revision.revision != index + 1U
                || revision.recorded_at < previous_recorded
                || revision.effective_from < previous_effective
                || revision.previous_revision_digest != previous
                || revision.revision_digest
                       != threat_revision_digest(registry_id_, revision)) {
                return false;
            }
            previous = revision.revision_digest;
            previous_recorded = revision.recorded_at;
            previous_effective = revision.effective_from;
            ++expected_threat_count;
        }
    }
    if (expected_threat_count != threat_revision_count_) {
        return false;
    }

    std::size_t expected_policy_count = 0U;
    for (const auto& [id, history] : policies_) {
        std::string previous = registry_id_;
        std::uint64_t previous_recorded = 0U;
        std::uint64_t previous_effective = 0U;
        for (std::size_t index = 0U; index < history.size(); ++index) {
            const auto& revision = history[index];
            const auto draft = draft_from_policy(revision);
            const auto* threat = threat_model(revision.threat_model_id,
                                              revision.threat_model_revision);
            if (revision.id != id || !valid_policy_draft_shape(draft)
                || !sorted_unique_by(revision.rules, rule_sort_less)
                || revision.revision != index + 1U || threat == nullptr
                || threat->revision_digest != revision.threat_model_digest
                || revision.recorded_at < threat->recorded_at
                || revision.effective_from < threat->effective_from
                || revision.review_at > threat->review_at
                || revision.recorded_at < previous_recorded
                || revision.effective_from < previous_effective
                || revision.previous_revision_digest != previous
                || revision.revision_digest
                       != policy_revision_digest(registry_id_, revision)) {
                return false;
            }
            if (std::any_of(revision.rules.begin(), revision.rules.end(), [&](const auto& rule) {
                    return (rule.deprecate_at.has_value()
                            && *rule.deprecate_at < revision.effective_from)
                           || (rule.prohibit_at.has_value()
                               && *rule.prohibit_at < revision.effective_from);
                })) {
                return false;
            }
            previous = revision.revision_digest;
            previous_recorded = revision.recorded_at;
            previous_effective = revision.effective_from;
            ++expected_policy_count;
        }
    }
    if (expected_policy_count != policy_revision_count_) {
        return false;
    }

    for (const auto& [provider_id, manifest] : providers_) {
        if (provider_id != manifest.provider_id || !valid_provider_shape(manifest)
            || !sorted_unique_by(manifest.capabilities, capability_sort_less)) {
            return false;
        }
    }

    std::size_t expected_assessment_count = 0U;
    std::map<std::string, std::string, std::less<>> expected_index;
    for (const auto& [provider_id, history] : assessments_) {
        const auto provider_found = providers_.find(provider_id);
        if (provider_found == providers_.end() || history.empty()) {
            return false;
        }
        auto state = ProviderState::declared;
        std::uint64_t previous_recorded = provider_found->second.declared_at;
        std::string previous = provider_manifest_digest(registry_id_, provider_found->second);
        for (std::size_t index = 0U; index < history.size(); ++index) {
            const auto& assessment = history[index];
            const auto draft = draft_from_assessment(assessment);
            bool transition_valid = false;
            const auto next = transitioned_state(state, assessment.kind, &transition_valid);
            const bool evaluates_routes =
                assessment.kind == ProviderAssessmentKind::observation
                || assessment.kind == ProviderAssessmentKind::qualification;
            if (assessment.provider_id != provider_id
                || !valid_assessment_draft_shape(draft)
                || !sorted_unique_by(assessment.routes, route_sort_less)
                || assessment.sequence != index + 1U
                || assessment.recorded_at < previous_recorded
                || assessment.previous_assessment_digest != previous
                || assessment.assessment_digest
                       != assessment_digest(registry_id_, assessment)
                || !transition_valid || evaluates_routes != !assessment.routes.empty()
                || !expected_index.emplace(assessment.id, provider_id).second) {
                return false;
            }

            const CryptoPolicyRevision* referenced_policy = nullptr;
            if (evaluates_routes) {
                referenced_policy = policy(assessment.policy_id, assessment.policy_revision);
                if (referenced_policy == nullptr
                    || policy_at(policies_, assessment.policy_id, assessment.recorded_at)
                           != referenced_policy
                    || std::any_of(
                        assessment.routes.begin(),
                        assessment.routes.end(),
                        [&](const auto& route) {
                            return !route_matches_capability(provider_found->second,
                                                             route)
                                   || find_rule(*referenced_policy,
                                                route.algorithm_id,
                                                route.function)
                                          == nullptr;
                        })) {
                    return false;
                }
            } else if (!assessment.policy_id.empty()
                       || assessment.policy_revision != 0U
                       || !assessment.module_validation_reference.empty()
                       || !assessment.module_validation_evidence_digest.empty()
                       || assessment.valid_until.has_value()) {
                return false;
            }

            if (assessment.kind == ProviderAssessmentKind::qualification) {
                const auto* threat = threat_model(referenced_policy->threat_model_id,
                                                  referenced_policy->threat_model_revision);
                if (threat == nullptr
                    || threat->revision_digest != referenced_policy->threat_model_digest
                    || !threat_qualification_ready(*threat)
                    || threat_at(threat_models_,
                                 threat->id,
                                 threat->revision,
                                 assessment.recorded_at)
                           == nullptr
                    || assessment.module_validation_reference.empty()
                    || assessment.module_validation_evidence_digest.empty()
                    || !assessment.valid_until.has_value()
                    || *assessment.valid_until > referenced_policy->review_at
                    || std::any_of(
                        assessment.routes.begin(),
                        assessment.routes.end(),
                        [&](const auto& route) {
                            return !route_was_observed(history, route, index)
                                   || route.algorithm_validation_reference.empty()
                                   || route.algorithm_validation_evidence_digest.empty()
                                   || !qualification_route_allowed(
                                       *referenced_policy,
                                       route,
                                       assessment.recorded_at);
                        })) {
                    return false;
                }
            }
            state = next;
            previous = assessment.assessment_digest;
            previous_recorded = assessment.recorded_at;
            ++expected_assessment_count;
        }
    }
    return expected_assessment_count == assessment_count_
           && expected_index == assessment_index_;
}

bool CryptoProviderRegistry::rebuild_indexes() {
    threat_revision_count_ = 0U;
    policy_revision_count_ = 0U;
    assessment_count_ = 0U;
    assessment_index_.clear();
    for (const auto& [unused_id, history] : threat_models_) {
        static_cast<void>(unused_id);
        if (history.size() > threat_revision_capacity_ - threat_revision_count_) {
            return false;
        }
        threat_revision_count_ += history.size();
    }
    for (const auto& [unused_id, history] : policies_) {
        static_cast<void>(unused_id);
        if (history.size() > policy_revision_capacity_ - policy_revision_count_) {
            return false;
        }
        policy_revision_count_ += history.size();
    }
    for (const auto& [provider_id, history] : assessments_) {
        if (history.size() > assessment_capacity_ - assessment_count_) {
            return false;
        }
        assessment_count_ += history.size();
        for (const auto& assessment : history) {
            if (!assessment_index_.emplace(assessment.id, provider_id).second) {
                return false;
            }
        }
    }
    return true;
}

const std::string& CryptoProviderRegistry::registry_id() const noexcept {
    return registry_id_;
}

const std::string& CryptoProviderRegistry::entity_namespace_id() const noexcept {
    return entity_namespace_id_;
}

const std::string& CryptoProviderRegistry::owner_entity_id() const noexcept {
    return owner_entity_id_;
}

std::size_t CryptoProviderRegistry::threat_revision_capacity() const noexcept {
    return threat_revision_capacity_;
}

std::size_t CryptoProviderRegistry::policy_revision_capacity() const noexcept {
    return policy_revision_capacity_;
}

std::size_t CryptoProviderRegistry::provider_capacity() const noexcept {
    return provider_capacity_;
}

std::size_t CryptoProviderRegistry::assessment_capacity() const noexcept {
    return assessment_capacity_;
}

std::size_t CryptoProviderRegistry::threat_revision_count() const noexcept {
    return threat_revision_count_;
}

std::size_t CryptoProviderRegistry::policy_revision_count() const noexcept {
    return policy_revision_count_;
}

std::size_t CryptoProviderRegistry::assessment_count() const noexcept {
    return assessment_count_;
}

const std::map<std::string,
               std::vector<CryptoThreatModelRevision>,
               std::less<>>&
CryptoProviderRegistry::threat_models() const noexcept {
    return threat_models_;
}

const std::map<std::string,
               std::vector<CryptoPolicyRevision>,
               std::less<>>&
CryptoProviderRegistry::policies() const noexcept {
    return policies_;
}

const std::map<std::string, CryptoProviderManifest, std::less<>>&
CryptoProviderRegistry::providers() const noexcept {
    return providers_;
}

const std::map<std::string,
               std::vector<CryptoProviderAssessment>,
               std::less<>>&
CryptoProviderRegistry::assessments() const noexcept {
    return assessments_;
}

} // namespace genesis::security
