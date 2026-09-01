#include "genesis/genesis.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unordered_set>

namespace {

void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

genesis::LineageIdentity identity() {
    genesis::LineageIdentity result;
    result.organism_id = "o";
    result.genesis_id = "g";
    result.lineage_id = "l";
    result.birth_event_id = "b";
    result.genome_hash = "gh";
    result.inherited_state_hash = "s";
    result.birth_snapshot_hash = "bs";
    result.identity_seed = "seed";
    result.lineage_signature = "PENDING_CRYPTO";
    result.cryptographic_provenance = "UNAVAILABLE";
    return result;
}

void validate_source_manifest(const std::filesystem::path& path) {
    std::ifstream input(path);
    check(input.good(), "source manifest cannot be opened");

    std::string line;
    check(static_cast<bool>(std::getline(input, line)), "source manifest header missing");
    check(line == "source_id\tpath\tbytes\tsha256\trole\tdecision", "source manifest header invalid");

    std::unordered_set<std::string> source_ids;
    std::size_t rows = 0;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const auto fields = genesis::split_fields(line, '\t');
        check(fields.size() == 6, "source manifest row must have six fields");
        check(source_ids.insert(fields[0]).second, "source manifest ID duplicated");
        check(!fields[1].empty(), "source path missing");
        check(!fields[2].empty() && std::all_of(fields[2].begin(), fields[2].end(), [](unsigned char c) {
            return std::isdigit(c) != 0;
        }), "source byte count invalid");
        check(fields[3].size() == 64 && std::all_of(fields[3].begin(), fields[3].end(), [](unsigned char c) {
            return std::isxdigit(c) != 0;
        }), "source SHA-256 invalid");
        check(!fields[4].empty() && !fields[5].empty(), "source role or decision missing");
        ++rows;
    }
    check(rows >= 10, "source manifest unexpectedly small");
}

} // namespace

int main(int argc, char** argv) {
    try {
        check(argc == 3, "requirement registry and source manifest paths required");

        const auto requirements = genesis::RequirementRegistry::load(argv[1]);
        check(requirements.size() >= 30, "requirement registry unexpectedly small");
        check(requirements.validate().empty(), "requirement registry invalid");
        validate_source_manifest(argv[2]);

        genesis::ProvenanceLedger ledger;
        ledger.append(1, "e1", "o", "birth", "p1");
        ledger.append(2, "e2", "o", "experience", "p2");
        check(ledger.verify(), "ledger invalid");

        const auto id = identity();
        check(genesis::validate(id).empty(), "identity invalid");
        check(genesis::represents_continuation(genesis::OriginKind::restore), "restore semantics invalid");
        check(!genesis::represents_continuation(genesis::OriginKind::clone), "clone semantics invalid");

        const genesis::Genome genome{
            "genome", "1", {{"gene", "v", "d"}}, {{"gene", "zygote", "always", 0.5}}, id};
        check(genesis::validate(genome).empty(), "genome invalid");

        genesis::Expression expression{"rna", "gene", "cell", "genome", 0.5, 1.0};
        check(genesis::transition(expression, genesis::RnaState::expressing), "RNA express transition failed");
        check(genesis::transition(expression, genesis::RnaState::consumed), "RNA consume transition failed");
        check(genesis::transition(expression, genesis::RnaState::removed), "RNA remove transition failed");
        check(!genesis::transition(expression, genesis::RnaState::expressing), "removed RNA revived");

        genesis::MaturityVector maturity;
        maturity.scores.fill(95);
        check(genesis::independence_gate(maturity), "independence rejected");
        maturity.scores[static_cast<std::size_t>(genesis::Dimension::security)] = 50;
        check(!genesis::independence_gate(maturity), "unsafe independence accepted");

        std::cout << "All Genesis tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
