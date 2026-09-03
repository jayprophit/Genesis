#pragma once
#include "genesis/perception/pipeline.hpp"
#include <filesystem>
#include <optional>
namespace genesis::perception {
enum class PerceptionStoreErrorCode { none,invalid_pipeline,invalid_identifier,not_found,io_error,conflicting_version,corrupt_record,unsupported_schema };
struct PerceptionStoreError { PerceptionStoreErrorCode code{PerceptionStoreErrorCode::none};std::string message; };
class PerceptionStore final {
public:
 PerceptionStore(std::filesystem::path root,std::size_t maximum_record_bytes=256U*1024U*1024U);
 [[nodiscard]] bool write(const PerceptionPipeline& pipeline,std::string_view version,PerceptionStoreError* error=nullptr)const;
 [[nodiscard]] std::optional<PerceptionPipeline> read(std::string_view organism_id,std::string_view version,PerceptionStoreError* error=nullptr)const;
 [[nodiscard]] static std::string serialize(const PerceptionPipeline& pipeline);
 [[nodiscard]] static std::optional<PerceptionPipeline> deserialize(std::string_view bytes,PerceptionStoreError* error=nullptr);
private:std::filesystem::path root_;std::size_t maximum_record_bytes_{};
};
}
