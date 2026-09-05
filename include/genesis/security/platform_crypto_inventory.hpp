#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::security {

// Serialized enum values are append-only migration boundaries.
enum class CryptoPlatformInventoryStatus : std::uint8_t {
    observed,
    unsupported_platform,
    enumeration_failed,
};

[[nodiscard]] std::string_view to_string(
    CryptoPlatformInventoryStatus value) noexcept;
[[nodiscard]] bool crypto_platform_inventory_status_from_string(
    std::string_view text,
    CryptoPlatformInventoryStatus& value) noexcept;

struct RegisteredCryptoProviderObservation final {
    std::string provider_name;
    std::string provider_name_digest;

    [[nodiscard]] bool operator==(
        const RegisteredCryptoProviderObservation&) const = default;
};

struct CryptoPlatformInventoryDraft final {
    std::string platform_id;
    std::string architecture;
    CryptoPlatformInventoryStatus status{
        CryptoPlatformInventoryStatus::unsupported_platform};
    std::uint32_t native_status{};
    std::uint64_t collected_at{};
    std::uint64_t elapsed_microseconds{};
    bool provider_registry_enumeration_attempted{false};
    std::vector<std::string> registered_provider_names;
};

// This record is deliberately narrower than provider qualification. It proves
// only that an operating-system provider registration was observed. The fixed
// false fields prevent registration from being confused with provider access,
// key access, cryptographic operation, or qualification.
struct CryptoPlatformInventory final {
    std::string platform_id;
    std::string architecture;
    CryptoPlatformInventoryStatus status{
        CryptoPlatformInventoryStatus::unsupported_platform};
    std::uint32_t native_status{};
    std::uint64_t collected_at{};
    std::uint64_t elapsed_microseconds{};
    bool provider_registry_enumeration_attempted{false};
    bool provider_registry_enumeration_succeeded{false};
    bool provider_opened{false};
    bool key_enumerated{false};
    bool key_opened{false};
    bool key_created{false};
    bool cryptographic_operation_executed{false};
    bool provider_qualified{false};
    std::vector<RegisteredCryptoProviderObservation> registered_providers;
    std::string evidence_digest;

    [[nodiscard]] bool verify() const;
    [[nodiscard]] bool operator==(const CryptoPlatformInventory&) const = default;
};

[[nodiscard]] CryptoPlatformInventory make_crypto_platform_inventory(
    CryptoPlatformInventoryDraft draft);

// On Windows this performs exactly one silent NCryptEnumStorageProviders call.
// It does not open a provider or enumerate, create, open, export, use, or delete
// keys. Microsoft prohibits this API during service StartService processing;
// callers must collect after service startup. Other platforms return a verified
// unsupported_platform observation.
[[nodiscard]] CryptoPlatformInventory probe_registered_crypto_providers(
    std::uint64_t collected_at);

// Deterministic, single-line JSON intended for local evidence capture. It
// contains provider registration names, never key material or native handles.
[[nodiscard]] std::string crypto_platform_inventory_json(
    const CryptoPlatformInventory& inventory);

} // namespace genesis::security
