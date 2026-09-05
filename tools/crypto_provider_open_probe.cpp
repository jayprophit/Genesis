#include "genesis/security/provider_open_observation.hpp"

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    if (argc != 2 || std::string_view(argv[1]).empty()) {
        std::cerr << "usage: genesis_crypto_provider_open_probe <registered-provider-name>\n";
        return 2;
    }
    try {
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                                 std::chrono::system_clock::now().time_since_epoch())
                                 .count();
        if (seconds < 0) {
            std::cerr << "provider-open probe failed: system time is out of range\n";
            return 2;
        }
        const auto inventory = genesis::security::probe_registered_crypto_providers(
            static_cast<std::uint64_t>(seconds));
        const auto observation =
            genesis::security::probe_registered_crypto_provider_open(
                inventory,
                argv[1],
                static_cast<std::uint64_t>(seconds));
        std::cout
            << genesis::security::crypto_provider_open_observation_json(observation)
            << '\n';
        return observation.status
                       == genesis::security::CryptoProviderOpenStatus::observed
                   ? 0
                   : 3;
    } catch (const std::exception& exception) {
        std::cerr << "provider-open probe failed: " << exception.what() << '\n';
        return 2;
    }
}
