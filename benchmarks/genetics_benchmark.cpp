#include "genesis/genesis.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>

namespace {

genesis::genetics::ParentPackage parent(std::string organism, std::string suffix) {
    genesis::genetics::ParentPackage result;
    result.identity.organism_id = organism;
    result.identity.genesis_id = "genesis-root";
    result.identity.lineage_id = "lineage-" + organism;
    result.identity.birth_event_id = "birth-" + organism;
    result.identity.identity_seed = "seed-" + organism;
    result.identity.lineage_signature = "PENDING_CRYPTOGRAPHIC_PROVIDER";
    result.identity.cryptographic_provenance = "UNAVAILABLE:provider-required";
    result.identity.generation = 1;
    result.identity.origin = genesis::OriginKind::restore;
    result.identity.ancestor_root_ids = {"genesis-root"};
    result.genome.genome_id = organism + ".genome";
    result.genome.schema_version = "1.0.0";
    result.genome.structural_strand = {
        {"shared", "shared-" + suffix, "payload"},
        {"unique-" + suffix, "unique-" + suffix, "payload"},
    };
    result.genome.regulatory_strand = {{"shared", "development", "always", 0.5}};
    result.genome.lineage_strand = result.identity;
    result.identity.genome_hash = genesis::genetics::GenomeStore::content_digest(result.genome);
    result.genome.lineage_strand.genome_hash = result.identity.genome_hash;
    return result;
}

} // namespace

int main() {
    genesis::genetics::BirthRequest request;
    request.parent_a = parent("bench-a", "a");
    request.parent_b = parent("bench-b", "b");
    request.birth_timestamp = 100;
    request.mutation_probability = 0.25;

    constexpr std::uint64_t iterations = 10000;
    const auto start = std::chrono::steady_clock::now();
    std::uint64_t successful = 0;
    std::string final_digest;
    for (std::uint64_t index = 0; index < iterations; ++index) {
        request.child_organism_id = "bench-child-" + std::to_string(index);
        request.birth_event_id = "bench-birth-" + std::to_string(index);
        request.seed = index;
        genesis::genetics::BirthError error;
        const auto result = genesis::genetics::BirthTransaction::execute(request, &error);
        if (result) {
            ++successful;
            final_digest = genesis::genetics::GenomeStore::content_digest(result->genome);
        }
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    std::cout << "genetics_birth_iterations=" << iterations
              << " successful=" << successful
              << " elapsed_ms=" << elapsed.count()
              << " final_content_digest=" << final_digest << '\n';
    return successful == iterations ? 0 : 1;
}
