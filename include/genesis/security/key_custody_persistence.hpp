#pragma once

#include "genesis/security/key_custody.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace genesis::security {

enum class KeyCustodyStoreErrorCode : std::uint8_t {
    none,
    invalid_identifier,
    invalid_registry,
    not_found,
    conflicting_version,
    corrupt_record,
    unsupported_schema,
    owner_binding_mismatch,
    io_error,
};

struct KeyCustodyStoreError final {
    KeyCustodyStoreErrorCode code{KeyCustodyStoreErrorCode::none};
    std::string message;
};

// Immutable snapshots contain key metadata and evidence digests only. They do
// not contain provider locator plaintext, key bytes, wrapped keys, PINs,
// passphrases, recovery secrets or provider credentials.
class KeyCustodyStore final {
public:
    explicit KeyCustodyStore(
        std::filesystem::path root,
        std::size_t maximum_record_bytes = 256U * 1024U * 1024U);

    [[nodiscard]] bool write(std::string_view owner_entity_id,
                             std::string_view version,
                             const KeyCustodyRegistry& registry,
                             KeyCustodyStoreError* error = nullptr) const;
    [[nodiscard]] std::optional<KeyCustodyRegistry> read(
        std::string_view owner_entity_id,
        std::string_view version,
        KeyCustodyStoreError* error = nullptr) const;

    [[nodiscard]] static std::string serialize(const KeyCustodyRegistry& registry);
    [[nodiscard]] static std::optional<KeyCustodyRegistry> deserialize(
        std::string_view bytes,
        KeyCustodyStoreError* error = nullptr);

    [[nodiscard]] const std::filesystem::path& root() const noexcept;
    [[nodiscard]] std::size_t maximum_record_bytes() const noexcept;

private:
    std::filesystem::path root_;
    std::size_t maximum_record_bytes_{};
};

} // namespace genesis::security
