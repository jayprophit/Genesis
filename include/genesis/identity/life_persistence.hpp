#pragma once

#include "genesis/identity/life_record.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace genesis::identity {

enum class LifeRecordStoreErrorCode : std::uint8_t {
    none,
    invalid_record,
    invalid_identifier,
    not_found,
    io_error,
    conflicting_version,
    corrupt_record,
    unsupported_schema,
};

struct LifeRecordStoreError final {
    LifeRecordStoreErrorCode code{LifeRecordStoreErrorCode::none};
    std::string message;
};

class LifeRecordStore final {
public:
    explicit LifeRecordStore(
        std::filesystem::path root,
        std::size_t maximum_record_bytes = 256U * 1024U * 1024U);

    [[nodiscard]] bool write(const DigitalLifeRecord& record,
                             std::string_view version,
                             LifeRecordStoreError* error = nullptr) const;
    [[nodiscard]] std::optional<DigitalLifeRecord> read(
        std::string_view record_entity_id,
        std::string_view organism_id,
        std::string_view lineage_anchor_digest,
        std::string_view version,
        LifeRecordStoreError* error = nullptr) const;

    [[nodiscard]] static std::string serialize(const DigitalLifeRecord& record);
    [[nodiscard]] static std::optional<DigitalLifeRecord> deserialize(
        std::string_view bytes,
        LifeRecordStoreError* error = nullptr);

private:
    std::filesystem::path root_;
    std::size_t maximum_record_bytes_{};
};

} // namespace genesis::identity
