#include "genesis/identity/entity_persistence.hpp"

#include "genesis/common/immutable_snapshot.hpp"
#include "genesis/runtime/runtime.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace genesis::identity {
namespace {

constexpr std::string_view kMagic = "GENESIS-ENTITY-REGISTRY";
constexpr std::string_view kFileSuffix = "entities";
constexpr std::uint64_t kSchemaVersion = 1U;
constexpr std::uint64_t kMaximumItems = 1'000'000U;
constexpr std::uint64_t kMaximumFieldBytes = 16U * 1024U * 1024U;
constexpr std::size_t kChecksumBytes = 64U;

void set_error(EntityStoreError* error, EntityStoreErrorCode code, std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->message = std::move(message);
    }
}

bool bounded_text(std::string_view value, std::size_t maximum) {
    return !value.empty() && value.size() <= maximum
           && std::all_of(value.begin(), value.end(), [](unsigned char character) {
                  return character >= 0x20U && character != 0x7fU;
              });
}

void map_file_error(const storage::ImmutableFileError& source,
                    bool writing,
                    EntityStoreError* target) {
    switch (source.code) {
    case storage::ImmutableFileErrorCode::none:
        set_error(target, EntityStoreErrorCode::none, {});
        return;
    case storage::ImmutableFileErrorCode::invalid_identifier:
        set_error(target, EntityStoreErrorCode::invalid_identifier, source.message);
        return;
    case storage::ImmutableFileErrorCode::not_found:
        set_error(target, EntityStoreErrorCode::not_found, source.message);
        return;
    case storage::ImmutableFileErrorCode::conflicting_version:
        set_error(target, EntityStoreErrorCode::conflicting_version, source.message);
        return;
    case storage::ImmutableFileErrorCode::record_too_large:
        set_error(target,
                  writing ? EntityStoreErrorCode::invalid_registry
                          : EntityStoreErrorCode::corrupt_record,
                  source.message);
        return;
    case storage::ImmutableFileErrorCode::unsafe_file_type:
        set_error(target,
                  writing ? EntityStoreErrorCode::io_error
                          : EntityStoreErrorCode::corrupt_record,
                  source.message);
        return;
    case storage::ImmutableFileErrorCode::io_error:
        set_error(target, EntityStoreErrorCode::io_error, source.message);
        return;
    }
    set_error(target, EntityStoreErrorCode::io_error, "unknown immutable-file error");
}

void append_u64(std::string& output, std::uint64_t value) {
    for (unsigned shift = 0U; shift < 64U; shift += 8U) {
        output.push_back(static_cast<char>((value >> shift) & 0xffU));
    }
}

std::uint64_t read_u64(std::string_view bytes, std::size_t& offset) {
    if (offset > bytes.size() || bytes.size() - offset < 8U) {
        throw std::runtime_error("truncated entity registry integer");
    }
    std::uint64_t value = 0U;
    for (unsigned shift = 0U; shift < 64U; shift += 8U) {
        value |= static_cast<std::uint64_t>(
                     static_cast<unsigned char>(bytes[offset++]))
                 << shift;
    }
    return value;
}

void append_string(std::string& output, std::string_view value) {
    append_u64(output, static_cast<std::uint64_t>(value.size()));
    output.append(value);
}

std::string read_string(std::string_view bytes, std::size_t& offset) {
    const auto length = read_u64(bytes, offset);
    const auto remaining = static_cast<std::uint64_t>(bytes.size() - offset);
    if (length > kMaximumFieldBytes || length > remaining
        || length > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("invalid entity registry field length");
    }
    const auto size = static_cast<std::size_t>(length);
    std::string value(bytes.substr(offset, size));
    offset += size;
    return value;
}

void append_entity(std::string& output, const EntityAddress& entity) {
    append_string(output, entity.entity_id);
    append_u64(output, static_cast<std::uint64_t>(entity.kind));
    append_string(output, entity.namespace_id);
    append_string(output, entity.local_key);
    append_string(output, entity.provenance_digest);
    append_u64(output, entity.registered_at);
}

EntityAddress read_entity(std::string_view bytes, std::size_t& offset) {
    EntityAddress entity;
    entity.entity_id = read_string(bytes, offset);
    const auto kind = read_u64(bytes, offset);
    if (kind > static_cast<std::uint64_t>(EntityKind::custom)) {
        throw std::runtime_error("invalid entity kind");
    }
    entity.kind = static_cast<EntityKind>(kind);
    entity.namespace_id = read_string(bytes, offset);
    entity.local_key = read_string(bytes, offset);
    entity.provenance_digest = read_string(bytes, offset);
    entity.registered_at = read_u64(bytes, offset);
    return entity;
}

void append_relation(std::string& output, const EntityRelation& relation) {
    append_string(output, relation.relation_id);
    append_u64(output, static_cast<std::uint64_t>(relation.kind));
    append_string(output, relation.subject_id);
    append_string(output, relation.object_id);
    append_u64(output, relation.version);
    append_u64(output, relation.effective_from);
    append_u64(output, relation.effective_until.has_value() ? 1U : 0U);
    append_u64(output, relation.effective_until.value_or(0U));
    append_u64(output, relation.recorded_at);
    append_string(output, relation.evidence_digest);
    append_u64(output, static_cast<std::uint64_t>(relation.state));
}

EntityRelation read_relation(std::string_view bytes, std::size_t& offset) {
    EntityRelation relation;
    relation.relation_id = read_string(bytes, offset);
    const auto kind = read_u64(bytes, offset);
    if (kind > static_cast<std::uint64_t>(RelationKind::custom)) {
        throw std::runtime_error("invalid relation kind");
    }
    relation.kind = static_cast<RelationKind>(kind);
    relation.subject_id = read_string(bytes, offset);
    relation.object_id = read_string(bytes, offset);
    relation.version = read_u64(bytes, offset);
    relation.effective_from = read_u64(bytes, offset);
    const auto has_until = read_u64(bytes, offset);
    const auto until = read_u64(bytes, offset);
    if (has_until > 1U || (has_until == 0U && until != 0U)) {
        throw std::runtime_error("invalid optional relation end time");
    }
    if (has_until == 1U) {
        relation.effective_until = until;
    }
    relation.recorded_at = read_u64(bytes, offset);
    relation.evidence_digest = read_string(bytes, offset);
    const auto state = read_u64(bytes, offset);
    if (state > static_cast<std::uint64_t>(RelationState::superseded)) {
        throw std::runtime_error("invalid relation state");
    }
    relation.state = static_cast<RelationState>(state);
    return relation;
}

} // namespace

EntityRegistryStore::EntityRegistryStore(std::filesystem::path root,
                                         std::size_t maximum_record_bytes)
    : root_(std::move(root)), maximum_record_bytes_(maximum_record_bytes) {
    static_cast<void>(storage::ImmutableSnapshotFiles(root_, maximum_record_bytes_));
}

std::string EntityRegistryStore::serialize(const EntityRegistry& registry) {
    if (!registry.verify()) {
        throw std::invalid_argument("cannot serialize an invalid entity registry");
    }

    std::string output{kMagic};
    append_u64(output, kSchemaVersion);
    append_string(output, registry.namespace_id_);
    append_string(output, registry.registrar_organism_id_);
    append_u64(output, static_cast<std::uint64_t>(registry.entity_capacity_));
    append_u64(output, static_cast<std::uint64_t>(registry.relation_version_capacity_));
    append_u64(output, static_cast<std::uint64_t>(registry.entities_.size()));
    for (const auto& [unused_id, entity] : registry.entities_) {
        static_cast<void>(unused_id);
        append_entity(output, entity);
    }
    append_u64(output, static_cast<std::uint64_t>(registry.relation_version_count_));
    for (const auto& [unused_id, history] : registry.relations_) {
        static_cast<void>(unused_id);
        for (const auto& relation : history) {
            append_relation(output, relation);
        }
    }
    output += runtime::sha256(output);
    return output;
}

std::optional<EntityRegistry> EntityRegistryStore::deserialize(std::string_view bytes,
                                                               EntityStoreError* error) {
    try {
        if (bytes.size() < kMagic.size() + 8U + kChecksumBytes
            || bytes.substr(0U, kMagic.size()) != kMagic) {
            throw std::runtime_error("invalid entity registry magic");
        }
        const auto payload = bytes.substr(0U, bytes.size() - kChecksumBytes);
        const auto checksum = bytes.substr(bytes.size() - kChecksumBytes);
        if (runtime::sha256(payload) != checksum) {
            throw std::runtime_error("entity registry checksum mismatch");
        }

        std::size_t offset = kMagic.size();
        if (read_u64(payload, offset) != kSchemaVersion) {
            set_error(error,
                      EntityStoreErrorCode::unsupported_schema,
                      "unsupported entity registry schema");
            return std::nullopt;
        }
        auto namespace_id = read_string(payload, offset);
        auto registrar_id = read_string(payload, offset);
        const auto entity_capacity = read_u64(payload, offset);
        const auto relation_capacity = read_u64(payload, offset);
        if (entity_capacity == 0U || relation_capacity == 0U
            || entity_capacity > kMaximumItems || relation_capacity > kMaximumItems
            || entity_capacity
                   > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())
            || relation_capacity
                   > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            throw std::runtime_error("invalid entity registry capacities");
        }
        EntityRegistry registry(std::move(namespace_id),
                                std::move(registrar_id),
                                static_cast<std::size_t>(entity_capacity),
                                static_cast<std::size_t>(relation_capacity));

        const auto entity_count = read_u64(payload, offset);
        if (entity_count > entity_capacity) {
            throw std::runtime_error("entity count exceeds registry capacity");
        }
        for (std::uint64_t index = 0U; index < entity_count; ++index) {
            auto entity = read_entity(payload, offset);
            EntityRegistryError registry_error;
            if (!registry.register_entity(std::move(entity), &registry_error)) {
                throw std::runtime_error("invalid stored entity: " + registry_error.message);
            }
        }

        const auto relation_count = read_u64(payload, offset);
        if (relation_count > relation_capacity) {
            throw std::runtime_error("relation-version count exceeds registry capacity");
        }
        for (std::uint64_t index = 0U; index < relation_count; ++index) {
            auto relation = read_relation(payload, offset);
            EntityRegistryError registry_error;
            if (!registry.record_relation(std::move(relation), &registry_error)) {
                throw std::runtime_error("invalid stored relation: " + registry_error.message);
            }
        }
        if (offset != payload.size() || !registry.verify()) {
            throw std::runtime_error("trailing or internally inconsistent entity registry data");
        }
        set_error(error, EntityStoreErrorCode::none, {});
        return registry;
    } catch (const std::exception& exception) {
        set_error(error, EntityStoreErrorCode::corrupt_record, exception.what());
        return std::nullopt;
    }
}

bool EntityRegistryStore::write(const EntityRegistry& registry,
                                std::string_view version,
                                EntityStoreError* error) const {
    try {
        const auto bytes = serialize(registry);
        storage::ImmutableSnapshotFiles files(root_, maximum_record_bytes_);
        storage::ImmutableFileError file_error;
        if (!files.write(registry.namespace_id(), version, kFileSuffix, bytes, &file_error)) {
            map_file_error(file_error, true, error);
            return false;
        }
        set_error(error, EntityStoreErrorCode::none, {});
        return true;
    } catch (const std::invalid_argument& exception) {
        set_error(error, EntityStoreErrorCode::invalid_registry, exception.what());
        return false;
    } catch (const std::exception& exception) {
        set_error(error, EntityStoreErrorCode::io_error, exception.what());
        return false;
    }
}

std::optional<EntityRegistry> EntityRegistryStore::read(
    std::string_view namespace_id,
    std::string_view registrar_organism_id,
    std::string_view version,
    EntityStoreError* error) const {
    try {
        if (!bounded_text(registrar_organism_id, 256U)) {
            throw std::invalid_argument("invalid registrar identity");
        }
        storage::ImmutableSnapshotFiles files(root_, maximum_record_bytes_);
        storage::ImmutableFileError file_error;
        const auto bytes = files.read(namespace_id, version, kFileSuffix, &file_error);
        if (!bytes.has_value()) {
            map_file_error(file_error, false, error);
            return std::nullopt;
        }
        auto registry = deserialize(*bytes, error);
        if (registry.has_value()
            && (registry->namespace_id() != namespace_id
                || registry->registrar_organism_id() != registrar_organism_id)) {
            set_error(error,
                      EntityStoreErrorCode::corrupt_record,
                      "entity registry namespace or registrar binding mismatch");
            return std::nullopt;
        }
        return registry;
    } catch (const std::invalid_argument& exception) {
        set_error(error, EntityStoreErrorCode::invalid_identifier, exception.what());
        return std::nullopt;
    } catch (const std::exception& exception) {
        set_error(error, EntityStoreErrorCode::io_error, exception.what());
        return std::nullopt;
    }
}

} // namespace genesis::identity
