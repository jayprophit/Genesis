#pragma once

#include "genesis/genetics/genome.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace genesis::genetics {

enum class GenomeStoreErrorCode {
    none,
    invalid_genome,
    invalid_identifier,
    not_found,
    io_error,
    conflicting_version,
    corrupt_record,
};

struct GenomeStoreError final {
    GenomeStoreErrorCode code{GenomeStoreErrorCode::none};
    std::string message;
};

class GenomeStore final {
public:
    explicit GenomeStore(std::filesystem::path root);

    [[nodiscard]] bool write(const Genome& genome, GenomeStoreError* error = nullptr) const;
    [[nodiscard]] std::optional<Genome> read(std::string_view genome_id,
                                             std::string_view schema_version,
                                             GenomeStoreError* error = nullptr) const;
    [[nodiscard]] bool contains(std::string_view genome_id,
                                std::string_view schema_version) const;

    [[nodiscard]] static std::string serialize(const Genome& genome);
    [[nodiscard]] static std::optional<Genome> deserialize(std::string_view bytes,
                                                           GenomeStoreError* error = nullptr);
    [[nodiscard]] static std::string digest(const Genome& genome);
    // Digest of inheritable genome content with self-referential lineage
    // integrity fields removed. This is safe to place in the lineage strand.
    [[nodiscard]] static std::string content_digest(const Genome& genome);

private:
    [[nodiscard]] std::filesystem::path record_path(std::string_view genome_id,
                                                     std::string_view schema_version) const;
    std::filesystem::path root_;
};

} // namespace genesis::genetics
