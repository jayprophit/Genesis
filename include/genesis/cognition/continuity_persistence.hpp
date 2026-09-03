#pragma once
#include "genesis/cognition/continuity.hpp"
#include <filesystem>
#include <optional>
#include <string>
namespace genesis::cognition {
enum class ContinuityStoreErrorCode { none,invalid_journal,invalid_identifier,not_found,io_error,conflicting_version,corrupt_record,unsupported_schema };
struct ContinuityStoreError { ContinuityStoreErrorCode code{ContinuityStoreErrorCode::none};std::string message; };
class ContinuityStore final {
public:
 ContinuityStore(std::filesystem::path root,std::size_t maximum_record_bytes=128U*1024U*1024U);
 [[nodiscard]] bool write(const AutobiographicalContinuity& journal,std::string_view version,ContinuityStoreError* error=nullptr)const;
 [[nodiscard]] std::optional<AutobiographicalContinuity> read(std::string_view organism_id,std::string_view version,ContinuityStoreError* error=nullptr)const;
 [[nodiscard]] static std::string serialize(const AutobiographicalContinuity& journal);
 [[nodiscard]] static std::optional<AutobiographicalContinuity> deserialize(std::string_view bytes,ContinuityStoreError* error=nullptr);
private:std::filesystem::path root_;std::size_t maximum_record_bytes_{};
};
}
