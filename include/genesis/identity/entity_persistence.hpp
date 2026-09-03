#pragma once

#include "genesis/identity/entity_registry.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace genesis::identity {

enum class EntityStoreErrorCode : std::uint8_t {
    none,
    invalid_registry,
    invalid_identifier,
    not_found,
    io_error,
    conflicting_version,
    corrupt_record,
    unsupported_schema,
};

struct EntityStoreError final {
    EntityStoreErrorCode code{EntityStoreErrorCode::none};
    std::string message;
};

class EntityRegistryStore final {
public:
    explicit EntityRegistryStore(
        std::filesystem::path root,
        std::size_t maximum_record_bytes = 256U * 1024U * 1024U);

    [[nodiscard]] bool write(const EntityRegistry& registry,
                             std::string_view version,
                             EntityStoreError* error = nullptr) const;
    [[nodiscard]] std::optional<EntityRegistry> read(
        std::string_view namespace_id,
        std::string_view registrar_organism_id,
        std::string_view version,
        EntityStoreError* error = nullptr) const;

    [[nodiscard]] static std::string serialize(const EntityRegistry& registry);
    [[nodiscard]] static std::optional<EntityRegistry> deserialize(
        std::string_view bytes,
        EntityStoreError* error = nullptr);

private:
    std::filesystem::path root_;
    std::size_t maximum_record_bytes_{};
};

} // namespace genesis::identity
