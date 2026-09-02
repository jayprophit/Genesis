#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace genesis {

struct LedgerEvent final {
    std::uint64_t sequence{};
    std::int64_t logical_time{};
    std::string event_id;
    std::string actor_id;
    std::string kind;
    std::string payload_digest;
    std::string previous_digest;
    std::string digest;
};

// Non-cryptographic FNV-1a integrity marker retained for baseline compatibility.
// It detects accidental corruption only and is not an authentication primitive.
[[nodiscard]] std::string diagnostic_checksum(const std::string& value);
[[nodiscard]] std::string checksum(const std::string& value);
[[nodiscard]] std::string material(const LedgerEvent& event);

class ProvenanceLedger final {
public:
    [[nodiscard]] const LedgerEvent& append(std::int64_t logical_time,
                                            std::string event_id,
                                            std::string actor_id,
                                            std::string kind,
                                            std::string payload_digest);
    [[nodiscard]] bool verify() const;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] const std::vector<LedgerEvent>& events() const noexcept;

private:
    std::vector<LedgerEvent> events_;
};

} // namespace genesis

