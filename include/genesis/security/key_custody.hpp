#pragma once

#include "genesis/identity/entity_registry.hpp"
#include "genesis/security/crypto_provider.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::security {

class KeyCustodyStore;

// Serialized enums are append-only migration boundaries. Existing values must
// never be reordered or repurposed.
enum class KeyOrigin : std::uint8_t {
    generated,
    established,
    derived,
    imported_wrapped,
    recovered,
};

enum class KeyExportPolicy : std::uint8_t {
    non_exportable,
    public_only,
    wrapped_export_only,
};

enum class KeyUsage : std::uint8_t {
    sign,
    verify,
    encrypt,
    decrypt,
    create_authentication_code,
    verify_authentication_code,
    derive,
    establish,
    wrap,
    unwrap,
};

enum class KeyLifecycleState : std::uint8_t {
    provisioned,
    active,
    suspended,
    retired,
    compromised,
    destroyed,
};

enum class KeyTransitionKind : std::uint8_t {
    activate,
    suspend,
    resume,
    retire,
    compromise,
    destroy,
};

enum class KeySuccessionKind : std::uint8_t {
    rotation,
    recovery,
};

[[nodiscard]] std::string_view to_string(KeyOrigin value) noexcept;
[[nodiscard]] std::string_view to_string(KeyExportPolicy value) noexcept;
[[nodiscard]] std::string_view to_string(KeyUsage value) noexcept;
[[nodiscard]] std::string_view to_string(KeyLifecycleState value) noexcept;
[[nodiscard]] std::string_view to_string(KeyTransitionKind value) noexcept;
[[nodiscard]] std::string_view to_string(KeySuccessionKind value) noexcept;
[[nodiscard]] bool key_origin_from_string(std::string_view text,
                                          KeyOrigin& value) noexcept;
[[nodiscard]] bool key_export_policy_from_string(
    std::string_view text,
    KeyExportPolicy& value) noexcept;
[[nodiscard]] bool key_usage_from_string(std::string_view text,
                                         KeyUsage& value) noexcept;
[[nodiscard]] bool key_lifecycle_state_from_string(
    std::string_view text,
    KeyLifecycleState& value) noexcept;
[[nodiscard]] bool key_transition_kind_from_string(
    std::string_view text,
    KeyTransitionKind& value) noexcept;
[[nodiscard]] bool key_succession_kind_from_string(
    std::string_view text,
    KeySuccessionKind& value) noexcept;

// This manifest deliberately contains no key bytes, passphrases, provider
// credentials, recovery secrets, wrapped keys or plaintext provider locator.
// provider_locator_digest binds an external provider-owned handle without
// making that handle part of the Genesis snapshot.
struct KeyHandleManifest final {
    std::string key_id;
    std::string owner_entity_id;
    std::string provider_id;
    std::string algorithm_id;
    CryptoFunction function{CryptoFunction::digital_signature};
    std::string implementation_route;
    std::string implementation_digest;
    std::string provider_locator_digest;
    KeyOrigin origin{KeyOrigin::generated};
    KeyExportPolicy export_policy{KeyExportPolicy::non_exportable};
    std::vector<KeyUsage> permitted_usages;
    std::string custodian_entity_id;
    std::string recovery_authority_entity_id;
    std::vector<std::string> operator_entity_ids;
    std::string custody_policy_digest;
    std::string creation_evidence_digest;
    std::string attestation_evidence_digest;
    std::string predecessor_key_id;
    std::uint64_t generation{1U};
    std::uint64_t not_before{};
    std::uint64_t not_after{};
    std::uint64_t registered_at{};

    [[nodiscard]] bool operator==(const KeyHandleManifest&) const = default;
};

struct KeyTransitionDraft final {
    std::string id;
    std::string key_id;
    KeyTransitionKind kind{KeyTransitionKind::activate};
    std::string actor_entity_id;
    std::string reason_digest;
    std::string evidence_digest;
    std::uint64_t occurred_at{};
    std::uint64_t recorded_at{};
};

struct KeyTransitionRecord final {
    std::string id;
    std::string key_id;
    KeyTransitionKind kind{KeyTransitionKind::activate};
    KeyLifecycleState state_after{KeyLifecycleState::provisioned};
    std::string actor_entity_id;
    std::string reason_digest;
    std::string evidence_digest;
    std::uint64_t sequence{};
    std::uint64_t occurred_at{};
    std::uint64_t recorded_at{};
    std::string previous_transition_digest;
    std::string transition_digest;

    [[nodiscard]] bool operator==(const KeyTransitionRecord&) const = default;
};

struct KeySuccessionDraft final {
    std::string id;
    KeySuccessionKind kind{KeySuccessionKind::rotation};
    std::string predecessor_key_id;
    std::string successor_key_id;
    std::string actor_entity_id;
    std::string reason_digest;
    std::string evidence_digest;
    std::uint64_t occurred_at{};
    std::uint64_t recorded_at{};
};

struct KeySuccessionRecord final {
    std::string id;
    KeySuccessionKind kind{KeySuccessionKind::rotation};
    std::string predecessor_key_id;
    std::string successor_key_id;
    std::string actor_entity_id;
    std::string reason_digest;
    std::string evidence_digest;
    std::uint64_t sequence{};
    std::uint64_t occurred_at{};
    std::uint64_t recorded_at{};
    std::string previous_succession_digest;
    std::string succession_digest;

    [[nodiscard]] bool operator==(const KeySuccessionRecord&) const = default;
};

struct KeyOperationRequest final {
    std::string key_id;
    KeyUsage usage{KeyUsage::sign};
    std::string actor_entity_id;
    std::string operation_context_digest;
    std::uint64_t requested_at{};
};

// This is a deny-by-default evidence preflight. Even a successful candidate
// does not execute cryptography, authenticate an entity, prove provenance or
// authorize an action.
struct KeyOperationDecision final {
    KeyLifecycleState state{KeyLifecycleState::provisioned};
    bool key_found{false};
    bool registry_valid{false};
    bool entity_registry_valid{false};
    bool owner_bound{false};
    bool actor_recorded{false};
    bool actor_role_matched{false};
    bool active{false};
    bool within_cryptoperiod{false};
    bool usage_permitted{false};
    bool provider_registry_valid{false};
    bool provider_integration_candidate{false};
    bool provider_route_bound{false};
    bool custody_policy_bound{false};
    bool operation_context_recorded{false};
    bool execution_candidate{false};
    bool cryptographic_operation_executed{false};
    bool identity_authenticated{false};
    bool provenance_authenticated{false};
    bool action_authorized{false};
    std::string reason;
};

struct KeyCustodyAudit final {
    std::size_t registry_errors{};
    std::size_t owner_mismatches{};
    std::size_t missing_custodians{};
    std::size_t missing_recovery_authorities{};
    std::size_t missing_operators{};
    std::size_t temporal_mismatches{};

    [[nodiscard]] bool clean() const noexcept;
};

enum class KeyCustodyErrorCode : std::uint8_t {
    none,
    invalid_manifest,
    capacity_exceeded,
    duplicate_entry,
    missing_key,
    missing_entity,
    owner_binding_mismatch,
    temporal_conflict,
    transition_conflict,
    provider_not_qualified,
    provider_binding_mismatch,
    custody_policy_mismatch,
    role_mismatch,
    succession_conflict,
};

struct KeyCustodyError final {
    KeyCustodyErrorCode code{KeyCustodyErrorCode::none};
    std::string message;
};

[[nodiscard]] std::string derive_key_custody_registry_id(
    std::string_view entity_namespace_id,
    std::string_view owner_entity_id);
[[nodiscard]] std::string derive_key_handle_id(
    std::string_view owner_entity_id,
    std::string_view provider_id,
    std::string_view algorithm_id,
    CryptoFunction function,
    std::string_view implementation_route,
    std::string_view implementation_digest,
    std::string_view provider_locator_digest,
    std::uint64_t generation);

class KeyCustodyRegistry final {
public:
    KeyCustodyRegistry(std::string registry_id,
                       std::string entity_namespace_id,
                       std::string owner_entity_id,
                       std::size_t key_capacity,
                       std::size_t transition_capacity,
                       std::size_t succession_capacity);

    [[nodiscard]] bool register_key(
        KeyHandleManifest manifest,
        const CryptoProviderRegistry& providers,
        const identity::EntityRegistry& entities,
        KeyCustodyError* error = nullptr);
    [[nodiscard]] bool record_transition(
        KeyTransitionDraft draft,
        const CryptoProviderRegistry& providers,
        const identity::EntityRegistry& entities,
        KeyCustodyError* error = nullptr);
    [[nodiscard]] bool record_succession(
        KeySuccessionDraft draft,
        const identity::EntityRegistry& entities,
        KeyCustodyError* error = nullptr);

    [[nodiscard]] const KeyHandleManifest* key(std::string_view key_id) const;
    [[nodiscard]] const std::vector<KeyTransitionRecord>* transition_history(
        std::string_view key_id) const;
    [[nodiscard]] KeyLifecycleState key_state(std::string_view key_id) const;
    [[nodiscard]] KeyLifecycleState key_state_at(std::string_view key_id,
                                                 std::uint64_t at) const;
    [[nodiscard]] const KeySuccessionRecord* succession_from(
        std::string_view predecessor_key_id) const;
    [[nodiscard]] const KeySuccessionRecord* succession_to(
        std::string_view successor_key_id) const;
    [[nodiscard]] KeyOperationDecision preflight(
        const KeyOperationRequest& request,
        const CryptoProviderRegistry& providers,
        const identity::EntityRegistry& entities) const;
    [[nodiscard]] KeyCustodyAudit audit_entities(
        const identity::EntityRegistry& entities) const;
    [[nodiscard]] bool verify() const;

    [[nodiscard]] const std::string& registry_id() const noexcept;
    [[nodiscard]] const std::string& entity_namespace_id() const noexcept;
    [[nodiscard]] const std::string& owner_entity_id() const noexcept;
    [[nodiscard]] std::size_t key_capacity() const noexcept;
    [[nodiscard]] std::size_t transition_capacity() const noexcept;
    [[nodiscard]] std::size_t succession_capacity() const noexcept;
    [[nodiscard]] std::size_t transition_count() const noexcept;
    [[nodiscard]] const std::map<std::string,
                                 KeyHandleManifest,
                                 std::less<>>&
    keys() const noexcept;
    [[nodiscard]] const std::map<std::string,
                                 std::vector<KeyTransitionRecord>,
                                 std::less<>>&
    transitions() const noexcept;
    [[nodiscard]] const std::vector<KeySuccessionRecord>& successions() const noexcept;

private:
    friend class KeyCustodyStore;

    [[nodiscard]] bool rebuild_indexes();

    std::string registry_id_;
    std::string entity_namespace_id_;
    std::string owner_entity_id_;
    std::size_t key_capacity_{};
    std::size_t transition_capacity_{};
    std::size_t succession_capacity_{};
    std::size_t transition_count_{};
    std::map<std::string, KeyHandleManifest, std::less<>> keys_;
    std::map<std::string,
             std::vector<KeyTransitionRecord>,
             std::less<>> transitions_;
    std::vector<KeySuccessionRecord> successions_;
    std::map<std::string, std::string, std::less<>> transition_index_;
    std::map<std::string, std::size_t, std::less<>> succession_id_index_;
    std::map<std::string, std::size_t, std::less<>> predecessor_index_;
    std::map<std::string, std::size_t, std::less<>> successor_index_;
};

} // namespace genesis::security
