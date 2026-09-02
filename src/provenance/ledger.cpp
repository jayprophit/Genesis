#include "genesis/provenance/ledger.hpp"

#include <iomanip>
#include <sstream>
#include <utility>

namespace genesis {

std::string diagnostic_checksum(const std::string& value) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const unsigned char character : value) {
        hash ^= character;
        hash *= 1099511628211ULL;
    }
    std::ostringstream output;
    output << std::hex << std::setw(16) << std::setfill('0') << hash;
    return output.str();
}

std::string checksum(const std::string& value) {
    return diagnostic_checksum(value);
}

std::string material(const LedgerEvent& event) {
    return std::to_string(event.sequence) + '|' + std::to_string(event.logical_time) + '|'
        + event.event_id + '|' + event.actor_id + '|' + event.kind + '|'
        + event.payload_digest + '|' + event.previous_digest;
}

const LedgerEvent& ProvenanceLedger::append(std::int64_t logical_time,
                                            std::string event_id,
                                            std::string actor_id,
                                            std::string kind,
                                            std::string payload_digest) {
    LedgerEvent event;
    event.sequence = events_.size();
    event.logical_time = logical_time;
    event.event_id = std::move(event_id);
    event.actor_id = std::move(actor_id);
    event.kind = std::move(kind);
    event.payload_digest = std::move(payload_digest);
    event.previous_digest = events_.empty() ? "GENESIS" : events_.back().digest;
    event.digest = diagnostic_checksum(material(event));
    events_.push_back(std::move(event));
    return events_.back();
}

bool ProvenanceLedger::verify() const {
    for (std::size_t index = 0; index < events_.size(); ++index) {
        const auto& event = events_[index];
        const auto expected_previous = index == 0 ? std::string{"GENESIS"} : events_[index - 1].digest;
        if (event.sequence != index || event.previous_digest != expected_previous
            || event.digest != diagnostic_checksum(material(event))) {
            return false;
        }
    }
    return true;
}

std::size_t ProvenanceLedger::size() const noexcept {
    return events_.size();
}

const std::vector<LedgerEvent>& ProvenanceLedger::events() const noexcept {
    return events_;
}

} // namespace genesis

