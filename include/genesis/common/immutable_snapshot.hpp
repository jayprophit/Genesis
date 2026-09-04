#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace genesis::storage {

enum class ImmutableFileErrorCode : std::uint8_t {
    none,
    invalid_identifier,
    not_found,
    io_error,
    conflicting_version,
    record_too_large,
    unsafe_file_type,
};

struct ImmutableFileError final {
    ImmutableFileErrorCode code{ImmutableFileErrorCode::none};
    std::string message;
};

// Shared filesystem boundary for immutable, versioned binary snapshots. Domain
// stores remain responsible for schema validation and checksums.
class ImmutableSnapshotFiles final {
public:
    explicit ImmutableSnapshotFiles(
        std::filesystem::path root,
        std::size_t maximum_record_bytes = 256U * 1024U * 1024U);

    [[nodiscard]] bool write(std::string_view owner_id,
                             std::string_view version,
                             std::string_view suffix,
                             std::string_view bytes,
                             ImmutableFileError* error = nullptr) const;
    [[nodiscard]] std::optional<std::string> read(
        std::string_view owner_id,
        std::string_view version,
        std::string_view suffix,
        ImmutableFileError* error = nullptr) const;

    [[nodiscard]] const std::filesystem::path& root() const noexcept;
    [[nodiscard]] std::size_t maximum_record_bytes() const noexcept;

private:
    [[nodiscard]] std::filesystem::path record_path(std::string_view owner_id,
                                                    std::string_view version,
                                                    std::string_view suffix) const;

    std::filesystem::path root_;
    std::size_t maximum_record_bytes_{};
};

} // namespace genesis::storage
