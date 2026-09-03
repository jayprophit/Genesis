#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::identity {

class EntityRegistryStore;

// Values are serialized by their numeric value. Append new values; do not
// reorder or repurpose existing values without a persistence migration.
enum class EntityKind : std::uint8_t {
    organism,
    shell,
    component,
    record,
    place,
    credential,
    experiment,
    account,
    person,
    organization,
    dataset,
    model,
    service,
    custom,
};

enum class RelationKind : std::uint8_t {
    owns,
    custodies,
    operates,
    embodies,
    contains,
    located_at,
    issued_by,
    subject_of,
    derived_from,
    supersedes,
    associated_with,
    custom,
};

enum class RelationState : std::uint8_t {
    active,
    ended,
    revoked,
    superseded,
};

[[nodiscard]] std::string_view to_string(EntityKind kind) noexcept;
[[nodiscard]] std::string_view to_string(RelationKind kind) noexcept;
[[nodiscard]] std::string_view to_string(RelationState state) noexcept;
[[nodiscard]] bool entity_kind_from_string(std::string_view text,
                                           EntityKind& kind) noexcept;
[[nodiscard]] bool relation_kind_from_string(std::string_view text,
                                             RelationKind& kind) noexcept;
[[nodiscard]] bool relation_state_from_string(std::string_view text,
                                              RelationState& state) noexcept;

// An address is an immutable, typed registry fact. provenance_digest is an
// integrity/evidence reference; it is not a signature or authentication proof.
struct EntityAddress final {
    std::string entity_id;
    EntityKind kind{EntityKind::custom};
    std::string namespace_id;
    std::string local_key;
    std::string provenance_digest;
    std::uint64_t registered_at{};

    [[nodiscard]] bool operator==(const EntityAddress&) const = default;
};

// A relation ID identifies one subject/kind/object/effective-from assertion.
// Amendments append monotonically increasing versions; earlier versions remain
// available. Effective intervals use [effective_from, effective_until).
struct EntityRelation final {
    std::string relation_id;
    RelationKind kind{RelationKind::custom};
    std::string subject_id;
    std::string object_id;
    std::uint64_t version{1};
    std::uint64_t effective_from{};
    std::optional<std::uint64_t> effective_until;
    std::uint64_t recorded_at{};
    std::string evidence_digest;
    RelationState state{RelationState::active};

    [[nodiscard]] bool operator==(const EntityRelation&) const = default;
};

// Registry queries return evidence-bearing facts only. Authorization must be
// decided by a separate deny-by-default policy boundary.
struct RelationQuery final {
    std::vector<EntityRelation> facts;
    bool action_authorized{false};
    bool organism_identity_reassigned{false};
};

enum class EntityRegistryErrorCode : std::uint8_t {
    none,
    invalid_entity,
    duplicate_entity,
    identifier_collision,
    entity_capacity_exceeded,
    invalid_relation,
    missing_subject,
    missing_object,
    self_relation,
    endpoint_kind_mismatch,
    temporal_conflict,
    relation_version_conflict,
    terminal_relation,
    relation_capacity_exceeded,
};

struct EntityRegistryError final {
    EntityRegistryErrorCode code{EntityRegistryErrorCode::none};
    std::string message;
};

[[nodiscard]] std::string derive_entity_id(std::string_view namespace_id,
                                           EntityKind kind,
                                           std::string_view local_key);
[[nodiscard]] EntityAddress make_entity_address(std::string namespace_id,
                                                EntityKind kind,
                                                std::string local_key,
                                                std::string provenance_digest,
                                                std::uint64_t registered_at);
[[nodiscard]] std::string derive_relation_id(std::string_view namespace_id,
                                             RelationKind kind,
                                             std::string_view subject_id,
                                             std::string_view object_id,
                                             std::uint64_t effective_from);
[[nodiscard]] EntityRelation make_entity_relation(std::string_view namespace_id,
                                                  RelationKind kind,
                                                  std::string subject_id,
                                                  std::string object_id,
                                                  std::uint64_t effective_from,
                                                  std::uint64_t recorded_at,
                                                  std::string evidence_digest);

class EntityRegistry final {
public:
    EntityRegistry(std::string namespace_id,
                   std::string registrar_organism_id,
                   std::size_t entity_capacity,
                   std::size_t relation_version_capacity);

    [[nodiscard]] bool register_entity(EntityAddress address,
                                       EntityRegistryError* error = nullptr);
    [[nodiscard]] bool record_relation(EntityRelation relation,
                                       EntityRegistryError* error = nullptr);

    [[nodiscard]] const EntityAddress* find_entity(std::string_view entity_id) const;
    [[nodiscard]] const EntityRelation* latest_relation(
        std::string_view relation_id) const;
    [[nodiscard]] std::vector<EntityRelation> relation_history(
        std::string_view relation_id) const;
    [[nodiscard]] RelationQuery relations_for(std::string_view entity_id) const;

    // Only an entity registered with kind=organism can resolve to an organism
    // identity. Accounts, shells, credentials, people and relations cannot.
    [[nodiscard]] std::optional<std::string> organism_identity(
        std::string_view entity_id) const;

    [[nodiscard]] bool verify() const;
    [[nodiscard]] const std::string& namespace_id() const noexcept;
    [[nodiscard]] const std::string& registrar_organism_id() const noexcept;
    [[nodiscard]] std::size_t entity_capacity() const noexcept;
    [[nodiscard]] std::size_t relation_version_capacity() const noexcept;
    [[nodiscard]] std::size_t entity_count() const noexcept;
    [[nodiscard]] std::size_t relation_count() const noexcept;
    [[nodiscard]] std::size_t relation_version_count() const noexcept;

private:
    friend class EntityRegistryStore;

    std::string namespace_id_;
    std::string registrar_organism_id_;
    std::size_t entity_capacity_{};
    std::size_t relation_version_capacity_{};
    std::size_t relation_version_count_{};
    std::map<std::string, EntityAddress, std::less<>> entities_;
    std::map<std::string, std::vector<EntityRelation>, std::less<>> relations_;
};

} // namespace genesis::identity
