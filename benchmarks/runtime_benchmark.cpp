#include "genesis/runtime/runtime.hpp"

#include <chrono>
#include <iostream>

int main() {
    constexpr std::size_t count = 100000;
    genesis::runtime::DeterministicDispatcher dispatcher{1024};
    std::size_t handled = 0;
    static_cast<void>(dispatcher.subscribe("tick", [&](const auto&) { ++handled; }));
    const auto start = std::chrono::steady_clock::now();
    for (std::size_t index = 0; index < count; ++index) {
        const std::optional<std::string> parent =
            index == 0 ? std::nullopt
                       : std::optional<std::string>{"event-" + std::to_string(index - 1)};
        static_cast<void>(dispatcher.publish({"event-" + std::to_string(index),
                                               "benchmark",
                                               "tick",
                                               parent,
                                               genesis::runtime::sha256(std::to_string(index))}));
    }
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "events=" << count << " handled=" << handled << " seconds=" << elapsed
              << " events_per_second=" << (static_cast<double>(count) / elapsed)
              << " history=" << dispatcher.history_size()
              << " evictions=" << dispatcher.history_eviction_count() << '\n';
    return handled == count ? 0 : 1;
}
