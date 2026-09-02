#include "genesis/genesis.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

genesis::LineageIdentity make_identity(const std::string& organism_id,
                                      const std::string& genesis_id,
                                      std::uint64_t generation) {
    genesis::LineageIdentity result;
    result.organism_id = organism_id;
    result.genesis_id = genesis_id;
    result.lineage_id = "lineage-" + organism_id;
    result.birth_event_id = "birth-" + organism_id;
    result.identity_seed = "seed-" + organism_id;
    result.lineage_signature = "PENDING_CRYPTOGRAPHIC_PROVIDER";
    result.cryptographic_provenance = "UNAVAILABLE:provider-required";
    result.generation = generation;
    result.birth_timestamp = static_cast<std::int64_t>(generation);
    result.origin = genesis::OriginKind::restore;
    result.ancestor_root_ids = {genesis_id};
    return result;
}

genesis::genetics::ParentPackage make_parent(const std::string& organism_id,
                                             const std::string& genesis_id,
                                             std::uint64_t generation,
                                             std::string gene_suffix,
                                             std::string memory_id,
                                             std::int64_t memory_timestamp) {
    genesis::genetics::ParentPackage result;
    result.identity = make_identity(organism_id, genesis_id, generation);
    result.genome.genome_id = organism_id + ".genome";
    result.genome.schema_version = "1.0.0";
    result.genome.structural_strand = {
        {"shared", "shared-" + gene_suffix, "payload-shared"},
        {"unique-" + gene_suffix, "unique-" + gene_suffix, "payload-unique"},
    };
    result.genome.regulatory_strand = {
        {"shared", "development", "always", 0.5},
        {"unique-" + gene_suffix, "development", "always", 0.75},
    };
    result.genome.lineage_strand = result.identity;
    result.identity.genome_hash = genesis::genetics::GenomeStore::content_digest(result.genome);
    result.genome.lineage_strand.genome_hash = result.identity.genome_hash;

    genesis::memory::InheritedMemory memory;
    memory.memory_id = std::move(memory_id);
    memory.origin = genesis::memory::Origin::direct_experience;
    memory.source_parent = "experience-source";
    memory.source_digest = "memory-source-digest";
    memory.source_timestamp = memory_timestamp;
    memory.inheritance_timestamp = memory_timestamp;
    memory.confidence = 0.9;
    memory.transformation_history = {"compressed"};
    memory.compression_history = {"lossless"};
    result.inherited_memory = {std::move(memory)};
    return result;
}

genesis::genetics::BirthRequest make_request() {
    genesis::genetics::BirthRequest request;
    request.parent_a = make_parent("parent-a", "genesis-root", 2, "a", "memory-a", 20);
    request.parent_b = make_parent("parent-b", "genesis-root", 4, "b", "memory-b", 30);
    request.child_organism_id = "child-1";
    request.birth_event_id = "birth-child-1";
    request.birth_timestamp = 100;
    request.seed = 42;
    request.mutation_probability = 1.0;
    return request;
}

void test_origin_codec() {
    for (const auto origin : {genesis::memory::Origin::inherited_parent_a,
                              genesis::memory::Origin::inherited_parent_b,
                              genesis::memory::Origin::direct_experience,
                              genesis::memory::Origin::taught,
                              genesis::memory::Origin::observed,
                              genesis::memory::Origin::inferred,
                              genesis::memory::Origin::simulated,
                              genesis::memory::Origin::generated,
                              genesis::memory::Origin::external_source}) {
        genesis::memory::Origin decoded{};
        check(genesis::memory::origin_from_string(genesis::memory::to_string(origin), decoded),
              "memory origin failed to decode");
        check(decoded == origin, "memory origin round trip changed value");
    }
    genesis::memory::Origin ignored{};
    check(!genesis::memory::origin_from_string("UNKNOWN", ignored),
          "unknown memory origin was accepted");
    check(genesis::memory::to_string(static_cast<genesis::memory::Origin>(99)) == "UNKNOWN",
          "invalid memory origin did not remain bounded");
}

void test_genome_store() {
    auto request = make_request();
    const auto genome = request.parent_a.genome;
    const auto bytes = genesis::genetics::GenomeStore::serialize(genome);
    genesis::genetics::GenomeStoreError error;
    const auto decoded = genesis::genetics::GenomeStore::deserialize(bytes, &error);
    check(decoded.has_value(), "genome deserialize failed");
    check(error.code == genesis::genetics::GenomeStoreErrorCode::none,
          "successful genome deserialize returned an error");
    check(genesis::genetics::GenomeStore::serialize(*decoded) == bytes,
          "genome serialization is not deterministic");
    check(genesis::genetics::GenomeStore::digest(genome)
              == genesis::runtime::sha256(bytes),
          "genome digest does not match serialized bytes");

    auto lineage_changed = genome;
    lineage_changed.lineage_strand.genome_hash = "different-self-reference";
    check(genesis::genetics::GenomeStore::content_digest(lineage_changed)
              == genesis::genetics::GenomeStore::content_digest(genome),
          "content digest included self-referential lineage state");

    const auto malformed = genesis::genetics::GenomeStore::deserialize("GENESIS-GENOME-V1", &error);
    check(!malformed.has_value() && error.code == genesis::genetics::GenomeStoreErrorCode::corrupt_record,
          "truncated genome record was accepted");

    const auto root = std::filesystem::temp_directory_path() / "genesis-stage2-genome-store";
    std::error_code cleanup_error;
    std::filesystem::remove_all(root, cleanup_error);
    genesis::genetics::GenomeStore store(root);
    check(store.write(genome, &error), "genome store write failed");
    check(store.contains(genome.genome_id, genome.schema_version),
          "genome store does not contain committed record");
    const auto stored = store.read(genome.genome_id, genome.schema_version, &error);
    check(stored.has_value(), "genome store read failed");
    check(genesis::genetics::GenomeStore::serialize(*stored) == bytes,
          "genome store round trip changed bytes");
    check(store.write(genome, &error), "idempotent genome store write failed");

    auto conflicting = genome;
    conflicting.structural_strand.front().variant = "conflicting-version";
    check(!store.write(conflicting, &error)
              && error.code == genesis::genetics::GenomeStoreErrorCode::conflicting_version,
          "conflicting immutable genome version was accepted");
    check(!store.read("../escape", genome.schema_version, &error)
              && error.code == genesis::genetics::GenomeStoreErrorCode::invalid_identifier,
          "unsafe genome identifier was accepted");

    std::filesystem::remove_all(root, cleanup_error);
}

void test_birth_transaction() {
    const auto request = make_request();
    const auto parent_a_before = genesis::genetics::GenomeStore::serialize(request.parent_a.genome);
    const auto parent_b_before = genesis::genetics::GenomeStore::serialize(request.parent_b.genome);

    genesis::genetics::BirthError error;
    const auto first = genesis::genetics::BirthTransaction::execute(request, &error);
    check(first.has_value(), "deterministic birth failed");
    check(error.code == genesis::genetics::BirthErrorCode::none,
          "successful birth returned an error");
    const auto second = genesis::genetics::BirthTransaction::execute(request, &error);
    check(second.has_value(), "repeat deterministic birth failed");
    check(genesis::genetics::GenomeStore::serialize(first->genome)
              == genesis::genetics::GenomeStore::serialize(second->genome),
          "same birth seed produced different genome bytes");
    check(first->transaction_id == second->transaction_id
              && first->birth_snapshot_hash == second->birth_snapshot_hash,
          "same birth seed produced different transaction identity");
    check(first->mutations.size() == first->genome.structural_strand.size(),
          "mutation probability one did not record each structural mutation");
    check(first->inherited_memory.size() == 2, "parent memory was not inherited");
    check(first->inherited_memory[0].origin == genesis::memory::Origin::inherited_parent_a,
          "parent A memory origin was not preserved");
    check(first->inherited_memory[1].origin == genesis::memory::Origin::inherited_parent_b,
          "parent B memory origin was not preserved");
    for (const auto& memory : first->inherited_memory) {
        check(memory.inheritance_timestamp == request.birth_timestamp,
              "inherited memory crossed the birth boundary");
        check(std::find(memory.transformation_history.begin(), memory.transformation_history.end(),
                        "birth_cutoff") != memory.transformation_history.end(),
              "birth cutoff transformation was not recorded");
    }
    check(first->identity.organism_id == request.child_organism_id,
          "child organism identity is incorrect");
    check(first->identity.origin == genesis::OriginKind::child,
          "child origin was not recorded");
    check(first->identity.generation == 5, "child generation did not advance from max parent");
    check(first->identity.parent_a_id == request.parent_a.identity.organism_id
              && first->identity.parent_b_id == request.parent_b.identity.organism_id,
          "child parentage was not recorded");
    check(first->identity.genome_hash == first->genome.lineage_strand.genome_hash,
          "child genome hash is not bound to its lineage strand");
    check(first->identity.birth_snapshot_hash == first->birth_snapshot_hash
              && first->genome.lineage_strand.birth_snapshot_hash == first->birth_snapshot_hash,
          "birth snapshot hash is not bound consistently");
    check(genesis::validate(first->identity).empty(), "child identity is invalid");
    check(genesis::validate(first->genome).empty(), "child genome is invalid");
    check(genesis::genetics::GenomeStore::serialize(request.parent_a.genome) == parent_a_before,
          "birth mutated parent A genome");
    check(genesis::genetics::GenomeStore::serialize(request.parent_b.genome) == parent_b_before,
          "birth mutated parent B genome");

    auto duplicate = request;
    duplicate.parent_b.identity.organism_id = duplicate.parent_a.identity.organism_id;
    check(!genesis::genetics::BirthTransaction::execute(duplicate, &error)
              && error.code == genesis::genetics::BirthErrorCode::duplicate_parent,
          "duplicate parents were accepted");

    auto future_memory = request;
    future_memory.parent_a.inherited_memory.front().source_timestamp = 101;
    check(!genesis::genetics::BirthTransaction::execute(future_memory, &error)
              && error.code == genesis::genetics::BirthErrorCode::invalid_parent,
          "post-birth memory was accepted as inherited state");

    auto invalid_probability = request;
    invalid_probability.mutation_probability = 2.0;
    check(!genesis::genetics::BirthTransaction::execute(invalid_probability, &error)
              && error.code == genesis::genetics::BirthErrorCode::invalid_probability,
          "out-of-range mutation probability was accepted");

    auto mismatched_lineage = request;
    mismatched_lineage.parent_a.genome.lineage_strand.lineage_id = "tampered-lineage";
    check(!genesis::genetics::BirthTransaction::execute(mismatched_lineage, &error)
              && error.code == genesis::genetics::BirthErrorCode::invalid_parent,
          "identity and genome lineage mismatch was accepted");
}

} // namespace

int main() {
    try {
        test_origin_codec();
        test_genome_store();
        test_birth_transaction();
        std::cout << "Genesis genetics tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
