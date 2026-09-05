#pragma once

#include "genesis/security/platform_crypto_inventory.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace genesis::security {

// Serialized values are append-only migration boundaries.
enum class CryptoProviderOpenStatus : std::uint8_t {
    observed,
    unsupported_platform,
    inventory_unavailable,
    not_registered,
    open_failed,
    property_query_failed,
    release_failed,
};

[[nodiscard]] std::string_view to_string(CryptoProviderOpenStatus value) noexcept;
[[nodiscard]] bool crypto_provider_open_status_from_string(
    std::string_view text,
    CryptoProviderOpenStatus& value) noexcept;

struct CryptoProviderOpenObservationDraft final {
    std::string platform_id;
    std::string architecture;
    std::string provider_name;
    std::string registration_inventory_digest;
    CryptoProviderOpenStatus status{CryptoProviderOpenStatus::unsupported_platform};
    std::uint32_t open_native_status{};
    std::uint32_t property_native_status{};
    std::uint32_t release_native_status{};
    std::uint32_t implementation_type_flags{};
    std::uint64_t observed_at{};
    std::uint64_t elapsed_microseconds{};
    bool registration_matched{false};
    bool provider_open_attempted{false};
    bool provider_open_succeeded{false};
    bool implementation_type_query_attempted{false};
    bool implementation_type_query_succeeded{false};
    bool provider_handle_released{false};
};

// A provider-open observation is still not a provider qualification. Native
// handles are never exposed or stored. The record binds only open/query/release
// results and provider-reported implementation-type flags; all key and
// cryptographic capability claims remain permanently false.
struct CryptoProviderOpenObservation final {
    std::string platform_id;
    std::string architecture;
    std::string provider_name;
    std::string provider_name_digest;
    std::string registration_inventory_digest;
    CryptoProviderOpenStatus status{CryptoProviderOpenStatus::unsupported_platform};
    std::uint32_t open_native_status{};
    std::uint32_t property_native_status{};
    std::uint32_t release_native_status{};
    std::uint32_t implementation_type_flags{};
    std::uint32_t unknown_implementation_type_flags{};
    std::uint64_t observed_at{};
    std::uint64_t elapsed_microseconds{};
    bool registration_matched{false};
    bool provider_open_attempted{false};
    bool provider_open_succeeded{false};
    bool implementation_type_query_attempted{false};
    bool implementation_type_query_succeeded{false};
    bool provider_handle_released{false};
    bool provider_reports_hardware{false};
    bool provider_reports_software{false};
    bool provider_reports_removable{false};
    bool provider_reports_hardware_rng{false};
    bool provider_reports_virtual_isolation{false};
    bool key_enumerated{false};
    bool key_opened{false};
    bool key_created{false};
    bool key_exported{false};
    bool cryptographic_operation_executed{false};
    bool provider_qualified{false};
    std::string evidence_digest;

    [[nodiscard]] bool verify() const;
    [[nodiscard]] bool operator==(
        const CryptoProviderOpenObservation&) const = default;
};

[[nodiscard]] CryptoProviderOpenObservation make_crypto_provider_open_observation(
    CryptoProviderOpenObservationDraft draft);

// The supplied name must first appear in a verified registration inventory.
// Windows loads that named KSP with the API-required zero flags, reads only its
// implementation-type DWORD with silent UI suppression, and releases the
// provider handle. No key API is called. Service integrations must invoke this
// after StartService processing to avoid the documented CNG deadlock boundary.
[[nodiscard]] CryptoProviderOpenObservation probe_registered_crypto_provider_open(
    const CryptoPlatformInventory& inventory,
    std::string_view provider_name,
    std::uint64_t observed_at);

[[nodiscard]] std::string crypto_provider_open_observation_json(
    const CryptoProviderOpenObservation& observation);

} // namespace genesis::security
