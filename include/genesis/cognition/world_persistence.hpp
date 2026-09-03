#pragma once
#include "genesis/cognition/world_model.hpp"
#include <filesystem>
#include <optional>
#include <string>
namespace genesis::cognition {
enum class WorldStoreErrorCode { none, invalid_model, invalid_identifier, not_found, io_error, conflicting_version, corrupt_record, unsupported_schema };
struct WorldStoreError { WorldStoreErrorCode code{WorldStoreErrorCode::none};std::string message; };
class WorldDynamicsStore final {
public:
 WorldDynamicsStore(std::filesystem::path root,std::size_t maximum_record_bytes=128U*1024U*1024U);
 [[nodiscard]] bool write(const WorldDynamics& world,std::string_view version,WorldStoreError* error=nullptr)const;
 [[nodiscard]] std::optional<WorldDynamics> read(std::string_view organism_id,std::string_view version,WorldStoreError* error=nullptr)const;
 [[nodiscard]] static std::string serialize(const WorldDynamics& world);
 [[nodiscard]] static std::optional<WorldDynamics> deserialize(std::string_view bytes,WorldStoreError* error=nullptr);
private:std::filesystem::path root_;std::size_t maximum_record_bytes_{};
};
}
