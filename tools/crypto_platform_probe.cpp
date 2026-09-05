#include "genesis/security/platform_crypto_inventory.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>

int main() {
    try {
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        const auto seconds =
            std::chrono::duration_cast<std::chrono::seconds>(now).count();
        if (seconds < 0) {
            std::cerr << "platform crypto probe failed: system time is out of range\n";
            return 2;
        }
        const auto inventory = genesis::security::probe_registered_crypto_providers(
            static_cast<std::uint64_t>(seconds));
        std::cout << genesis::security::crypto_platform_inventory_json(inventory)
                  << '\n';
        return inventory.status
                       == genesis::security::CryptoPlatformInventoryStatus::observed
                   ? 0
                   : 3;
    } catch (const std::exception& exception) {
        std::cerr << "platform crypto probe failed: " << exception.what() << '\n';
        return 2;
    }
}
