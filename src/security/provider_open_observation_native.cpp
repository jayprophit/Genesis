#include "genesis/security/provider_open_observation.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ncrypt.h>
#endif

namespace genesis::security {
namespace {

std::uint64_t elapsed_microseconds(
    std::chrono::steady_clock::time_point started_at) {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - started_at)
            .count());
}

bool inventory_contains(const CryptoPlatformInventory& inventory,
                        std::string_view provider_name) {
    return std::any_of(inventory.registered_providers.begin(),
                       inventory.registered_providers.end(),
                       [&](const RegisteredCryptoProviderObservation& provider) {
                           return provider.provider_name == provider_name;
                       });
}

CryptoProviderOpenObservationDraft empty_draft(
    const CryptoPlatformInventory& inventory,
    std::string_view provider_name,
    CryptoProviderOpenStatus status,
    std::uint64_t observed_at,
    std::chrono::steady_clock::time_point started_at) {
    return {inventory.platform_id,
            inventory.architecture,
            std::string(provider_name),
            inventory.evidence_digest,
            status,
            0U,
            0U,
            0U,
            0U,
            observed_at,
            elapsed_microseconds(started_at),
            false,
            false,
            false,
            false,
            false,
            false};
}

#if defined(_WIN32)

std::wstring utf16_provider_name(std::string_view provider_name) {
    const auto size = static_cast<int>(provider_name.size());
    const int required = MultiByteToWideChar(CP_UTF8,
                                             MB_ERR_INVALID_CHARS,
                                             provider_name.data(),
                                             size,
                                             nullptr,
                                             0);
    if (required <= 0) {
        throw std::runtime_error("provider name UTF-16 conversion failed");
    }
    std::wstring output(static_cast<std::size_t>(required), L'\0');
    if (MultiByteToWideChar(CP_UTF8,
                            MB_ERR_INVALID_CHARS,
                            provider_name.data(),
                            size,
                            output.data(),
                            required)
        != required) {
        throw std::runtime_error("provider name UTF-16 conversion failed");
    }
    return output;
}

#endif

} // namespace

CryptoProviderOpenObservation probe_registered_crypto_provider_open(
    const CryptoPlatformInventory& inventory,
    std::string_view provider_name,
    std::uint64_t observed_at) {
    const auto started_at = std::chrono::steady_clock::now();
    if (!inventory.verify()) {
        throw std::invalid_argument("provider-open probe requires a verified inventory");
    }
    if (!valid_registered_crypto_provider_name(provider_name)) {
        throw std::invalid_argument("provider-open probe received an invalid provider name");
    }
    if (observed_at < inventory.collected_at) {
        throw std::invalid_argument(
            "provider-open observation cannot predate its registration inventory");
    }
    if (inventory.status == CryptoPlatformInventoryStatus::unsupported_platform) {
        return make_crypto_provider_open_observation(empty_draft(
            inventory,
            provider_name,
            CryptoProviderOpenStatus::unsupported_platform,
            observed_at,
            started_at));
    }
    if (inventory.status != CryptoPlatformInventoryStatus::observed) {
        return make_crypto_provider_open_observation(empty_draft(
            inventory,
            provider_name,
            CryptoProviderOpenStatus::inventory_unavailable,
            observed_at,
            started_at));
    }
    if (!inventory_contains(inventory, provider_name)) {
        return make_crypto_provider_open_observation(empty_draft(
            inventory,
            provider_name,
            CryptoProviderOpenStatus::not_registered,
            observed_at,
            started_at));
    }

#if defined(_WIN32)
    const auto wide_name = utf16_provider_name(provider_name);
    NCRYPT_PROV_HANDLE handle = 0U;
    const SECURITY_STATUS open_status =
        NCryptOpenStorageProvider(&handle, wide_name.c_str(), 0U);
    if (open_status != ERROR_SUCCESS || handle == 0U) {
        auto draft = empty_draft(inventory,
                                 provider_name,
                                 CryptoProviderOpenStatus::open_failed,
                                 observed_at,
                                 started_at);
        draft.registration_matched = true;
        draft.provider_open_attempted = true;
        draft.open_native_status = open_status == ERROR_SUCCESS
                                       ? static_cast<std::uint32_t>(NTE_INVALID_HANDLE)
                                       : static_cast<std::uint32_t>(open_status);
        return make_crypto_provider_open_observation(std::move(draft));
    }

    DWORD implementation_type = 0U;
    DWORD bytes_written = 0U;
    SECURITY_STATUS property_status = NCryptGetProperty(
        handle,
        NCRYPT_IMPL_TYPE_PROPERTY,
        reinterpret_cast<PBYTE>(&implementation_type),
        static_cast<DWORD>(sizeof(implementation_type)),
        &bytes_written,
        NCRYPT_SILENT_FLAG);
    if (property_status == ERROR_SUCCESS
        && bytes_written != sizeof(implementation_type)) {
        property_status = NTE_INVALID_PARAMETER;
        implementation_type = 0U;
    }
    const SECURITY_STATUS release_status = NCryptFreeObject(handle);

    CryptoProviderOpenStatus status = CryptoProviderOpenStatus::observed;
    if (release_status != ERROR_SUCCESS) {
        status = CryptoProviderOpenStatus::release_failed;
    } else if (property_status != ERROR_SUCCESS) {
        status = CryptoProviderOpenStatus::property_query_failed;
    }
    CryptoProviderOpenObservationDraft draft{
        inventory.platform_id,
        inventory.architecture,
        std::string(provider_name),
        inventory.evidence_digest,
        status,
        0U,
        static_cast<std::uint32_t>(property_status),
        static_cast<std::uint32_t>(release_status),
        implementation_type,
        observed_at,
        elapsed_microseconds(started_at),
        true,
        true,
        true,
        true,
        property_status == ERROR_SUCCESS,
        release_status == ERROR_SUCCESS};
    return make_crypto_provider_open_observation(std::move(draft));
#else
    return make_crypto_provider_open_observation(empty_draft(
        inventory,
        provider_name,
        CryptoProviderOpenStatus::unsupported_platform,
        observed_at,
        started_at));
#endif
}

} // namespace genesis::security
