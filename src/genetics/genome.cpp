#include "genesis/genetics/genome.hpp"

#include <cmath>
#include <unordered_set>

namespace genesis {

std::vector<std::string> validate(const Genome& genome) {
    std::vector<std::string> errors;
    if (genome.genome_id.empty()) {
        errors.emplace_back("genome_id required");
    }
    if (genome.schema_version.empty()) {
        errors.emplace_back("schema_version required");
    }

    std::unordered_set<std::string> gene_ids;
    for (const auto& gene : genome.structural_strand) {
        if (gene.id.empty()) {
            errors.emplace_back("gene id required");
        } else if (!gene_ids.insert(gene.id).second) {
            errors.emplace_back("duplicate gene");
        }
        if (gene.variant.empty()) {
            errors.emplace_back("gene variant required");
        }
    }
    for (const auto& regulation : genome.regulatory_strand) {
        if (!gene_ids.contains(regulation.gene_id)) {
            errors.emplace_back("missing regulated gene");
        }
        if (!std::isfinite(regulation.activation_threshold)
            || regulation.activation_threshold < 0.0 || regulation.activation_threshold > 1.0) {
            errors.emplace_back("regulation threshold must be finite and in [0,1]");
        }
    }
    const auto lineage_errors = validate(genome.lineage_strand);
    errors.insert(errors.end(), lineage_errors.begin(), lineage_errors.end());
    return errors;
}

} // namespace genesis

