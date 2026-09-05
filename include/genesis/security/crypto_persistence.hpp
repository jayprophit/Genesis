#pragma once

#include "genesis/security/crypto_provider.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace genesis::security {

enum class CryptoStoreErrorCode : std::uint8_t {
    none,
    invalid_registry,
    invalid_identifier,
    not_found,
    io_error,
    conflicting_version,
    corrupt_record,
    unsupported_schema,
};

struct CryptoStoreError final {
    CryptoStoreErrorCode code{CryptoStoreErrorCode::none};
    std::string message;
};

// Immutable recovery for registry evidence only. Secret keys and provider
// runtime state are deliberately outside this format.
class CryptoProviderStore final {
public:
    explicit CryptoProviderStore(
        std::filesystem::path root,
        std::size_t maximum_record_bytes = 256U * 1024U * 1024U);

    [[nodiscard]] bool write(const CryptoProviderRegistry& registry,
                             std::string_view version,
                             CryptoStoreError* error = nullptr) const;
    [[nodiscard]] std::optional<CryptoProviderRegistry> read(
        std::string_view registry_id,
        std::string_view entity_namespace_id,
        std::string_view owner_entity_id,
        std::string_view version,
        CryptoStoreError* error = nullptr) const;

    [[nodiscard]] static std::string serialize(
        const CryptoProviderRegistry& registry);
    [[nodiscard]] static std::optional<CryptoProviderRegistry> deserialize(
        std::string_view bytes,
        CryptoStoreError* error = nullptr);

private:
    std::filesystem::path root_;
    std::size_t maximum_record_bytes_{};
};

} // namespace genesis::security
