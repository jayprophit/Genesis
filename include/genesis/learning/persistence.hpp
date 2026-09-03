#pragma once
#include "genesis/learning/consolidation.hpp"
#include <filesystem>
#include <optional>
#include <string>

namespace genesis::learning {
enum class RetentionStoreErrorCode { none, invalid_evaluator, invalid_identifier, not_found, io_error, conflicting_version, corrupt_record, unsupported_schema };
struct RetentionStoreError { RetentionStoreErrorCode code{RetentionStoreErrorCode::none}; std::string message; };
class RetentionStore final {
public:
 RetentionStore(std::filesystem::path root,std::size_t maximum_record_bytes=64U*1024U*1024U);
 [[nodiscard]] bool write(const RetentionEvaluator& evaluator,std::string_view version,RetentionStoreError* error=nullptr)const;
 [[nodiscard]] std::optional<RetentionEvaluator> read(std::string_view organism_id,std::string_view version,RetentionStoreError* error=nullptr)const;
 [[nodiscard]] static std::string serialize(const RetentionEvaluator& evaluator);
 [[nodiscard]] static std::optional<RetentionEvaluator> deserialize(std::string_view bytes,RetentionStoreError* error=nullptr);
private:
 std::filesystem::path root_;
 std::size_t maximum_record_bytes_{};
};
}
