#include "genesis/genetics/reproduction.hpp"

#include "genesis/genetics/persistence.hpp"
#include "genesis/runtime/runtime.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <tuple>
#include <utility>

namespace genesis::genetics {
namespace {

void clear_error(BirthError* error) {
    if (error != nullptr) {
        *error = {};
    }
}

void set_error(BirthError* error, BirthErrorCode code, std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->message = std::move(message);
    }
}

std::string join_errors(const std::vector<std::string>& errors) {
    std::string result;
    for (const auto& error : errors) {
        if (!result.empty()) {
            result += "; ";
        }
        result += error;
    }
    return result;
}

void append_field(std::string& target, std::string_view value) {
    target.append(std::to_string(value.size()));
    target.push_back(':');
    target.append(value);
}

std::string parent_digest(const ParentPackage& parent) {
    return GenomeStore::content_digest(parent.genome);
}

bool identities_match(const LineageIdentity& left, const LineageIdentity& right) {
    return left.organism_id == right.organism_id
        && left.genesis_id == right.genesis_id
        && left.lineage_id == right.lineage_id
        && left.birth_event_id == right.birth_event_id
        && left.parent_a_id == right.parent_a_id
        && left.parent_b_id == right.parent_b_id
        && left.genome_hash == right.genome_hash
        && left.inherited_state_hash == right.inherited_state_hash
        && left.birth_snapshot_hash == right.birth_snapshot_hash
        && left.identity_seed == right.identity_seed
        && left.lineage_signature == right.lineage_signature
        && left.cryptographic_provenance == right.cryptographic_provenance
        && left.generation == right.generation
        && left.birth_timestamp == right.birth_timestamp
        && left.ancestor_root_ids == right.ancestor_root_ids
        && left.origin == right.origin;
}

std::vector<std::string> validate_parent(const ParentPackage& parent,
                                         std::string_view label,
                                         std::int64_t birth_timestamp) {
    std::vector<std::string> errors;
    const auto identity_errors = genesis::validate(parent.identity);
    for (const auto& error : identity_errors) {
        errors.push_back(std::string(label) + " identity: " + error);
    }
    const auto genome_errors = genesis::validate(parent.genome);
    for (const auto& error : genome_errors) {
        errors.push_back(std::string(label) + " genome: " + error);
    }
    if (parent.identity.organism_id != parent.genome.lineage_strand.organism_id) {
        errors.push_back(std::string(label) + " identity and genome organism IDs differ");
    }
    if (!identities_match(parent.identity, parent.genome.lineage_strand)) {
        errors.push_back(std::string(label) + " identity and genome lineage strands differ");
    }
    const auto expected_hash = parent_digest(parent);
    if (!parent.identity.genome_hash.empty() && parent.identity.genome_hash != expected_hash) {
        errors.push_back(std::string(label) + " genome hash does not match content");
    }
    for (const auto& memory : parent.inherited_memory) {
        const auto memory_errors = memory::validate(memory);
        for (const auto& error : memory_errors) {
            errors.push_back(std::string(label) + " memory: " + error);
        }
        if (memory.source_timestamp > birth_timestamp) {
            errors.push_back(std::string(label) + " memory is newer than the birth cutoff");
        }
    }
    return errors;
}

std::vector<memory::InheritedMemory> copy_inherited_memory(
    const ParentPackage& parent,
    memory::Origin origin,
    std::int64_t birth_timestamp) {
    std::vector<memory::InheritedMemory> result;
    result.reserve(parent.inherited_memory.size());
    for (const auto& source : parent.inherited_memory) {
        auto copy = source;
        copy.origin = origin;
        copy.source_parent = parent.identity.organism_id;
        copy.inheritance_timestamp = birth_timestamp;
        copy.transformation_history.push_back("birth_cutoff");
        result.push_back(std::move(copy));
    }
    return result;
}

std::string inherited_digest(const std::vector<memory::InheritedMemory>& memories) {
    std::vector<std::string> digests;
    digests.reserve(memories.size());
    for (const auto& memory : memories) {
        digests.push_back(memory::digest(memory));
    }
    std::sort(digests.begin(), digests.end());
    std::string material{"genesis.inherited-state.v1"};
    for (const auto& digest : digests) {
        append_field(material, digest);
    }
    return runtime::sha256(material);
}

} // namespace

std::optional<BirthResult> BirthTransaction::execute(const BirthRequest& request,
                                                     BirthError* error) {
    clear_error(error);
    if (request.child_organism_id.empty() || request.birth_event_id.empty()) {
        set_error(error, BirthErrorCode::invalid_request,
                  "child_organism_id and birth_event_id are required");
        return std::nullopt;
    }
    if (request.parent_a.identity.organism_id == request.parent_b.identity.organism_id) {
        set_error(error, BirthErrorCode::duplicate_parent,
                  "two-parent birth requires distinct parent identities");
        return std::nullopt;
    }
    if (request.child_organism_id == request.parent_a.identity.organism_id
        || request.child_organism_id == request.parent_b.identity.organism_id) {
        set_error(error, BirthErrorCode::duplicate_parent,
                  "child identity must be distinct from both parents");
        return std::nullopt;
    }
    if (!std::isfinite(request.mutation_probability)
        || request.mutation_probability < 0.0 || request.mutation_probability > 1.0) {
        set_error(error, BirthErrorCode::invalid_probability,
                  "mutation_probability must be finite and in [0,1]");
        return std::nullopt;
    }

    const auto parent_a_errors = validate_parent(request.parent_a, "parent_a", request.birth_timestamp);
    const auto parent_b_errors = validate_parent(request.parent_b, "parent_b", request.birth_timestamp);
    if (!parent_a_errors.empty() || !parent_b_errors.empty()) {
        std::vector<std::string> errors = parent_a_errors;
        errors.insert(errors.end(), parent_b_errors.begin(), parent_b_errors.end());
        set_error(error, BirthErrorCode::invalid_parent, join_errors(errors));
        return std::nullopt;
    }
    if (request.parent_a.identity.generation == std::numeric_limits<std::uint64_t>::max()
        || request.parent_b.identity.generation == std::numeric_limits<std::uint64_t>::max()) {
        set_error(error, BirthErrorCode::invalid_parent, "parent generation cannot advance");
        return std::nullopt;
    }

    std::mt19937_64 random(request.seed);
    std::map<std::string, std::pair<std::optional<Gene>, std::optional<Gene>>, std::less<>> genes;
    for (const auto& gene : request.parent_a.genome.structural_strand) {
        genes[gene.id].first = gene;
    }
    for (const auto& gene : request.parent_b.genome.structural_strand) {
        genes[gene.id].second = gene;
    }

    Genome child_genome;
    child_genome.genome_id = request.child_organism_id + ".genome";
    child_genome.schema_version = "1.0.0";
    std::vector<MutationRecord> mutations;
    std::size_t mutation_index = 0;
    for (const auto& [gene_id, candidates] : genes) {
        Gene selected;
        if (candidates.first && candidates.second) {
            selected = (random() & 1U) == 0U ? *candidates.first : *candidates.second;
        } else {
            selected = candidates.first ? *candidates.first : *candidates.second;
        }
        if (std::bernoulli_distribution(request.mutation_probability)(random)) {
            const auto original_variant = selected.variant;
            const auto mutation_material = request.child_organism_id + "|" + gene_id + "|"
                + std::to_string(request.seed) + "|" + std::to_string(mutation_index);
            const auto mutation_digest = runtime::sha256(mutation_material);
            selected.variant += "|mut-" + mutation_digest.substr(0, 16);
            selected.payload_digest = runtime::sha256(original_variant + "|" + selected.variant);
            mutations.push_back({
                "mutation-" + mutation_digest.substr(0, 24), gene_id, original_variant,
                selected.variant, "seeded_mutation", request.mutation_probability,
                std::max(request.parent_a.identity.generation,
                         request.parent_b.identity.generation) + 1,
                "PENDING", "PENDING", "PENDING",
            });
        }
        child_genome.structural_strand.push_back(std::move(selected));
        ++mutation_index;
    }

    using RegulationKey = std::tuple<std::string, std::string, std::string>;
    std::map<RegulationKey,
             std::pair<std::optional<Regulation>, std::optional<Regulation>>>
        regulations;
    for (const auto& regulation : request.parent_a.genome.regulatory_strand) {
        regulations[{regulation.gene_id, regulation.stage, regulation.condition}].first = regulation;
    }
    for (const auto& regulation : request.parent_b.genome.regulatory_strand) {
        regulations[{regulation.gene_id, regulation.stage, regulation.condition}].second = regulation;
    }
    for (const auto& [key, candidates] : regulations) {
        const auto regulation = candidates.first && candidates.second
            ? ((random() & 1U) == 0U ? *candidates.first : *candidates.second)
            : (candidates.first ? *candidates.first : *candidates.second);
        if (std::find_if(child_genome.structural_strand.begin(), child_genome.structural_strand.end(),
                         [&](const Gene& gene) { return gene.id == regulation.gene_id; })
            != child_genome.structural_strand.end()) {
            child_genome.regulatory_strand.push_back(regulation);
        }
    }

    LineageIdentity child_identity;
    child_identity.organism_id = request.child_organism_id;
    child_identity.genesis_id = request.parent_a.identity.genesis_id == request.parent_b.identity.genesis_id
        ? request.parent_a.identity.genesis_id
        : "genesis-" + runtime::sha256(request.parent_a.identity.genesis_id + "|"
                                        + request.parent_b.identity.genesis_id).substr(0, 24);
    child_identity.birth_event_id = request.birth_event_id;
    child_identity.birth_timestamp = request.birth_timestamp;
    child_identity.parent_a_id = request.parent_a.identity.organism_id;
    child_identity.parent_b_id = request.parent_b.identity.organism_id;
    child_identity.generation = std::max(request.parent_a.identity.generation,
                                         request.parent_b.identity.generation) + 1;
    child_identity.identity_seed = runtime::sha256("identity|" + request.child_organism_id + "|"
                                                   + std::to_string(request.seed));
    child_identity.lineage_id = "lineage-" + runtime::sha256(
        request.parent_a.identity.organism_id + "|" + request.parent_b.identity.organism_id + "|"
        + request.birth_event_id + "|" + std::to_string(request.seed)).substr(0, 32);
    child_identity.origin = OriginKind::child;
    child_identity.lineage_signature = "PENDING_CRYPTOGRAPHIC_PROVIDER";
    child_identity.cryptographic_provenance = "UNAVAILABLE:provider-required";
    std::set<std::string> roots;
    roots.insert(request.parent_a.identity.ancestor_root_ids.begin(),
                 request.parent_a.identity.ancestor_root_ids.end());
    roots.insert(request.parent_b.identity.ancestor_root_ids.begin(),
                 request.parent_b.identity.ancestor_root_ids.end());
    if (roots.empty()) {
        roots.insert(request.parent_a.identity.genesis_id);
        roots.insert(request.parent_b.identity.genesis_id);
    }
    child_identity.ancestor_root_ids.assign(roots.begin(), roots.end());

    std::vector<memory::InheritedMemory> inherited =
        copy_inherited_memory(request.parent_a, memory::Origin::inherited_parent_a,
                              request.birth_timestamp);
    auto parent_b_memory = copy_inherited_memory(request.parent_b,
                                                 memory::Origin::inherited_parent_b,
                                                 request.birth_timestamp);
    inherited.insert(inherited.end(), parent_b_memory.begin(), parent_b_memory.end());
    std::sort(inherited.begin(), inherited.end(), [](const auto& left, const auto& right) {
        return std::tie(left.source_parent, left.memory_id, left.source_digest)
            < std::tie(right.source_parent, right.memory_id, right.source_digest);
    });

    child_genome.lineage_strand = child_identity;
    child_identity.genome_hash = GenomeStore::content_digest(child_genome);
    child_identity.inherited_state_hash = inherited_digest(inherited);
    child_genome.lineage_strand.genome_hash = child_identity.genome_hash;
    child_genome.lineage_strand.inherited_state_hash = child_identity.inherited_state_hash;
    const auto snapshot_material = request.child_organism_id + "|" + request.birth_event_id + "|"
        + child_identity.genome_hash + "|" + child_identity.inherited_state_hash + "|"
        + std::to_string(request.birth_timestamp);
    child_identity.birth_snapshot_hash = runtime::sha256("birth-snapshot|" + snapshot_material);
    child_genome.lineage_strand.birth_snapshot_hash = child_identity.birth_snapshot_hash;

    const auto transaction_material = request.parent_a.identity.organism_id + "|"
        + request.parent_b.identity.organism_id + "|" + request.child_organism_id + "|"
        + request.birth_event_id + "|" + std::to_string(request.seed);
    BirthResult result;
    result.genome = std::move(child_genome);
    result.identity = child_identity;
    result.inherited_memory = std::move(inherited);
    result.mutations = std::move(mutations);
    result.birth_snapshot_hash = child_identity.birth_snapshot_hash;
    result.transaction_id = "birth-" + runtime::sha256(transaction_material).substr(0, 32);
    clear_error(error);
    return result;
}

} // namespace genesis::genetics
