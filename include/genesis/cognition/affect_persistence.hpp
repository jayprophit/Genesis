#pragma once
#include "genesis/cognition/affect.hpp"
#include <filesystem>
#include <optional>
#include <string>
namespace genesis::cognition {
enum class AffectStoreErrorCode { none,invalid_model,invalid_identifier,not_found,io_error,conflicting_version,corrupt_record,unsupported_schema };
struct AffectStoreError { AffectStoreErrorCode code{AffectStoreErrorCode::none};std::string message; };
class AffectStore final {
public:
 AffectStore(std::filesystem::path root,std::size_t maximum_record_bytes=128U*1024U*1024U);
 [[nodiscard]] bool write(const AffectRegulator& model,std::string_view version,AffectStoreError* error=nullptr)const;
 [[nodiscard]] std::optional<AffectRegulator> read(std::string_view organism_id,std::string_view version,AffectStoreError* error=nullptr)const;
 [[nodiscard]] static std::string serialize(const AffectRegulator& model);
 [[nodiscard]] static std::optional<AffectRegulator> deserialize(std::string_view bytes,AffectStoreError* error=nullptr);
private:std::filesystem::path root_;std::size_t maximum_record_bytes_{};
};
}
