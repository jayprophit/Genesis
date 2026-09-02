#include "genesis/genetics/persistence.hpp"

#include "genesis/runtime/runtime.hpp"

#include <bit>
#include <cctype>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace genesis::genetics {
namespace {

constexpr std::string_view magic = "GENESIS-GENOME-V1";
constexpr std::uint64_t maximum_field_size = 1024U * 1024U;
constexpr std::uint64_t maximum_collection_size = 100000U;

void set_error(GenomeStoreError* error, GenomeStoreErrorCode code, std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->message = std::move(message);
    }
}

std::string validation_message(const std::vector<std::string>& errors) {
    std::string message{"genome validation failed"};
    for (const auto& error : errors) {
        message += "; " + error;
    }
    return message;
}

void validate_identifier(std::string_view value, std::string_view field) {
    if (value.empty() || value == "." || value == ".." || value.size() > 128U) {
        throw std::invalid_argument(std::string(field) + " is not a safe identifier");
    }
    for (const unsigned char character : value) {
        if (!(std::isalnum(character) != 0 || character == '_' || character == '-' || character == '.')) {
            throw std::invalid_argument(std::string(field) + " contains an unsafe character");
        }
    }
}

void append_u64(std::string& output, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64U; shift += 8U) {
        output.push_back(static_cast<char>((value >> shift) & 0xffU));
    }
}

std::uint64_t read_u64(std::string_view bytes, std::size_t& offset) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint64_t)) {
        throw std::runtime_error("genome record ended while reading an integer");
    }
    std::uint64_t value = 0;
    for (unsigned shift = 0; shift < 64U; shift += 8U) {
        value |= static_cast<std::uint64_t>(static_cast<unsigned char>(bytes[offset++])) << shift;
    }
    return value;
}

void append_string(std::string& output, std::string_view value) {
    append_u64(output, value.size());
    output.append(value);
}

std::string read_string(std::string_view bytes, std::size_t& offset) {
    const auto size = read_u64(bytes, offset);
    if (size > maximum_field_size || size > bytes.size() - offset) {
        throw std::runtime_error("genome record contains an invalid string length");
    }
    const std::string value{bytes.substr(offset, static_cast<std::size_t>(size))};
    offset += static_cast<std::size_t>(size);
    return value;
}

void append_string_vector(std::string& output, const std::vector<std::string>& values) {
    if (values.size() > maximum_collection_size) {
        throw std::invalid_argument("genome collection is too large");
    }
    append_u64(output, values.size());
    for (const auto& value : values) {
        append_string(output, value);
    }
}

std::vector<std::string> read_string_vector(std::string_view bytes, std::size_t& offset) {
    const auto count = read_u64(bytes, offset);
    if (count > maximum_collection_size) {
        throw std::runtime_error("genome collection count is too large");
    }
    std::vector<std::string> values;
    values.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t index = 0; index < count; ++index) {
        values.push_back(read_string(bytes, offset));
    }
    return values;
}

void append_identity(std::string& output, const LineageIdentity& identity) {
    append_string(output, identity.organism_id);
    append_string(output, identity.genesis_id);
    append_string(output, identity.lineage_id);
    append_string(output, identity.birth_event_id);
    append_string(output, identity.parent_a_id);
    append_string(output, identity.parent_b_id);
    append_string(output, identity.genome_hash);
    append_string(output, identity.inherited_state_hash);
    append_string(output, identity.birth_snapshot_hash);
    append_string(output, identity.identity_seed);
    append_string(output, identity.lineage_signature);
    append_string(output, identity.cryptographic_provenance);
    append_u64(output, identity.generation);
    append_u64(output, static_cast<std::uint64_t>(identity.birth_timestamp));
    append_string_vector(output, identity.ancestor_root_ids);
    append_u64(output, static_cast<std::uint64_t>(identity.origin));
}

LineageIdentity read_identity(std::string_view bytes, std::size_t& offset) {
    LineageIdentity identity;
    identity.organism_id = read_string(bytes, offset);
    identity.genesis_id = read_string(bytes, offset);
    identity.lineage_id = read_string(bytes, offset);
    identity.birth_event_id = read_string(bytes, offset);
    identity.parent_a_id = read_string(bytes, offset);
    identity.parent_b_id = read_string(bytes, offset);
    identity.genome_hash = read_string(bytes, offset);
    identity.inherited_state_hash = read_string(bytes, offset);
    identity.birth_snapshot_hash = read_string(bytes, offset);
    identity.identity_seed = read_string(bytes, offset);
    identity.lineage_signature = read_string(bytes, offset);
    identity.cryptographic_provenance = read_string(bytes, offset);
    identity.generation = read_u64(bytes, offset);
    identity.birth_timestamp = static_cast<std::int64_t>(read_u64(bytes, offset));
    identity.ancestor_root_ids = read_string_vector(bytes, offset);
    const auto origin = read_u64(bytes, offset);
    if (origin > static_cast<std::uint64_t>(OriginKind::child)) {
        throw std::runtime_error("genome record contains an invalid origin kind");
    }
    identity.origin = static_cast<OriginKind>(origin);
    return identity;
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open genome record");
    }
    std::string bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (input.bad()) {
        throw std::runtime_error("cannot read genome record");
    }
    return bytes;
}

} // namespace

GenomeStore::GenomeStore(std::filesystem::path root)
    : root_(std::move(root)) {
    if (root_.empty()) {
        throw std::invalid_argument("genome store root cannot be empty");
    }
}

std::string GenomeStore::serialize(const Genome& genome) {
    const auto errors = genesis::validate(genome);
    if (!errors.empty()) {
        throw std::invalid_argument(validation_message(errors));
    }

    std::string output{magic};
    append_string(output, genome.genome_id);
    append_string(output, genome.schema_version);
    if (genome.structural_strand.size() > maximum_collection_size
        || genome.regulatory_strand.size() > maximum_collection_size) {
        throw std::invalid_argument("genome strand is too large");
    }
    append_u64(output, genome.structural_strand.size());
    for (const auto& gene : genome.structural_strand) {
        append_string(output, gene.id);
        append_string(output, gene.variant);
        append_string(output, gene.payload_digest);
    }
    append_u64(output, genome.regulatory_strand.size());
    for (const auto& regulation : genome.regulatory_strand) {
        append_string(output, regulation.gene_id);
        append_string(output, regulation.stage);
        append_string(output, regulation.condition);
        append_u64(output, std::bit_cast<std::uint64_t>(regulation.activation_threshold));
    }
    append_identity(output, genome.lineage_strand);
    return output;
}

std::optional<Genome> GenomeStore::deserialize(std::string_view bytes, GenomeStoreError* error) {
    try {
        if (bytes.size() < magic.size() || bytes.substr(0, magic.size()) != magic) {
            throw std::runtime_error("genome record magic is invalid");
        }
        std::size_t offset = magic.size();
        Genome genome;
        genome.genome_id = read_string(bytes, offset);
        genome.schema_version = read_string(bytes, offset);
        const auto structural_count = read_u64(bytes, offset);
        if (structural_count > maximum_collection_size) {
            throw std::runtime_error("structural strand is too large");
        }
        genome.structural_strand.reserve(static_cast<std::size_t>(structural_count));
        for (std::uint64_t index = 0; index < structural_count; ++index) {
            genome.structural_strand.push_back(
                {read_string(bytes, offset), read_string(bytes, offset), read_string(bytes, offset)});
        }
        const auto regulatory_count = read_u64(bytes, offset);
        if (regulatory_count > maximum_collection_size) {
            throw std::runtime_error("regulatory strand is too large");
        }
        genome.regulatory_strand.reserve(static_cast<std::size_t>(regulatory_count));
        for (std::uint64_t index = 0; index < regulatory_count; ++index) {
            Regulation regulation;
            regulation.gene_id = read_string(bytes, offset);
            regulation.stage = read_string(bytes, offset);
            regulation.condition = read_string(bytes, offset);
            regulation.activation_threshold =
                std::bit_cast<double>(read_u64(bytes, offset));
            genome.regulatory_strand.push_back(std::move(regulation));
        }
        genome.lineage_strand = read_identity(bytes, offset);
        if (offset != bytes.size()) {
            throw std::runtime_error("genome record contains trailing bytes");
        }
        const auto errors = genesis::validate(genome);
        if (!errors.empty()) {
            throw std::runtime_error(validation_message(errors));
        }
        set_error(error, GenomeStoreErrorCode::none, {});
        return genome;
    } catch (const std::exception& exception) {
        set_error(error, GenomeStoreErrorCode::corrupt_record, exception.what());
        return std::nullopt;
    }
}

std::string GenomeStore::digest(const Genome& genome) {
    return runtime::sha256(serialize(genome));
}

std::string GenomeStore::content_digest(const Genome& genome) {
    Genome canonical = genome;
    // The lineage strand stores integrity values that depend on the genome
    // itself. Clear those self-referential fields before hashing the content
    // so the resulting digest is stable and can be embedded in the strand.
    canonical.lineage_strand.genome_hash.clear();
    canonical.lineage_strand.inherited_state_hash.clear();
    canonical.lineage_strand.birth_snapshot_hash.clear();
    canonical.lineage_strand.lineage_signature.clear();
    canonical.lineage_strand.cryptographic_provenance.clear();
    return runtime::sha256(serialize(canonical));
}

std::filesystem::path GenomeStore::record_path(std::string_view genome_id,
                                               std::string_view schema_version) const {
    validate_identifier(genome_id, "genome_id");
    validate_identifier(schema_version, "schema_version");
    return root_ / (std::string(genome_id) + "." + std::string(schema_version) + ".genome");
}

bool GenomeStore::write(const Genome& genome, GenomeStoreError* error) const {
    set_error(error, GenomeStoreErrorCode::none, {});
    std::string bytes;
    try {
        bytes = serialize(genome);
        const auto target = record_path(genome.genome_id, genome.schema_version);
        std::error_code filesystem_error;
        std::filesystem::create_directories(root_, filesystem_error);
        if (filesystem_error) {
            set_error(error, GenomeStoreErrorCode::io_error,
                      "cannot create genome store root: " + filesystem_error.message());
            return false;
        }

        if (std::filesystem::exists(target, filesystem_error)) {
            if (filesystem_error) {
                set_error(error, GenomeStoreErrorCode::io_error,
                          "cannot inspect existing genome record: " + filesystem_error.message());
                return false;
            }
            const auto existing = read_file(target);
            if (existing == bytes) {
                return true;
            }
            set_error(error,
                      GenomeStoreErrorCode::conflicting_version,
                      "an immutable genome version already exists with different content");
            return false;
        }

        const auto temporary = target.string() + ".tmp-" + runtime::sha256(bytes);
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output) {
                set_error(error, GenomeStoreErrorCode::io_error,
                          "cannot create temporary genome record");
                return false;
            }
            output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            output.flush();
            if (!output) {
                set_error(error, GenomeStoreErrorCode::io_error,
                          "cannot flush temporary genome record");
                return false;
            }
        }

        std::filesystem::rename(temporary, target, filesystem_error);
        if (filesystem_error) {
            std::error_code cleanup_error;
            std::filesystem::remove(temporary, cleanup_error);
            if (std::filesystem::exists(target, cleanup_error)) {
                const auto existing = read_file(target);
                if (existing == bytes) {
                    return true;
                }
                set_error(error,
                          GenomeStoreErrorCode::conflicting_version,
                          "an immutable genome version was created concurrently");
                return false;
            }
            set_error(error, GenomeStoreErrorCode::io_error,
                      "cannot commit genome record: " + filesystem_error.message());
            return false;
        }
        return true;
    } catch (const std::invalid_argument& exception) {
        set_error(error, GenomeStoreErrorCode::invalid_genome, exception.what());
        return false;
    } catch (const std::exception& exception) {
        set_error(error, GenomeStoreErrorCode::io_error, exception.what());
        return false;
    }
}

std::optional<Genome> GenomeStore::read(std::string_view genome_id,
                                        std::string_view schema_version,
                                        GenomeStoreError* error) const {
    set_error(error, GenomeStoreErrorCode::none, {});
    try {
        const auto target = record_path(genome_id, schema_version);
        std::error_code filesystem_error;
        if (!std::filesystem::exists(target, filesystem_error) || filesystem_error) {
            set_error(error, GenomeStoreErrorCode::not_found, "genome version was not found");
            return std::nullopt;
        }
        const auto bytes = read_file(target);
        auto genome = deserialize(bytes, error);
        if (!genome) {
            return std::nullopt;
        }
        if (genome->genome_id != genome_id || genome->schema_version != schema_version
            || digest(*genome) != runtime::sha256(bytes)) {
            set_error(error, GenomeStoreErrorCode::corrupt_record,
                      "genome record identity or digest does not match its contents");
            return std::nullopt;
        }
        return genome;
    } catch (const std::invalid_argument& exception) {
        set_error(error, GenomeStoreErrorCode::invalid_identifier, exception.what());
        return std::nullopt;
    } catch (const std::exception& exception) {
        set_error(error, GenomeStoreErrorCode::io_error, exception.what());
        return std::nullopt;
    }
}

bool GenomeStore::contains(std::string_view genome_id, std::string_view schema_version) const {
    try {
        std::error_code error;
        return std::filesystem::is_regular_file(record_path(genome_id, schema_version), error)
            && !error;
    } catch (...) {
        return false;
    }
}

} // namespace genesis::genetics
