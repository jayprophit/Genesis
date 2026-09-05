#include "genesis/security/platform_crypto_inventory.hpp"

#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ncrypt.h>
#endif

namespace genesis::security {
namespace {

std::string native_architecture() {
#if defined(_M_ARM64) || defined(__aarch64__)
    return "arm64";
#elif defined(_M_X64) || defined(__x86_64__)
    return "x86_64";
#elif defined(_M_IX86) || defined(__i386__)
    return "x86";
#else
    return "unknown";
#endif
}

std::uint64_t elapsed_microseconds(
    std::chrono::steady_clock::time_point started_at) {
    const auto elapsed = std::chrono::steady_clock::now() - started_at;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
}

#if defined(_WIN32)

class ProviderList final {
public:
    explicit ProviderList(NCryptProviderName* providers) noexcept
        : providers_(providers) {}
    ~ProviderList() noexcept {
        if (providers_ != nullptr) {
            static_cast<void>(NCryptFreeBuffer(providers_));
        }
    }

    ProviderList(const ProviderList&) = delete;
    ProviderList& operator=(const ProviderList&) = delete;

private:
    NCryptProviderName* providers_{};
};

std::string utf8_provider_name(const wchar_t* name) {
    if (name == nullptr || *name == L'\0') {
        throw std::runtime_error("CNG returned an empty provider name");
    }
    const int required = WideCharToMultiByte(CP_UTF8,
                                             WC_ERR_INVALID_CHARS,
                                             name,
                                             -1,
                                             nullptr,
                                             0,
                                             nullptr,
                                             nullptr);
    if (required <= 1 || required > 4097) {
        throw std::runtime_error("CNG provider name is invalid or exceeds the limit");
    }
    std::string output(static_cast<std::size_t>(required), '\0');
    const int written = WideCharToMultiByte(CP_UTF8,
                                            WC_ERR_INVALID_CHARS,
                                            name,
                                            -1,
                                            output.data(),
                                            required,
                                            nullptr,
                                            nullptr);
    if (written != required) {
        throw std::runtime_error("CNG provider name UTF-8 conversion failed");
    }
    output.pop_back();
    return output;
}

#endif

} // namespace

CryptoPlatformInventory probe_registered_crypto_providers(
    std::uint64_t collected_at) {
    const auto started_at = std::chrono::steady_clock::now();
#if defined(_WIN32)
    DWORD count = 0U;
    NCryptProviderName* raw_providers = nullptr;
    const SECURITY_STATUS status = NCryptEnumStorageProviders(
        &count,
        &raw_providers,
        NCRYPT_SILENT_FLAG);
    ProviderList release(raw_providers);
    if (status != ERROR_SUCCESS) {
        return make_crypto_platform_inventory(
            {"windows-cng-ncrypt",
             native_architecture(),
             CryptoPlatformInventoryStatus::enumeration_failed,
             static_cast<std::uint32_t>(status),
             collected_at,
             elapsed_microseconds(started_at),
             true,
             {}});
    }
    if (count > 256U || (count != 0U && raw_providers == nullptr)) {
        return make_crypto_platform_inventory(
            {"windows-cng-ncrypt",
             native_architecture(),
             CryptoPlatformInventoryStatus::enumeration_failed,
             static_cast<std::uint32_t>(ERROR_INVALID_DATA),
             collected_at,
             elapsed_microseconds(started_at),
             true,
             {}});
    }
    try {
        std::vector<std::string> names;
        names.reserve(count);
        for (DWORD index = 0U; index < count; ++index) {
            names.push_back(utf8_provider_name(raw_providers[index].pszName));
        }
        return make_crypto_platform_inventory(
            {"windows-cng-ncrypt",
             native_architecture(),
             CryptoPlatformInventoryStatus::observed,
             0U,
             collected_at,
             elapsed_microseconds(started_at),
             true,
             std::move(names)});
    } catch (...) {
        return make_crypto_platform_inventory(
            {"windows-cng-ncrypt",
             native_architecture(),
             CryptoPlatformInventoryStatus::enumeration_failed,
             static_cast<std::uint32_t>(ERROR_INVALID_DATA),
             collected_at,
             elapsed_microseconds(started_at),
             true,
             {}});
    }
#else
    return make_crypto_platform_inventory(
        {"unsupported",
         native_architecture(),
         CryptoPlatformInventoryStatus::unsupported_platform,
         0U,
         collected_at,
         elapsed_microseconds(started_at),
         false,
         {}});
#endif
}

} // namespace genesis::security
