#pragma once

#include "genesis/identity/lineage.hpp"

#include <string>
#include <vector>

namespace genesis {

struct Gene final {
    std::string id;
    std::string variant;
    std::string payload_digest;
};

struct Regulation final {
    std::string gene_id;
    std::string stage;
    std::string condition;
    double activation_threshold{};
};

struct Genome final {
    std::string genome_id;
    std::string schema_version;
    std::vector<Gene> structural_strand;
    std::vector<Regulation> regulatory_strand;
    LineageIdentity lineage_strand;
};

[[nodiscard]] std::vector<std::string> validate(const Genome& genome);

} // namespace genesis

