#include "genesis/identity/entity_registry.hpp"

#include "genesis/runtime/runtime.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace genesis::identity {
namespace {

constexpr std::size_t kMaximumNamespaceLength = 128;
constexpr std::size_t kMaximumRegistrarLength = 256;
constexpr std::size_t kMaximumLocalKeyLength = 1024;
constexpr std::size_t kMaximumEntityIdLength = 160;
constexpr std::size_t kMaximumRegistryItems = 1'000'000;

bool bounded_text(std::string_view value, std::size_t maximum) {
    if (value.empty() || value.size() > maximum) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return character >= 0x20U && character != 0x7fU;
    });
}

bool safe_namespace(std::string_view value) {
    if (!bounded_text(value, kMaximumNamespaceLength) || value == "." || value == "..") {
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

template <typename Enum>
bool enum_in_range(Enum value, Enum maximum) {
    return static_cast<std::uint64_t>(value) <= static_cast<std::uint64_t>(maximum);
}

void append_material(std::string& output, std::string_view value) {
    output.append(std::to_string(value.size()));
    output.push_back(':');
    output.append(value);
}

void set_error(EntityRegistryError* error,
               EntityRegistryErrorCode code,
               std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->message = std::move(message);
    }
}

bool valid_entity(const EntityAddress& address, std::string_view expected_namespace) {
    if (!enum_in_range(address.kind, EntityKind::custom)
        || !safe_namespace(address.namespace_id)
        || address.namespace_id != expected_namespace
        || !bounded_text(address.local_key, kMaximumLocalKeyLength)
        || !bounded_text(address.entity_id, kMaximumEntityIdLength)
        || !digest(address.provenance_digest)) {
        return false;
    }
    return address.entity_id
           == derive_entity_id(address.namespace_id, address.kind, address.local_key);
}

bool valid_relation_shape(const EntityRelation& relation, std::string_view namespace_id) {
    if (!enum_in_range(relation.kind, RelationKind::custom)
        || !enum_in_range(relation.state, RelationState::superseded)
        || !bounded_text(relation.subject_id, kMaximumEntityIdLength)
        || !bounded_text(relation.object_id, kMaximumEntityIdLength)
        || relation.subject_id == relation.object_id || relation.version == 0U
        || !digest(relation.evidence_digest)) {
        return false;
    }
    if (relation.state == RelationState::active && relation.effective_until.has_value()) {
        return false;
    }
    if (relation.state != RelationState::active
        && (!relation.effective_until.has_value()
            || *relation.effective_until <= relation.effective_from
            || relation.recorded_at < *relation.effective_until)) {
        return false;
    }
    return relation.relation_id
           == derive_relation_id(namespace_id,
                                 relation.kind,
                                 relation.subject_id,
                                 relation.object_id,
                                 relation.effective_from);
}

bool endpoint_kinds_match(RelationKind relation,
                          EntityKind subject,
                          EntityKind object) noexcept {
    switch (relation) {
    case RelationKind::embodies:
        return subject == EntityKind::organism && object == EntityKind::shell;
    case RelationKind::located_at:
        return object == EntityKind::place;
    case RelationKind::issued_by:
    case RelationKind::subject_of:
        return subject == EntityKind::credential;
    case RelationKind::supersedes:
        return subject == object;
    default:
        return true;
    }
}

template <typename Enum, std::size_t Size>
bool parse_enum(std::string_view text,
                const std::array<std::string_view, Size>& names,
                Enum& result) noexcept {
    const auto found = std::find(names.begin(), names.end(), text);
    if (found == names.end()) {
        return false;
    }
    result = static_cast<Enum>(std::distance(names.begin(), found));
    return true;
}

constexpr std::array<std::string_view, 14> kEntityKindNames{
    "organism", "shell",       "component", "record", "place",
    "credential", "experiment", "account",   "person", "organization",
    "dataset",    "model",       "service",   "custom",
};

constexpr std::array<std::string_view, 12> kRelationKindNames{
    "owns",       "custodies",  "operates",     "embodies",
    "contains",   "located_at", "issued_by",    "subject_of",
    "derived_from", "supersedes", "associated_with", "custom",
};

constexpr std::array<std::string_view, 4> kRelationStateNames{
    "active", "ended", "revoked", "superseded",
};

} // namespace

std::string_view to_string(EntityKind kind) noexcept {
    const auto index = static_cast<std::size_t>(kind);
    return index < kEntityKindNames.size() ? kEntityKindNames[index] : "unknown";
}

std::string_view to_string(RelationKind kind) noexcept {
    const auto index = static_cast<std::size_t>(kind);
    return index < kRelationKindNames.size() ? kRelationKindNames[index] : "unknown";
}

std::string_view to_string(RelationState state) noexcept {
    const auto index = static_cast<std::size_t>(state);
    return index < kRelationStateNames.size() ? kRelationStateNames[index] : "unknown";
}

bool entity_kind_from_string(std::string_view text, EntityKind& kind) noexcept {
    return parse_enum(text, kEntityKindNames, kind);
}

bool relation_kind_from_string(std::string_view text, RelationKind& kind) noexcept {
    return parse_enum(text, kRelationKindNames, kind);
}

bool relation_state_from_string(std::string_view text, RelationState& state) noexcept {
    return parse_enum(text, kRelationStateNames, state);
}

std::string derive_entity_id(std::string_view namespace_id,
                             EntityKind kind,
                             std::string_view local_key) {
    if (!safe_namespace(namespace_id) || !enum_in_range(kind, EntityKind::custom)
        || !bounded_text(local_key, kMaximumLocalKeyLength)) {
        throw std::invalid_argument("invalid entity address material");
    }
    std::string material{"genesis.entity-address.v1"};
    append_material(material, namespace_id);
    append_material(material, to_string(kind));
    append_material(material, local_key);
    return "genesis.entity.v1." + std::string(to_string(kind)) + "."
           + runtime::sha256(material);
}

EntityAddress make_entity_address(std::string namespace_id,
                                  EntityKind kind,
                                  std::string local_key,
                                  std::string provenance_digest,
                                  std::uint64_t registered_at) {
    EntityAddress address;
    address.entity_id = derive_entity_id(namespace_id, kind, local_key);
    address.kind = kind;
    address.namespace_id = std::move(namespace_id);
    address.local_key = std::move(local_key);
    address.provenance_digest = std::move(provenance_digest);
    address.registered_at = registered_at;
    return address;
}

std::string derive_relation_id(std::string_view namespace_id,
                               RelationKind kind,
                               std::string_view subject_id,
                               std::string_view object_id,
                               std::uint64_t effective_from) {
    if (!safe_namespace(namespace_id) || !enum_in_range(kind, RelationKind::custom)
        || !bounded_text(subject_id, kMaximumEntityIdLength)
        || !bounded_text(object_id, kMaximumEntityIdLength) || subject_id == object_id) {
        throw std::invalid_argument("invalid entity relation material");
    }
    std::string material{"genesis.entity-relation.v1"};
    append_material(material, namespace_id);
    append_material(material, to_string(kind));
    append_material(material, subject_id);
    append_material(material, object_id);
    append_material(material, std::to_string(effective_from));
    return "genesis.relation.v1." + std::string(to_string(kind)) + "."
           + runtime::sha256(material);
}

EntityRelation make_entity_relation(std::string_view namespace_id,
                                    RelationKind kind,
                                    std::string subject_id,
                                    std::string object_id,
                                    std::uint64_t effective_from,
                                    std::uint64_t recorded_at,
                                    std::string evidence_digest) {
    EntityRelation relation;
    relation.relation_id = derive_relation_id(
        namespace_id, kind, subject_id, object_id, effective_from);
    relation.kind = kind;
    relation.subject_id = std::move(subject_id);
    relation.object_id = std::move(object_id);
    relation.effective_from = effective_from;
    relation.recorded_at = recorded_at;
    relation.evidence_digest = std::move(evidence_digest);
    return relation;
}

EntityRegistry::EntityRegistry(std::string namespace_id,
                               std::string registrar_organism_id,
                               std::size_t entity_capacity,
                               std::size_t relation_version_capacity)
    : namespace_id_(std::move(namespace_id)),
      registrar_organism_id_(std::move(registrar_organism_id)),
      entity_capacity_(entity_capacity),
      relation_version_capacity_(relation_version_capacity) {
    if (!safe_namespace(namespace_id_)
        || !bounded_text(registrar_organism_id_, kMaximumRegistrarLength)
        || entity_capacity_ == 0U || relation_version_capacity_ == 0U
        || entity_capacity_ > kMaximumRegistryItems
        || relation_version_capacity_ > kMaximumRegistryItems) {
        throw std::invalid_argument("invalid entity registry configuration");
    }
}

bool EntityRegistry::register_entity(EntityAddress address, EntityRegistryError* error) {
    set_error(error, EntityRegistryErrorCode::none, {});
    if (!valid_entity(address, namespace_id_)) {
        set_error(error,
                  EntityRegistryErrorCode::invalid_entity,
                  "entity fields, type, evidence or derived identifier are invalid");
        return false;
    }
    const auto existing = entities_.find(address.entity_id);
    if (existing != entities_.end()) {
        const auto code = existing->second == address
                              ? EntityRegistryErrorCode::duplicate_entity
                              : EntityRegistryErrorCode::identifier_collision;
        set_error(error,
                  code,
                  code == EntityRegistryErrorCode::duplicate_entity
                      ? "entity address is already registered"
                      : "entity identifier is already bound to different immutable fields");
        return false;
    }
    if (entities_.size() >= entity_capacity_) {
        set_error(error,
                  EntityRegistryErrorCode::entity_capacity_exceeded,
                  "entity capacity exceeded");
        return false;
    }
    auto entity_id = address.entity_id;
    entities_.emplace(std::move(entity_id), std::move(address));
    return true;
}

bool EntityRegistry::record_relation(EntityRelation relation, EntityRegistryError* error) {
    set_error(error, EntityRegistryErrorCode::none, {});
    if (relation.subject_id == relation.object_id) {
        set_error(error,
                  EntityRegistryErrorCode::self_relation,
                  "self-relations are not meaningful registry facts");
        return false;
    }
    if (!valid_relation_shape(relation, namespace_id_)) {
        set_error(error,
                  EntityRegistryErrorCode::invalid_relation,
                  "relation fields, interval, evidence or derived identifier are invalid");
        return false;
    }
    const auto subject = entities_.find(relation.subject_id);
    if (subject == entities_.end()) {
        set_error(error,
                  EntityRegistryErrorCode::missing_subject,
                  "relation subject is not registered");
        return false;
    }
    const auto object = entities_.find(relation.object_id);
    if (object == entities_.end()) {
        set_error(error,
                  EntityRegistryErrorCode::missing_object,
                  "relation object is not registered");
        return false;
    }
    if (!endpoint_kinds_match(relation.kind, subject->second.kind, object->second.kind)) {
        set_error(error,
                  EntityRegistryErrorCode::endpoint_kind_mismatch,
                  "relation endpoint kinds violate the typed relation contract");
        return false;
    }
    if (relation.recorded_at < subject->second.registered_at
        || relation.recorded_at < object->second.registered_at) {
        set_error(error,
                  EntityRegistryErrorCode::temporal_conflict,
                  "a relation cannot be recorded before either endpoint is registered");
        return false;
    }
    if (relation_version_count_ >= relation_version_capacity_) {
        set_error(error,
                  EntityRegistryErrorCode::relation_capacity_exceeded,
                  "relation-version capacity exceeded");
        return false;
    }

    auto found = relations_.find(relation.relation_id);
    if (found == relations_.end()) {
        if (relation.version != 1U) {
            set_error(error,
                      EntityRegistryErrorCode::relation_version_conflict,
                      "a relation history must begin at version one");
            return false;
        }
        auto relation_id = relation.relation_id;
        relations_.emplace(std::move(relation_id),
                           std::vector<EntityRelation>{std::move(relation)});
        ++relation_version_count_;
        return true;
    }

    const auto& previous = found->second.back();
    if (previous.version == std::numeric_limits<std::uint64_t>::max()
        || relation.version != previous.version + 1U
        || relation.kind != previous.kind || relation.subject_id != previous.subject_id
        || relation.object_id != previous.object_id
        || relation.effective_from != previous.effective_from
        || relation.recorded_at < previous.recorded_at) {
        set_error(error,
                  EntityRegistryErrorCode::relation_version_conflict,
                  "relation versions must be contiguous and preserve immutable identity fields");
        return false;
    }
    if (previous.state != RelationState::active) {
        set_error(error,
                  EntityRegistryErrorCode::terminal_relation,
                  "a terminal relation history cannot be rewritten");
        return false;
    }
    found->second.push_back(std::move(relation));
    ++relation_version_count_;
    return true;
}

const EntityAddress* EntityRegistry::find_entity(std::string_view entity_id) const {
    const auto found = entities_.find(entity_id);
    return found == entities_.end() ? nullptr : &found->second;
}

const EntityRelation* EntityRegistry::latest_relation(std::string_view relation_id) const {
    const auto found = relations_.find(relation_id);
    return found == relations_.end() || found->second.empty() ? nullptr
                                                              : &found->second.back();
}

std::vector<EntityRelation> EntityRegistry::relation_history(
    std::string_view relation_id) const {
    const auto found = relations_.find(relation_id);
    return found == relations_.end() ? std::vector<EntityRelation>{} : found->second;
}

RelationQuery EntityRegistry::relations_for(std::string_view entity_id) const {
    RelationQuery query;
    for (const auto& [unused_id, history] : relations_) {
        static_cast<void>(unused_id);
        if (!history.empty()) {
            const auto& latest = history.back();
            if (latest.subject_id == entity_id || latest.object_id == entity_id) {
                query.facts.push_back(latest);
            }
        }
    }
    return query;
}

std::optional<std::string> EntityRegistry::organism_identity(
    std::string_view entity_id) const {
    const auto* entity = find_entity(entity_id);
    if (entity == nullptr || entity->kind != EntityKind::organism) {
        return std::nullopt;
    }
    return entity->local_key;
}

bool EntityRegistry::verify() const {
    if (!safe_namespace(namespace_id_)
        || !bounded_text(registrar_organism_id_, kMaximumRegistrarLength)
        || entity_capacity_ == 0U || relation_version_capacity_ == 0U
        || entity_capacity_ > kMaximumRegistryItems
        || relation_version_capacity_ > kMaximumRegistryItems
        || entities_.size() > entity_capacity_
        || relation_version_count_ > relation_version_capacity_) {
        return false;
    }
    const auto registrar_address = derive_entity_id(
        namespace_id_, EntityKind::organism, registrar_organism_id_);
    const auto registrar = entities_.find(registrar_address);
    if (registrar == entities_.end() || registrar->second.kind != EntityKind::organism
        || registrar->second.local_key != registrar_organism_id_) {
        return false;
    }
    for (const auto& [id, entity] : entities_) {
        if (id != entity.entity_id || !valid_entity(entity, namespace_id_)) {
            return false;
        }
    }

    std::size_t counted_versions = 0U;
    for (const auto& [id, history] : relations_) {
        if (history.empty()) {
            return false;
        }
        const EntityRelation* previous = nullptr;
        for (const auto& relation : history) {
            if (id != relation.relation_id || !valid_relation_shape(relation, namespace_id_)) {
                return false;
            }
            const auto* subject = find_entity(relation.subject_id);
            const auto* object = find_entity(relation.object_id);
            if (subject == nullptr || object == nullptr
                || !endpoint_kinds_match(relation.kind, subject->kind, object->kind)
                || relation.recorded_at < subject->registered_at
                || relation.recorded_at < object->registered_at) {
                return false;
            }
            if (previous == nullptr) {
                if (relation.version != 1U) {
                    return false;
                }
            } else if (previous->state != RelationState::active
                       || previous->version == std::numeric_limits<std::uint64_t>::max()
                       || relation.version != previous->version + 1U
                       || relation.kind != previous->kind
                       || relation.subject_id != previous->subject_id
                       || relation.object_id != previous->object_id
                       || relation.effective_from != previous->effective_from
                       || relation.recorded_at < previous->recorded_at) {
                return false;
            }
            previous = &relation;
            ++counted_versions;
        }
    }
    return counted_versions == relation_version_count_;
}

const std::string& EntityRegistry::namespace_id() const noexcept {
    return namespace_id_;
}

const std::string& EntityRegistry::registrar_organism_id() const noexcept {
    return registrar_organism_id_;
}

std::size_t EntityRegistry::entity_capacity() const noexcept {
    return entity_capacity_;
}

std::size_t EntityRegistry::relation_version_capacity() const noexcept {
    return relation_version_capacity_;
}

std::size_t EntityRegistry::entity_count() const noexcept {
    return entities_.size();
}

std::size_t EntityRegistry::relation_count() const noexcept {
    return relations_.size();
}

std::size_t EntityRegistry::relation_version_count() const noexcept {
    return relation_version_count_;
}

} // namespace genesis::identity
