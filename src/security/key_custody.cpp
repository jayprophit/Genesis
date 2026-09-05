#include "genesis/security/key_custody.hpp"

#include "genesis/runtime/runtime.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <set>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace genesis::security {
namespace {

constexpr std::size_t kMaximumTextBytes = 1024U;
constexpr std::size_t kMaximumOperators = 64U;

bool text(std::string_view value) {
    return !value.empty() && value.size() <= kMaximumTextBytes
           && std::all_of(value.begin(), value.end(), [](unsigned char character) {
                  return character >= 0x20U && character != 0x7fU;
              });
}

bool digest(std::string_view value) {
    return value.size() == 64U
           && std::all_of(value.begin(), value.end(), [](unsigned char character) {
                  return std::isxdigit(character) != 0;
              });
}

void append_material(std::string& material, std::string_view value) {
    material += std::to_string(value.size());
    material.push_back(':');
    material.append(value);
}

template <typename Integer>
void append_number(std::string& material, Integer value) {
    append_material(material, std::to_string(value));
}

template <typename Enum>
constexpr auto ordinal(Enum value) noexcept {
    return static_cast<std::underlying_type_t<Enum>>(value);
}

template <typename Enum, std::size_t Size>
std::string_view enum_name(
    Enum value,
    const std::array<std::string_view, Size>& names) noexcept {
    const auto index = static_cast<std::size_t>(ordinal(value));
    return index < names.size() ? names[index] : std::string_view{"unknown"};
}

template <typename Enum, std::size_t Size>
bool parse_enum(std::string_view text_value,
                const std::array<std::string_view, Size>& names,
                Enum& value) noexcept {
    const auto found = std::find(names.begin(), names.end(), text_value);
    if (found == names.end()) {
        return false;
    }
    value = static_cast<Enum>(std::distance(names.begin(), found));
    return true;
}

constexpr std::array<std::string_view, 5U> kOriginNames{
    "generated", "established", "derived", "imported_wrapped", "recovered"};
constexpr std::array<std::string_view, 3U> kExportPolicyNames{
    "non_exportable", "public_only", "wrapped_export_only"};
constexpr std::array<std::string_view, 10U> kUsageNames{
    "sign",
    "verify",
    "encrypt",
    "decrypt",
    "create_authentication_code",
    "verify_authentication_code",
    "derive",
    "establish",
    "wrap",
    "unwrap"};
constexpr std::array<std::string_view, 6U> kStateNames{
    "provisioned", "active", "suspended", "retired", "compromised", "destroyed"};
constexpr std::array<std::string_view, 6U> kTransitionNames{
    "activate", "suspend", "resume", "retire", "compromise", "destroy"};
constexpr std::array<std::string_view, 2U> kSuccessionNames{"rotation", "recovery"};

void set_error(KeyCustodyError* error,
               KeyCustodyErrorCode code,
               std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->message = std::move(message);
    }
}

bool usage_matches_function(KeyUsage usage, CryptoFunction function) noexcept {
    switch (function) {
    case CryptoFunction::message_authentication:
        return usage == KeyUsage::create_authentication_code
               || usage == KeyUsage::verify_authentication_code;
    case CryptoFunction::authenticated_encryption:
        return usage == KeyUsage::encrypt || usage == KeyUsage::decrypt;
    case CryptoFunction::digital_signature:
        return usage == KeyUsage::sign || usage == KeyUsage::verify;
    case CryptoFunction::key_establishment:
        return usage == KeyUsage::establish;
    case CryptoFunction::key_derivation:
        return usage == KeyUsage::derive;
    case CryptoFunction::key_wrapping:
        return usage == KeyUsage::wrap || usage == KeyUsage::unwrap;
    case CryptoFunction::secure_hash:
    case CryptoFunction::random_generation:
        return false;
    }
    return false;
}

template <typename Value>
bool sorted_unique_enums(const std::vector<Value>& values) {
    return std::adjacent_find(values.begin(), values.end(), [](Value left, Value right) {
               return ordinal(left) >= ordinal(right);
           }) == values.end();
}

bool sorted_unique_text(const std::vector<std::string>& values) {
    return std::all_of(values.begin(), values.end(), [](const std::string& value) {
               return text(value);
           })
           && std::adjacent_find(values.begin(), values.end(), std::greater_equal<>())
                  == values.end();
}

const CryptoAlgorithmCapability* capability_for(
    const CryptoProviderManifest& provider,
    const KeyHandleManifest& key) {
    const auto found = std::find_if(
        provider.capabilities.begin(),
        provider.capabilities.end(),
        [&](const CryptoAlgorithmCapability& capability) {
            return capability.algorithm_id == key.algorithm_id
                   && capability.function == key.function
                   && capability.implementation_route == key.implementation_route
                   && capability.implementation_digest == key.implementation_digest;
        });
    return found == provider.capabilities.end() ? nullptr : &*found;
}

bool manifest_shape(const KeyHandleManifest& manifest) {
    if (!text(manifest.key_id) || !text(manifest.owner_entity_id)
        || !text(manifest.provider_id) || !text(manifest.algorithm_id)
        || !text(manifest.implementation_route)
        || !digest(manifest.implementation_digest)
        || !digest(manifest.provider_locator_digest)
        || !text(manifest.custodian_entity_id)
        || !text(manifest.recovery_authority_entity_id)
        || manifest.recovery_authority_entity_id == manifest.custodian_entity_id
        || !digest(manifest.custody_policy_digest)
        || !digest(manifest.creation_evidence_digest)
        || !digest(manifest.attestation_evidence_digest)
        || manifest.permitted_usages.empty()
        || manifest.permitted_usages.size() > kUsageNames.size()
        || !sorted_unique_enums(manifest.permitted_usages)
        || manifest.operator_entity_ids.empty()
        || manifest.operator_entity_ids.size() > kMaximumOperators
        || !sorted_unique_text(manifest.operator_entity_ids)
        || !std::binary_search(manifest.operator_entity_ids.begin(),
                               manifest.operator_entity_ids.end(),
                               manifest.custodian_entity_id)
        || std::binary_search(manifest.operator_entity_ids.begin(),
                              manifest.operator_entity_ids.end(),
                              manifest.recovery_authority_entity_id)
        || manifest.generation == 0U || manifest.not_before >= manifest.not_after
        || ordinal(manifest.origin) > ordinal(KeyOrigin::recovered)
        || ordinal(manifest.export_policy) > ordinal(KeyExportPolicy::wrapped_export_only)
        || !crypto_function_requires_key_material(manifest.function)) {
        return false;
    }
    if ((manifest.generation == 1U) != manifest.predecessor_key_id.empty()) {
        return false;
    }
    if (!manifest.predecessor_key_id.empty()
        && (!text(manifest.predecessor_key_id)
            || manifest.predecessor_key_id == manifest.key_id)) {
        return false;
    }
    if (manifest.origin == KeyOrigin::recovered && manifest.predecessor_key_id.empty()) {
        return false;
    }
    return std::all_of(manifest.permitted_usages.begin(),
                       manifest.permitted_usages.end(),
                       [&](KeyUsage usage) {
                           return ordinal(usage) <= ordinal(KeyUsage::unwrap)
                                  && usage_matches_function(usage, manifest.function);
                       });
}

std::string transition_material(std::string_view registry_id,
                                const KeyTransitionRecord& record) {
    std::string material{"genesis-key-transition-v1"};
    append_material(material, registry_id);
    append_material(material, record.id);
    append_material(material, record.key_id);
    append_number(material, ordinal(record.kind));
    append_number(material, ordinal(record.state_after));
    append_material(material, record.actor_entity_id);
    append_material(material, record.reason_digest);
    append_material(material, record.evidence_digest);
    append_number(material, record.sequence);
    append_number(material, record.occurred_at);
    append_number(material, record.recorded_at);
    append_material(material, record.previous_transition_digest);
    return material;
}

std::string succession_material(std::string_view registry_id,
                                const KeySuccessionRecord& record) {
    std::string material{"genesis-key-succession-v1"};
    append_material(material, registry_id);
    append_material(material, record.id);
    append_number(material, ordinal(record.kind));
    append_material(material, record.predecessor_key_id);
    append_material(material, record.successor_key_id);
    append_material(material, record.actor_entity_id);
    append_material(material, record.reason_digest);
    append_material(material, record.evidence_digest);
    append_number(material, record.sequence);
    append_number(material, record.occurred_at);
    append_number(material, record.recorded_at);
    append_material(material, record.previous_succession_digest);
    return material;
}

std::optional<KeyLifecycleState> transition_state(KeyLifecycleState current,
                                                  KeyTransitionKind kind) {
    switch (current) {
    case KeyLifecycleState::provisioned:
        if (kind == KeyTransitionKind::activate) {
            return KeyLifecycleState::active;
        }
        if (kind == KeyTransitionKind::compromise) {
            return KeyLifecycleState::compromised;
        }
        if (kind == KeyTransitionKind::destroy) {
            return KeyLifecycleState::destroyed;
        }
        break;
    case KeyLifecycleState::active:
        if (kind == KeyTransitionKind::suspend) {
            return KeyLifecycleState::suspended;
        }
        if (kind == KeyTransitionKind::retire) {
            return KeyLifecycleState::retired;
        }
        if (kind == KeyTransitionKind::compromise) {
            return KeyLifecycleState::compromised;
        }
        break;
    case KeyLifecycleState::suspended:
        if (kind == KeyTransitionKind::resume) {
            return KeyLifecycleState::active;
        }
        if (kind == KeyTransitionKind::retire) {
            return KeyLifecycleState::retired;
        }
        if (kind == KeyTransitionKind::compromise) {
            return KeyLifecycleState::compromised;
        }
        break;
    case KeyLifecycleState::retired:
        if (kind == KeyTransitionKind::compromise) {
            return KeyLifecycleState::compromised;
        }
        [[fallthrough]];
    case KeyLifecycleState::compromised:
        if (kind == KeyTransitionKind::destroy) {
            return KeyLifecycleState::destroyed;
        }
        break;
    case KeyLifecycleState::destroyed:
        break;
    }
    return std::nullopt;
}

bool role_matches(const KeyHandleManifest& manifest,
                  KeyTransitionKind kind,
                  std::string_view actor) {
    if (actor == manifest.custodian_entity_id) {
        return true;
    }
    if (kind == KeyTransitionKind::compromise
        && actor == manifest.recovery_authority_entity_id) {
        return true;
    }
    if ((kind == KeyTransitionKind::suspend
         || kind == KeyTransitionKind::compromise)
        && std::binary_search(manifest.operator_entity_ids.begin(),
                              manifest.operator_entity_ids.end(),
                              actor)) {
        return true;
    }
    if (kind == KeyTransitionKind::destroy
        && actor == manifest.recovery_authority_entity_id) {
        return true;
    }
    return false;
}

bool actor_exists_at(const identity::EntityRegistry& entities,
                     std::string_view actor,
                     std::uint64_t at) {
    const auto* entity = entities.find_entity(actor);
    return entity != nullptr && entity->registered_at <= at;
}

bool compromised_at(const KeyCustodyRegistry& registry,
                    std::string_view key_id,
                    std::uint64_t at) {
    const auto* history = registry.transition_history(key_id);
    return history != nullptr
           && std::any_of(history->begin(), history->end(), [&](const auto& record) {
                  return record.kind == KeyTransitionKind::compromise
                         && record.occurred_at <= at && record.recorded_at <= at;
              });
}

bool provider_binding_matches(const KeyHandleManifest& key,
                              const CryptoProviderDecision& decision) {
    return decision.provider_id == key.provider_id
           && decision.algorithm_id == key.algorithm_id
           && decision.function == key.function
           && decision.implementation_route == key.implementation_route
           && decision.implementation_digest == key.implementation_digest;
}

} // namespace

std::string_view to_string(KeyOrigin value) noexcept {
    return enum_name(value, kOriginNames);
}

std::string_view to_string(KeyExportPolicy value) noexcept {
    return enum_name(value, kExportPolicyNames);
}

std::string_view to_string(KeyUsage value) noexcept {
    return enum_name(value, kUsageNames);
}

std::string_view to_string(KeyLifecycleState value) noexcept {
    return enum_name(value, kStateNames);
}

std::string_view to_string(KeyTransitionKind value) noexcept {
    return enum_name(value, kTransitionNames);
}

std::string_view to_string(KeySuccessionKind value) noexcept {
    return enum_name(value, kSuccessionNames);
}

bool key_origin_from_string(std::string_view text_value, KeyOrigin& value) noexcept {
    return parse_enum(text_value, kOriginNames, value);
}

bool key_export_policy_from_string(std::string_view text_value,
                                   KeyExportPolicy& value) noexcept {
    return parse_enum(text_value, kExportPolicyNames, value);
}

bool key_usage_from_string(std::string_view text_value, KeyUsage& value) noexcept {
    return parse_enum(text_value, kUsageNames, value);
}

bool key_lifecycle_state_from_string(std::string_view text_value,
                                     KeyLifecycleState& value) noexcept {
    return parse_enum(text_value, kStateNames, value);
}

bool key_transition_kind_from_string(std::string_view text_value,
                                     KeyTransitionKind& value) noexcept {
    return parse_enum(text_value, kTransitionNames, value);
}

bool key_succession_kind_from_string(std::string_view text_value,
                                     KeySuccessionKind& value) noexcept {
    return parse_enum(text_value, kSuccessionNames, value);
}

bool KeyCustodyAudit::clean() const noexcept {
    return registry_errors == 0U && owner_mismatches == 0U
           && missing_custodians == 0U && missing_recovery_authorities == 0U
           && missing_operators == 0U && temporal_mismatches == 0U;
}

std::string derive_key_custody_registry_id(std::string_view entity_namespace_id,
                                           std::string_view owner_entity_id) {
    if (!text(entity_namespace_id) || !text(owner_entity_id)) {
        throw std::invalid_argument("invalid key-custody registry identity material");
    }
    std::string material{"genesis-key-custody-registry-v1"};
    append_material(material, entity_namespace_id);
    append_material(material, owner_entity_id);
    return "key-custody-" + runtime::sha256(material);
}

std::string derive_key_handle_id(std::string_view owner_entity_id,
                                 std::string_view provider_id,
                                 std::string_view algorithm_id,
                                 CryptoFunction function,
                                 std::string_view implementation_route,
                                 std::string_view implementation_digest,
                                 std::string_view provider_locator_digest,
                                 std::uint64_t generation) {
    if (!text(owner_entity_id) || !text(provider_id) || !text(algorithm_id)
        || !text(implementation_route) || !digest(implementation_digest)
        || !digest(provider_locator_digest) || generation == 0U
        || ordinal(function) > ordinal(CryptoFunction::key_wrapping)) {
        throw std::invalid_argument("invalid key-handle identity material");
    }
    std::string material{"genesis-key-handle-v1"};
    append_material(material, owner_entity_id);
    append_material(material, provider_id);
    append_material(material, algorithm_id);
    append_number(material, ordinal(function));
    append_material(material, implementation_route);
    append_material(material, implementation_digest);
    append_material(material, provider_locator_digest);
    append_number(material, generation);
    return "key-" + runtime::sha256(material);
}

KeyCustodyRegistry::KeyCustodyRegistry(std::string registry_id,
                                       std::string entity_namespace_id,
                                       std::string owner_entity_id,
                                       std::size_t key_capacity,
                                       std::size_t transition_capacity,
                                       std::size_t succession_capacity)
    : registry_id_(std::move(registry_id)),
      entity_namespace_id_(std::move(entity_namespace_id)),
      owner_entity_id_(std::move(owner_entity_id)),
      key_capacity_(key_capacity),
      transition_capacity_(transition_capacity),
      succession_capacity_(succession_capacity) {
    constexpr auto maximum_items = std::size_t{1'000'000U};
    if (registry_id_
            != derive_key_custody_registry_id(entity_namespace_id_, owner_entity_id_)
        || key_capacity_ == 0U || transition_capacity_ == 0U
        || succession_capacity_ == 0U || key_capacity_ > maximum_items
        || transition_capacity_ > maximum_items
        || succession_capacity_ > maximum_items) {
        throw std::invalid_argument("invalid key-custody registry configuration");
    }
}

bool KeyCustodyRegistry::register_key(
    KeyHandleManifest manifest,
    const CryptoProviderRegistry& providers,
    const identity::EntityRegistry& entities,
    KeyCustodyError* error) {
    if (!entities.verify() || !providers.verify()) {
        set_error(error,
                  KeyCustodyErrorCode::invalid_manifest,
                  "a prerequisite registry is invalid");
        return false;
    }
    if (entities.namespace_id() != entity_namespace_id_
        || providers.entity_namespace_id() != entity_namespace_id_
        || providers.owner_entity_id() != owner_entity_id_
        || manifest.owner_entity_id != owner_entity_id_) {
        set_error(error,
                  KeyCustodyErrorCode::owner_binding_mismatch,
                  "key manifest owner or namespace does not match its registries");
        return false;
    }
    if (!manifest_shape(manifest)) {
        set_error(error,
                  KeyCustodyErrorCode::invalid_manifest,
                  "key manifest is malformed or contains an invalid key-use contract");
        return false;
    }
    const auto expected_id = derive_key_handle_id(manifest.owner_entity_id,
                                                   manifest.provider_id,
                                                   manifest.algorithm_id,
                                                   manifest.function,
                                                   manifest.implementation_route,
                                                   manifest.implementation_digest,
                                                   manifest.provider_locator_digest,
                                                   manifest.generation);
    if (manifest.key_id != expected_id) {
        set_error(error,
                  KeyCustodyErrorCode::invalid_manifest,
                  "key identifier does not bind the exact external handle evidence");
        return false;
    }
    if (keys_.contains(manifest.key_id)) {
        set_error(error, KeyCustodyErrorCode::duplicate_entry, "key already registered");
        return false;
    }
    if (keys_.size() >= key_capacity_) {
        set_error(error, KeyCustodyErrorCode::capacity_exceeded, "key capacity exceeded");
        return false;
    }
    const auto* provider = providers.provider(manifest.provider_id);
    if (provider == nullptr || capability_for(*provider, manifest) == nullptr) {
        set_error(error,
                  KeyCustodyErrorCode::provider_binding_mismatch,
                  "key does not bind an exact declared provider route");
        return false;
    }
    const auto* owner = entities.find_entity(owner_entity_id_);
    const auto* custodian = entities.find_entity(manifest.custodian_entity_id);
    const auto* recovery = entities.find_entity(manifest.recovery_authority_entity_id);
    if (owner == nullptr || custodian == nullptr || recovery == nullptr) {
        set_error(error,
                  KeyCustodyErrorCode::missing_entity,
                  "owner, custodian or recovery authority is not registered");
        return false;
    }
    if (owner->kind != identity::EntityKind::organism
        && owner->kind != identity::EntityKind::organization) {
        set_error(error,
                  KeyCustodyErrorCode::owner_binding_mismatch,
                  "key registry owner must be an organism or organization");
        return false;
    }
    if (owner->registered_at > manifest.registered_at
        || custodian->registered_at > manifest.registered_at
        || recovery->registered_at > manifest.registered_at) {
        set_error(error,
                  KeyCustodyErrorCode::temporal_conflict,
                  "key role was registered after the key manifest");
        return false;
    }
    for (const auto& operator_id : manifest.operator_entity_ids) {
        if (!actor_exists_at(entities, operator_id, manifest.registered_at)) {
            set_error(error,
                      KeyCustodyErrorCode::missing_entity,
                      "key operator is missing or temporally invalid");
            return false;
        }
    }
    if (!manifest.predecessor_key_id.empty()) {
        const auto* predecessor = key(manifest.predecessor_key_id);
        if (predecessor == nullptr || predecessor->owner_entity_id != manifest.owner_entity_id
            || predecessor->function != manifest.function
            || predecessor->generation == std::numeric_limits<std::uint64_t>::max()
            || manifest.generation != predecessor->generation + 1U
            || predecessor->registered_at >= manifest.registered_at
            || predecessor->provider_locator_digest
                   == manifest.provider_locator_digest) {
            set_error(error,
                      KeyCustodyErrorCode::succession_conflict,
                      "key predecessor binding is invalid");
            return false;
        }
    }
    const auto key_id = manifest.key_id;
    keys_.emplace(key_id, std::move(manifest));
    set_error(error, KeyCustodyErrorCode::none, {});
    return true;
}

bool KeyCustodyRegistry::record_transition(
    KeyTransitionDraft draft,
    const CryptoProviderRegistry& providers,
    const identity::EntityRegistry& entities,
    KeyCustodyError* error) {
    if (!entities.verify() || !providers.verify()) {
        set_error(error,
                  KeyCustodyErrorCode::transition_conflict,
                  "a prerequisite registry is invalid");
        return false;
    }
    const auto* manifest = key(draft.key_id);
    if (manifest == nullptr) {
        set_error(error, KeyCustodyErrorCode::missing_key, "key is not registered");
        return false;
    }
    if (!text(draft.id) || !text(draft.actor_entity_id)
        || !digest(draft.reason_digest) || !digest(draft.evidence_digest)
        || draft.occurred_at > draft.recorded_at
        || draft.recorded_at < manifest->registered_at
        || ordinal(draft.kind) > ordinal(KeyTransitionKind::destroy)) {
        set_error(error,
                  KeyCustodyErrorCode::transition_conflict,
                  "key transition is malformed or temporally invalid");
        return false;
    }
    if (transition_index_.contains(draft.id)) {
        set_error(error,
                  KeyCustodyErrorCode::duplicate_entry,
                  "key transition identifier already exists");
        return false;
    }
    if (transition_count_ >= transition_capacity_) {
        set_error(error,
                  KeyCustodyErrorCode::capacity_exceeded,
                  "key transition capacity exceeded");
        return false;
    }
    if (!actor_exists_at(entities, draft.actor_entity_id, draft.occurred_at)) {
        set_error(error,
                  KeyCustodyErrorCode::missing_entity,
                  "transition actor is missing or temporally invalid");
        return false;
    }
    if (!role_matches(*manifest, draft.kind, draft.actor_entity_id)) {
        set_error(error,
                  KeyCustodyErrorCode::role_mismatch,
                  "transition actor is not recorded for the required custody role");
        return false;
    }
    const auto* history = transition_history(draft.key_id);
    const auto current = history == nullptr || history->empty()
                             ? KeyLifecycleState::provisioned
                             : history->back().state_after;
    const auto next = transition_state(current, draft.kind);
    if (!next.has_value()) {
        set_error(error,
                  KeyCustodyErrorCode::transition_conflict,
                  "key lifecycle transition is not permitted");
        return false;
    }
    if (history != nullptr && !history->empty()
        && (draft.recorded_at <= history->back().recorded_at
            || draft.occurred_at < history->back().occurred_at)) {
        set_error(error,
                  KeyCustodyErrorCode::temporal_conflict,
                  "key transition history is not monotonic");
        return false;
    }
    if (draft.kind == KeyTransitionKind::activate
        || draft.kind == KeyTransitionKind::resume) {
        if (draft.occurred_at < manifest->not_before
            || draft.occurred_at >= manifest->not_after) {
            set_error(error,
                      KeyCustodyErrorCode::temporal_conflict,
                      "key activation is outside its cryptoperiod");
            return false;
        }
        const auto provider_decision = providers.evaluate(manifest->provider_id,
                                                          manifest->algorithm_id,
                                                          manifest->function,
                                                          draft.occurred_at);
        if (!provider_decision.integration_candidate) {
            set_error(error,
                      KeyCustodyErrorCode::provider_not_qualified,
                      "provider route is not a current integration candidate");
            return false;
        }
        if (!provider_binding_matches(*manifest, provider_decision)) {
            set_error(error,
                      KeyCustodyErrorCode::provider_binding_mismatch,
                      "qualified provider evidence binds a different route");
            return false;
        }
        if (provider_decision.key_custody_evidence_digest
            != manifest->custody_policy_digest) {
            set_error(error,
                      KeyCustodyErrorCode::custody_policy_mismatch,
                      "qualified route binds a different custody policy");
            return false;
        }
    }

    KeyTransitionRecord record;
    record.id = std::move(draft.id);
    record.key_id = std::move(draft.key_id);
    record.kind = draft.kind;
    record.state_after = *next;
    record.actor_entity_id = std::move(draft.actor_entity_id);
    record.reason_digest = std::move(draft.reason_digest);
    record.evidence_digest = std::move(draft.evidence_digest);
    record.sequence = history == nullptr ? 1U : history->size() + 1U;
    record.occurred_at = draft.occurred_at;
    record.recorded_at = draft.recorded_at;
    record.previous_transition_digest =
        history == nullptr ? std::string(64U, '0') : history->back().transition_digest;
    record.transition_digest = runtime::sha256(transition_material(registry_id_, record));

    const auto transition_id = record.id;
    const auto key_id = record.key_id;
    transitions_[key_id].push_back(std::move(record));
    transition_index_.emplace(transition_id, key_id);
    ++transition_count_;
    set_error(error, KeyCustodyErrorCode::none, {});
    return true;
}

bool KeyCustodyRegistry::record_succession(
    KeySuccessionDraft draft,
    const identity::EntityRegistry& entities,
    KeyCustodyError* error) {
    if (!entities.verify()) {
        set_error(error,
                  KeyCustodyErrorCode::succession_conflict,
                  "a prerequisite registry is invalid");
        return false;
    }
    const auto* predecessor = key(draft.predecessor_key_id);
    const auto* successor = key(draft.successor_key_id);
    if (predecessor == nullptr || successor == nullptr) {
        set_error(error,
                  KeyCustodyErrorCode::missing_key,
                  "succession endpoint is not registered");
        return false;
    }
    if (!text(draft.id) || !text(draft.actor_entity_id)
        || !digest(draft.reason_digest) || !digest(draft.evidence_digest)
        || draft.predecessor_key_id == draft.successor_key_id
        || draft.occurred_at > draft.recorded_at
        || ordinal(draft.kind) > ordinal(KeySuccessionKind::recovery)) {
        set_error(error,
                  KeyCustodyErrorCode::succession_conflict,
                  "key succession record is malformed");
        return false;
    }
    if (successions_.size() >= succession_capacity_) {
        set_error(error,
                  KeyCustodyErrorCode::capacity_exceeded,
                  "key succession capacity exceeded");
        return false;
    }
    if (succession_id_index_.contains(draft.id)
        || predecessor_index_.contains(draft.predecessor_key_id)
        || successor_index_.contains(draft.successor_key_id)) {
        set_error(error,
                  KeyCustodyErrorCode::duplicate_entry,
                  "key succession identifier or endpoint is already linked");
        return false;
    }
    if (!successions_.empty()
        && (draft.recorded_at <= successions_.back().recorded_at
            || draft.occurred_at < successions_.back().occurred_at)) {
        set_error(error,
                  KeyCustodyErrorCode::temporal_conflict,
                  "key succession history is not monotonic");
        return false;
    }
    if (!actor_exists_at(entities, draft.actor_entity_id, draft.occurred_at)) {
        set_error(error,
                  KeyCustodyErrorCode::missing_entity,
                  "succession actor is missing or temporally invalid");
        return false;
    }
    if (successor->predecessor_key_id != predecessor->key_id
        || successor->owner_entity_id != predecessor->owner_entity_id
        || successor->function != predecessor->function
        || predecessor->generation == std::numeric_limits<std::uint64_t>::max()
        || successor->generation != predecessor->generation + 1U
        || draft.occurred_at < successor->registered_at
        || successor->provider_locator_digest
               == predecessor->provider_locator_digest) {
        set_error(error,
                  KeyCustodyErrorCode::succession_conflict,
                  "successor manifest does not continue the predecessor lineage");
        return false;
    }
    const auto predecessor_state = key_state_at(predecessor->key_id, draft.occurred_at);
    const auto successor_state = key_state_at(successor->key_id, draft.occurred_at);
    if (successor_state != KeyLifecycleState::active) {
        set_error(error,
                  KeyCustodyErrorCode::succession_conflict,
                  "successor key is not active at succession recording time");
        return false;
    }
    if (draft.kind == KeySuccessionKind::rotation) {
        if (draft.actor_entity_id != predecessor->custodian_entity_id
            || predecessor_state != KeyLifecycleState::retired
            || successor->origin == KeyOrigin::recovered) {
            set_error(error,
                      KeyCustodyErrorCode::role_mismatch,
                      "rotation requires the custodian, retired predecessor and non-recovery successor");
            return false;
        }
    } else if (draft.actor_entity_id != predecessor->recovery_authority_entity_id
               || (predecessor_state != KeyLifecycleState::compromised
                   && predecessor_state != KeyLifecycleState::destroyed)
               || !compromised_at(*this, predecessor->key_id, draft.occurred_at)
               || successor->origin != KeyOrigin::recovered) {
        set_error(error,
                  KeyCustodyErrorCode::role_mismatch,
                  "recovery requires its recorded authority and a recovered successor");
        return false;
    }

    KeySuccessionRecord record;
    record.id = std::move(draft.id);
    record.kind = draft.kind;
    record.predecessor_key_id = std::move(draft.predecessor_key_id);
    record.successor_key_id = std::move(draft.successor_key_id);
    record.actor_entity_id = std::move(draft.actor_entity_id);
    record.reason_digest = std::move(draft.reason_digest);
    record.evidence_digest = std::move(draft.evidence_digest);
    record.sequence = successions_.size() + 1U;
    record.occurred_at = draft.occurred_at;
    record.recorded_at = draft.recorded_at;
    record.previous_succession_digest = successions_.empty()
                                                ? std::string(64U, '0')
                                                : successions_.back().succession_digest;
    record.succession_digest = runtime::sha256(succession_material(registry_id_, record));

    const auto index = successions_.size();
    successions_.push_back(std::move(record));
    succession_id_index_.emplace(successions_.back().id, index);
    predecessor_index_.emplace(successions_.back().predecessor_key_id, index);
    successor_index_.emplace(successions_.back().successor_key_id, index);
    set_error(error, KeyCustodyErrorCode::none, {});
    return true;
}

const KeyHandleManifest* KeyCustodyRegistry::key(std::string_view key_id) const {
    const auto found = keys_.find(key_id);
    return found == keys_.end() ? nullptr : &found->second;
}

const std::vector<KeyTransitionRecord>* KeyCustodyRegistry::transition_history(
    std::string_view key_id) const {
    const auto found = transitions_.find(key_id);
    return found == transitions_.end() ? nullptr : &found->second;
}

KeyLifecycleState KeyCustodyRegistry::key_state(std::string_view key_id) const {
    const auto* history = transition_history(key_id);
    return history == nullptr || history->empty() ? KeyLifecycleState::provisioned
                                                  : history->back().state_after;
}

KeyLifecycleState KeyCustodyRegistry::key_state_at(std::string_view key_id,
                                                   std::uint64_t at) const {
    auto state = KeyLifecycleState::provisioned;
    const auto* history = transition_history(key_id);
    if (history != nullptr) {
        for (const auto& record : *history) {
            if (record.recorded_at > at || record.occurred_at > at) {
                break;
            }
            state = record.state_after;
        }
    }
    return state;
}

const KeySuccessionRecord* KeyCustodyRegistry::succession_from(
    std::string_view predecessor_key_id) const {
    const auto found = predecessor_index_.find(predecessor_key_id);
    return found == predecessor_index_.end() ? nullptr : &successions_[found->second];
}

const KeySuccessionRecord* KeyCustodyRegistry::succession_to(
    std::string_view successor_key_id) const {
    const auto found = successor_index_.find(successor_key_id);
    return found == successor_index_.end() ? nullptr : &successions_[found->second];
}

KeyOperationDecision KeyCustodyRegistry::preflight(
    const KeyOperationRequest& request,
    const CryptoProviderRegistry& providers,
    const identity::EntityRegistry& entities) const {
    KeyOperationDecision decision;
    decision.registry_valid = verify();
    if (!decision.registry_valid) {
        decision.reason = "key_custody_registry_invalid";
        return decision;
    }
    decision.entity_registry_valid = entities.verify();
    if (!decision.entity_registry_valid) {
        decision.reason = "entity_registry_invalid";
        return decision;
    }
    const auto* manifest = key(request.key_id);
    if (manifest == nullptr) {
        decision.reason = "key_not_registered";
        return decision;
    }
    decision.key_found = true;
    decision.owner_bound = entities.namespace_id() == entity_namespace_id_
                           && providers.entity_namespace_id() == entity_namespace_id_
                           && providers.owner_entity_id() == owner_entity_id_
                           && manifest->owner_entity_id == owner_entity_id_;
    if (!decision.owner_bound) {
        decision.reason = "key_owner_binding_mismatch";
        return decision;
    }
    decision.actor_recorded =
        actor_exists_at(entities, request.actor_entity_id, request.requested_at);
    if (!decision.actor_recorded) {
        decision.reason = "operation_actor_not_recorded";
        return decision;
    }
    decision.actor_role_matched =
        request.actor_entity_id == manifest->custodian_entity_id
        || std::binary_search(manifest->operator_entity_ids.begin(),
                              manifest->operator_entity_ids.end(),
                              request.actor_entity_id);
    decision.state = key_state_at(request.key_id, request.requested_at);
    decision.active = decision.state == KeyLifecycleState::active;
    decision.within_cryptoperiod = request.requested_at >= manifest->not_before
                                  && request.requested_at < manifest->not_after;
    decision.usage_permitted =
        std::binary_search(manifest->permitted_usages.begin(),
                           manifest->permitted_usages.end(),
                           request.usage,
                           [](KeyUsage left, KeyUsage right) {
                               return ordinal(left) < ordinal(right);
                           });
    decision.operation_context_recorded = digest(request.operation_context_digest);

    decision.provider_registry_valid = providers.verify();
    if (!decision.provider_registry_valid) {
        decision.reason = "provider_registry_invalid";
        return decision;
    }
    const auto provider_decision = providers.evaluate(manifest->provider_id,
                                                      manifest->algorithm_id,
                                                      manifest->function,
                                                      request.requested_at);
    decision.provider_integration_candidate = provider_decision.integration_candidate;
    decision.provider_route_bound = provider_binding_matches(*manifest, provider_decision);
    decision.custody_policy_bound =
        provider_decision.key_custody_evidence_digest
        == manifest->custody_policy_digest;
    decision.execution_candidate =
        decision.actor_role_matched && decision.active && decision.within_cryptoperiod
        && decision.usage_permitted && decision.operation_context_recorded
        && decision.provider_integration_candidate && decision.provider_route_bound
        && decision.custody_policy_bound;

    if (decision.execution_candidate) {
        decision.reason = "evidence_gated_execution_candidate_only";
    } else if (!decision.actor_role_matched) {
        decision.reason = "operation_actor_role_not_recorded";
    } else if (!decision.active) {
        decision.reason = "key_not_active";
    } else if (!decision.within_cryptoperiod) {
        decision.reason = "key_outside_cryptoperiod";
    } else if (!decision.usage_permitted) {
        decision.reason = "key_usage_not_permitted";
    } else if (!decision.operation_context_recorded) {
        decision.reason = "operation_context_digest_missing";
    } else if (!decision.provider_integration_candidate) {
        decision.reason = "provider_not_an_integration_candidate";
    } else if (!decision.provider_route_bound) {
        decision.reason = "provider_route_binding_mismatch";
    } else {
        decision.reason = "custody_policy_binding_mismatch";
    }
    return decision;
}

KeyCustodyAudit KeyCustodyRegistry::audit_entities(
    const identity::EntityRegistry& entities) const {
    KeyCustodyAudit audit;
    if (!verify() || !entities.verify()) {
        ++audit.registry_errors;
    }
    const auto* owner = entities.find_entity(owner_entity_id_);
    if (entities.namespace_id() != entity_namespace_id_ || owner == nullptr
        || (owner->kind != identity::EntityKind::organism
            && owner->kind != identity::EntityKind::organization)) {
        ++audit.owner_mismatches;
    }
    for (const auto& [unused, manifest] : keys_) {
        static_cast<void>(unused);
        const auto* custodian = entities.find_entity(manifest.custodian_entity_id);
        const auto* recovery = entities.find_entity(manifest.recovery_authority_entity_id);
        if (custodian == nullptr) {
            ++audit.missing_custodians;
        } else if (custodian->registered_at > manifest.registered_at) {
            ++audit.temporal_mismatches;
        }
        if (recovery == nullptr) {
            ++audit.missing_recovery_authorities;
        } else if (recovery->registered_at > manifest.registered_at) {
            ++audit.temporal_mismatches;
        }
        for (const auto& operator_id : manifest.operator_entity_ids) {
            const auto* operator_entity = entities.find_entity(operator_id);
            if (operator_entity == nullptr) {
                ++audit.missing_operators;
            } else if (operator_entity->registered_at > manifest.registered_at) {
                ++audit.temporal_mismatches;
            }
        }
    }
    for (const auto& [unused, history] : transitions_) {
        static_cast<void>(unused);
        for (const auto& record : history) {
            if (!actor_exists_at(entities, record.actor_entity_id, record.occurred_at)) {
                ++audit.temporal_mismatches;
            }
        }
    }
    for (const auto& record : successions_) {
        if (!actor_exists_at(entities, record.actor_entity_id, record.occurred_at)) {
            ++audit.temporal_mismatches;
        }
    }
    return audit;
}

bool KeyCustodyRegistry::verify() const {
    if (!text(registry_id_) || !text(entity_namespace_id_) || !text(owner_entity_id_)
        || registry_id_
               != derive_key_custody_registry_id(entity_namespace_id_, owner_entity_id_)
        || key_capacity_ == 0U || transition_capacity_ == 0U
        || succession_capacity_ == 0U || keys_.size() > key_capacity_
        || transition_count_ > transition_capacity_
        || successions_.size() > succession_capacity_) {
        return false;
    }

    for (const auto& [key_id, manifest] : keys_) {
        if (key_id != manifest.key_id || manifest.owner_entity_id != owner_entity_id_
            || !manifest_shape(manifest)
            || manifest.key_id
                   != derive_key_handle_id(manifest.owner_entity_id,
                                           manifest.provider_id,
                                           manifest.algorithm_id,
                                           manifest.function,
                                           manifest.implementation_route,
                                           manifest.implementation_digest,
                                           manifest.provider_locator_digest,
                                           manifest.generation)) {
            return false;
        }
        if (!manifest.predecessor_key_id.empty()) {
            const auto found = keys_.find(manifest.predecessor_key_id);
            if (found == keys_.end() || found->second.owner_entity_id != owner_entity_id_
                || found->second.function != manifest.function
                || found->second.generation == std::numeric_limits<std::uint64_t>::max()
                || manifest.generation != found->second.generation + 1U
                || found->second.registered_at >= manifest.registered_at
                || found->second.provider_locator_digest
                       == manifest.provider_locator_digest) {
                return false;
            }
        }
    }

    std::map<std::string, std::string, std::less<>> expected_transition_index;
    std::size_t expected_transition_count = 0U;
    for (const auto& [key_id, history] : transitions_) {
        if (!keys_.contains(key_id) || history.empty()) {
            return false;
        }
        auto state = KeyLifecycleState::provisioned;
        auto previous = std::string(64U, '0');
        std::uint64_t previous_occurred = 0U;
        std::uint64_t previous_recorded = 0U;
        std::uint64_t sequence = 1U;
        for (const auto& record : history) {
            const auto next = transition_state(state, record.kind);
            if (!next.has_value() || record.key_id != key_id || !text(record.id)
                || !text(record.actor_entity_id) || !digest(record.reason_digest)
                || !digest(record.evidence_digest) || record.sequence != sequence
                || record.state_after != *next
                || record.previous_transition_digest != previous
                || record.transition_digest
                       != runtime::sha256(transition_material(registry_id_, record))
                || record.occurred_at > record.recorded_at
                || record.recorded_at < keys_.at(key_id).registered_at
                || !role_matches(keys_.at(key_id), record.kind, record.actor_entity_id)
                || ((record.kind == KeyTransitionKind::activate
                     || record.kind == KeyTransitionKind::resume)
                    && (record.occurred_at < keys_.at(key_id).not_before
                        || record.occurred_at >= keys_.at(key_id).not_after))
                || (sequence > 1U
                    && (record.occurred_at < previous_occurred
                        || record.recorded_at <= previous_recorded))
                || !expected_transition_index.emplace(record.id, key_id).second) {
                return false;
            }
            state = *next;
            previous = record.transition_digest;
            previous_occurred = record.occurred_at;
            previous_recorded = record.recorded_at;
            ++sequence;
            ++expected_transition_count;
        }
    }
    if (expected_transition_count != transition_count_
        || expected_transition_index != transition_index_) {
        return false;
    }

    std::map<std::string, std::size_t, std::less<>> expected_id_index;
    std::map<std::string, std::size_t, std::less<>> expected_predecessors;
    std::map<std::string, std::size_t, std::less<>> expected_successors;
    auto previous = std::string(64U, '0');
    std::uint64_t previous_occurred = 0U;
    std::uint64_t previous_recorded = 0U;
    for (std::size_t index = 0U; index < successions_.size(); ++index) {
        const auto& record = successions_[index];
        const auto predecessor = keys_.find(record.predecessor_key_id);
        const auto successor = keys_.find(record.successor_key_id);
        if (predecessor == keys_.end() || successor == keys_.end()
            || !text(record.id) || !text(record.actor_entity_id)
            || !digest(record.reason_digest) || !digest(record.evidence_digest)
            || record.predecessor_key_id == record.successor_key_id
            || record.sequence != index + 1U
            || record.previous_succession_digest != previous
            || record.succession_digest
                   != runtime::sha256(succession_material(registry_id_, record))
            || record.occurred_at > record.recorded_at
            || (index > 0U
                && (record.occurred_at < previous_occurred
                    || record.recorded_at <= previous_recorded))
            || successor->second.predecessor_key_id != predecessor->first
            || successor->second.owner_entity_id != predecessor->second.owner_entity_id
            || successor->second.function != predecessor->second.function
            || predecessor->second.generation
                   == std::numeric_limits<std::uint64_t>::max()
            || successor->second.generation != predecessor->second.generation + 1U
            || record.occurred_at < successor->second.registered_at
            || successor->second.provider_locator_digest
                   == predecessor->second.provider_locator_digest
            || key_state_at(successor->first, record.occurred_at)
                   != KeyLifecycleState::active
            || !expected_id_index.emplace(record.id, index).second
            || !expected_predecessors.emplace(record.predecessor_key_id, index).second
            || !expected_successors.emplace(record.successor_key_id, index).second) {
            return false;
        }
        const auto predecessor_state = key_state_at(predecessor->first, record.occurred_at);
        if (record.kind == KeySuccessionKind::rotation) {
            if (record.actor_entity_id != predecessor->second.custodian_entity_id
                || predecessor_state != KeyLifecycleState::retired
                || successor->second.origin == KeyOrigin::recovered) {
                return false;
            }
        } else if (record.kind == KeySuccessionKind::recovery) {
            if (record.actor_entity_id
                    != predecessor->second.recovery_authority_entity_id
                || (predecessor_state != KeyLifecycleState::compromised
                    && predecessor_state != KeyLifecycleState::destroyed)
                || !compromised_at(*this, predecessor->first, record.occurred_at)
                || successor->second.origin != KeyOrigin::recovered) {
                return false;
            }
        } else {
            return false;
        }
        previous = record.succession_digest;
        previous_occurred = record.occurred_at;
        previous_recorded = record.recorded_at;
    }
    return expected_id_index == succession_id_index_
           && expected_predecessors == predecessor_index_
           && expected_successors == successor_index_;
}

bool KeyCustodyRegistry::rebuild_indexes() {
    transition_index_.clear();
    succession_id_index_.clear();
    predecessor_index_.clear();
    successor_index_.clear();
    transition_count_ = 0U;
    for (const auto& [key_id, history] : transitions_) {
        for (const auto& record : history) {
            if (!transition_index_.emplace(record.id, key_id).second) {
                return false;
            }
            ++transition_count_;
        }
    }
    for (std::size_t index = 0U; index < successions_.size(); ++index) {
        const auto& record = successions_[index];
        if (!succession_id_index_.emplace(record.id, index).second
            || !predecessor_index_.emplace(record.predecessor_key_id, index).second
            || !successor_index_.emplace(record.successor_key_id, index).second) {
            return false;
        }
    }
    return true;
}

const std::string& KeyCustodyRegistry::registry_id() const noexcept {
    return registry_id_;
}

const std::string& KeyCustodyRegistry::entity_namespace_id() const noexcept {
    return entity_namespace_id_;
}

const std::string& KeyCustodyRegistry::owner_entity_id() const noexcept {
    return owner_entity_id_;
}

std::size_t KeyCustodyRegistry::key_capacity() const noexcept {
    return key_capacity_;
}

std::size_t KeyCustodyRegistry::transition_capacity() const noexcept {
    return transition_capacity_;
}

std::size_t KeyCustodyRegistry::succession_capacity() const noexcept {
    return succession_capacity_;
}

std::size_t KeyCustodyRegistry::transition_count() const noexcept {
    return transition_count_;
}

const std::map<std::string, KeyHandleManifest, std::less<>>&
KeyCustodyRegistry::keys() const noexcept {
    return keys_;
}

const std::map<std::string, std::vector<KeyTransitionRecord>, std::less<>>&
KeyCustodyRegistry::transitions() const noexcept {
    return transitions_;
}

const std::vector<KeySuccessionRecord>& KeyCustodyRegistry::successions() const noexcept {
    return successions_;
}

} // namespace genesis::security
