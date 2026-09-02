#include "genesis/genesis.hpp"

#include <algorithm>
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

void validate_cycle_detection() {
    const auto path = std::filesystem::temp_directory_path() / "genesis_registry_cycle_test.tsv";
    constexpr const char* header = "id\tname\tpurpose\tparent\tdependencies\tinterfaces\timplementation_files\ttests\tbenchmarks\tscore_0_100\tstatus\tversion\tevidence\tprovenance\tlast_verified\taliases\n";
    {
        std::ofstream output(path, std::ios::trunc);
        check(output.good(), "cannot create registry cycle fixture");
        output << header;
        output << "A\tA\tcycle fixture\tDOMAIN-ENGINEERING\tB\t-\t-\t-\t-\t0\tSPECIFIED\t1.0.0\t-\ttest\t2026-09-02\t-\n";
        output << "B\tB\tcycle fixture\tDOMAIN-ENGINEERING\tA\t-\t-\t-\t-\t0\tSPECIFIED\t1.0.0\t-\ttest\t2026-09-02\t-\n";
    }
    const auto fixture = genesis::RequirementRegistry::load(path);
    const auto errors = fixture.validate();
    check(std::any_of(errors.begin(), errors.end(), [](const std::string& error) {
        return error.find("dependency cycle") != std::string::npos;
    }), "dependency cycle was not detected");
    std::filesystem::remove(path);

    const auto duplicate_path = std::filesystem::temp_directory_path() / "genesis_registry_duplicate_test.tsv";
    {
        std::ofstream output(duplicate_path, std::ios::trunc);
        check(output.good(), "cannot create registry duplicate fixture");
        output << header;
        output << "A\tA\tduplicate fixture\tDOMAIN-ENGINEERING\t-\t-\t-\t-\t-\t0\tSPECIFIED\t1.0.0\t-\ttest\t2026-09-02\t-\n";
        output << "A\tA-again\tduplicate fixture\tDOMAIN-ENGINEERING\t-\t-\t-\t-\t-\t0\tSPECIFIED\t1.0.0\t-\ttest\t2026-09-02\t-\n";
    }
    bool duplicate_rejected = false;
    try {
        static_cast<void>(genesis::RequirementRegistry::load(duplicate_path));
    } catch (const std::runtime_error&) {
        duplicate_rejected = true;
    }
    check(duplicate_rejected, "duplicate requirement ID was not rejected");
    std::filesystem::remove(duplicate_path);
}

} // namespace

int main(int argc, char** argv) {
    try {
        check(argc == 5, "requirement registry, source manifest, domain registry and canonical source paths required");

        const auto requirements = genesis::RequirementRegistry::load(argv[1]);
        check(requirements.size() >= 40, "requirement registry unexpectedly small");
        for (const auto* legacy_id : {"REQ-REG-001", "REQ-PROV-001", "REQ-RUN-001", "REQ-PHY-001",
                                      "REQ-CHEM-001", "REQ-ID-001", "REQ-LIN-001", "REQ-GEN-001",
                                      "REQ-HELIX-001", "REQ-HELIX-002", "REQ-HELIX-003", "REQ-RNA-001",
                                      "REQ-REPRO-001", "REQ-BIRTH-001", "REQ-INH-001", "REQ-CELL-001",
                                      "REQ-TISSUE-001", "REQ-HOME-001", "REQ-IMM-001", "REQ-COG-001",
                                      "REQ-DEV-001", "REQ-PAR-001", "REQ-MODAL-001", "REQ-CRYPTO-001",
                                      "REQ-SRC-001", "REQ-CORPUS-001", "REQ-EVAL-001", "REQ-EVT-001",
                                      "REQ-RES-001", "REQ-SVC-001", "REQ-TEL-001", "REQ-MEM-001",
                                      "REQ-POL-001", "REQ-ADAPT-001"}) {
            check(requirements.find(legacy_id) != nullptr, "legacy requirement ID was dropped");
        }
        const auto domains = genesis::DomainRegistry::load(argv[3]);
        check(domains.size() >= 10, "domain registry unexpectedly small");
        check(domains.validate().empty(), "domain registry invalid");
        check(requirements.validate(&domains).empty(), "requirement registry invalid");
        const auto canonical = genesis::RequirementRegistry::load(argv[4]);
        check(canonical.size() == 1451, "canonical source section count changed");
        check(canonical.validate(&domains).empty(), "canonical source section registry invalid");
        validate_source_manifest(argv[2]);
        validate_cycle_detection();

        genesis::ProvenanceLedger ledger;
        static_cast<void>(ledger.append(1, "e1", "o", "birth", "p1"));
        static_cast<void>(ledger.append(2, "e2", "o", "experience", "p2"));
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
