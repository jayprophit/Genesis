#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace genesis {

enum class OriginKind { genesis, restore, clone, fork, child };

struct LineageIdentity final {
    std::string organism_id;
    std::string genesis_id;
    std::string lineage_id;
    std::string birth_event_id;
    std::string parent_a_id;
    std::string parent_b_id;
    std::string genome_hash;
    std::string inherited_state_hash;
    std::string birth_snapshot_hash;
    std::string identity_seed;
    std::string lineage_signature;
    std::string cryptographic_provenance;
    std::uint64_t generation{};
    std::int64_t birth_timestamp{};
    std::vector<std::string> ancestor_root_ids;
    OriginKind origin{OriginKind::genesis};
};

[[nodiscard]] std::vector<std::string> validate(const LineageIdentity& identity);
[[nodiscard]] bool represents_continuation(OriginKind kind) noexcept;

} // namespace genesis

