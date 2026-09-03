#pragma once
#include "genesis/cognition/workspace.hpp"
#include <filesystem>
#include <optional>
#include <string>
namespace genesis::cognition {
enum class BeliefStoreErrorCode { none, invalid_model, invalid_identifier, not_found, io_error, conflicting_version, corrupt_record, unsupported_schema };
struct BeliefStoreError { BeliefStoreErrorCode code{BeliefStoreErrorCode::none}; std::string message; };
class BeliefStore final {
public:
 BeliefStore(std::filesystem::path root,std::size_t maximum_record_bytes=64U*1024U*1024U);
 [[nodiscard]] bool write(const BeliefModel& model,std::string_view version,BeliefStoreError* error=nullptr)const;
 [[nodiscard]] std::optional<BeliefModel> read(std::string_view owner_id,std::string_view version,BeliefStoreError* error=nullptr)const;
 [[nodiscard]] static std::string serialize(const BeliefModel& model);
 [[nodiscard]] static std::optional<BeliefModel> deserialize(std::string_view bytes,BeliefStoreError* error=nullptr);
private: std::filesystem::path root_;std::size_t maximum_record_bytes_{};
};
}
