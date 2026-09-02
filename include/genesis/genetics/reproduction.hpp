#pragma once

#include "genesis/genetics/genome.hpp"
#include "genesis/memory/origin.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace genesis::genetics {

struct MutationRecord final {
    std::string mutation_id;
    std::string location;
    std::string original_state;
    std::string new_state;
    std::string cause;
    double probability{};
    std::uint64_t generation{};
    std::string predicted_effect;
    std::string observed_effect;
    std::string fitness_result;
};

struct ParentPackage final {
    LineageIdentity identity;
    Genome genome;
    // This vector is the caller's pre-birth snapshot. Birth copies values only;
    // it never retains a reference to a parent's mutable memory.
    std::vector<memory::InheritedMemory> inherited_memory;
};

struct BirthRequest final {
    ParentPackage parent_a;
    ParentPackage parent_b;
    std::string child_organism_id;
    std::string birth_event_id;
    std::int64_t birth_timestamp{};
    std::uint64_t seed{};
    double mutation_probability{};
};

enum class BirthErrorCode {
    none,
    invalid_request,
    invalid_parent,
    duplicate_parent,
    invalid_probability,
    recombination_failed,
};

struct BirthError final {
    BirthErrorCode code{BirthErrorCode::none};
    std::string message;
};

struct BirthResult final {
    Genome genome;
    LineageIdentity identity;
    std::vector<memory::InheritedMemory> inherited_memory;
    std::vector<MutationRecord> mutations;
    std::string birth_snapshot_hash;
    std::string transaction_id;
};

class BirthTransaction final {
public:
    [[nodiscard]] static std::optional<BirthResult> execute(
        const BirthRequest& request,
        BirthError* error = nullptr);
};

} // namespace genesis::genetics
