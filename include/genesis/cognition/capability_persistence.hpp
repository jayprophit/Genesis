#pragma once
#include "genesis/cognition/capability.hpp"
#include <filesystem>
#include <optional>
#include <string>
namespace genesis::cognition {
enum class CapabilityStoreErrorCode { none,invalid_model,invalid_identifier,not_found,io_error,conflicting_version,corrupt_record,unsupported_schema };
struct CapabilityStoreError { CapabilityStoreErrorCode code{CapabilityStoreErrorCode::none};std::string message; };
class CapabilityStore final {
public:
 CapabilityStore(std::filesystem::path root,std::size_t maximum_record_bytes=64U*1024U*1024U);
 [[nodiscard]] bool write(const SelfCapabilityModel& model,std::string_view version,CapabilityStoreError* error=nullptr)const;
 [[nodiscard]] std::optional<SelfCapabilityModel> read(std::string_view organism_id,std::string_view version,CapabilityStoreError* error=nullptr)const;
 [[nodiscard]] static std::string serialize(const SelfCapabilityModel& model);
 [[nodiscard]] static std::optional<SelfCapabilityModel> deserialize(std::string_view bytes,CapabilityStoreError* error=nullptr);
private:std::filesystem::path root_;std::size_t maximum_record_bytes_{};
};
}
