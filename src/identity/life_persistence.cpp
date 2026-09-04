#include "genesis/identity/life_persistence.hpp"

#include "genesis/common/immutable_snapshot.hpp"
#include "genesis/runtime/runtime.hpp"

#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace genesis::identity {
namespace {

constexpr std::string_view kMagic = "GENESIS-LIFE-RECORD";
constexpr std::string_view kFileSuffix = "life";
constexpr std::uint64_t kSchemaVersion = 1U;
constexpr std::uint64_t kMaximumItems = 1'000'000U;
constexpr std::uint64_t kMaximumFieldBytes = 16U * 1024U * 1024U;
constexpr std::size_t kChecksumBytes = 64U;

void set_error(LifeRecordStoreError* error,
               LifeRecordStoreErrorCode code,
               std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->message = std::move(message);
    }
}

bool digest(std::string_view value) {
    return value.size() == 64U
           && std::all_of(value.begin(), value.end(), [](unsigned char character) {
                  return (character >= '0' && character <= '9')
                         || (character >= 'a' && character <= 'f');
              });
}

void map_file_error(const storage::ImmutableFileError& source,
                    bool writing,
                    LifeRecordStoreError* target) {
    switch (source.code) {
    case storage::ImmutableFileErrorCode::none:
        set_error(target, LifeRecordStoreErrorCode::none, {});
        return;
    case storage::ImmutableFileErrorCode::invalid_identifier:
        set_error(target, LifeRecordStoreErrorCode::invalid_identifier, source.message);
        return;
    case storage::ImmutableFileErrorCode::not_found:
        set_error(target, LifeRecordStoreErrorCode::not_found, source.message);
        return;
    case storage::ImmutableFileErrorCode::conflicting_version:
        set_error(target, LifeRecordStoreErrorCode::conflicting_version, source.message);
        return;
    case storage::ImmutableFileErrorCode::record_too_large:
        set_error(target,
                  writing ? LifeRecordStoreErrorCode::invalid_record
                          : LifeRecordStoreErrorCode::corrupt_record,
                  source.message);
        return;
    case storage::ImmutableFileErrorCode::unsafe_file_type:
        set_error(target,
                  writing ? LifeRecordStoreErrorCode::io_error
                          : LifeRecordStoreErrorCode::corrupt_record,
                  source.message);
        return;
    case storage::ImmutableFileErrorCode::io_error:
        set_error(target, LifeRecordStoreErrorCode::io_error, source.message);
        return;
    }
    set_error(target, LifeRecordStoreErrorCode::io_error, "unknown immutable-file error");
}

void append_u64(std::string& output, std::uint64_t value) {
    for (unsigned shift = 0U; shift < 64U; shift += 8U) {
        output.push_back(static_cast<char>((value >> shift) & 0xffU));
    }
}

std::uint64_t read_u64(std::string_view bytes, std::size_t& offset) {
    if (offset > bytes.size() || bytes.size() - offset < 8U) {
        throw std::runtime_error("truncated life record integer");
    }
    std::uint64_t value = 0U;
    for (unsigned shift = 0U; shift < 64U; shift += 8U) {
        value |= static_cast<std::uint64_t>(
                     static_cast<unsigned char>(bytes[offset++]))
                 << shift;
    }
    return value;
}

void append_string(std::string& output, std::string_view value) {
    append_u64(output, static_cast<std::uint64_t>(value.size()));
    output.append(value);
}

std::string read_string(std::string_view bytes, std::size_t& offset) {
    const auto length = read_u64(bytes, offset);
    const auto remaining = static_cast<std::uint64_t>(bytes.size() - offset);
    if (length > kMaximumFieldBytes || length > remaining
        || length > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("invalid life record field length");
    }
    const auto size = static_cast<std::size_t>(length);
    std::string value(bytes.substr(offset, size));
    offset += size;
    return value;
}

void append_optional_string(std::string& output,
                            const std::optional<std::string>& value) {
    append_u64(output, value.has_value() ? 1U : 0U);
    append_string(output, value.value_or(""));
}

std::optional<std::string> read_optional_string(std::string_view bytes,
                                                std::size_t& offset) {
    const auto present = read_u64(bytes, offset);
    auto value = read_string(bytes, offset);
    if (present > 1U || (present == 0U && !value.empty())) {
        throw std::runtime_error("invalid optional life record string");
    }
    return present == 1U ? std::optional<std::string>{std::move(value)} : std::nullopt;
}

void append_optional_time(std::string& output,
                          const std::optional<std::uint64_t>& value) {
    append_u64(output, value.has_value() ? 1U : 0U);
    append_u64(output, value.value_or(0U));
}

std::optional<std::uint64_t> read_optional_time(std::string_view bytes,
                                               std::size_t& offset) {
    const auto present = read_u64(bytes, offset);
    const auto value = read_u64(bytes, offset);
    if (present > 1U || (present == 0U && value != 0U)) {
        throw std::runtime_error("invalid optional life record time");
    }
    return present == 1U ? std::optional<std::uint64_t>{value} : std::nullopt;
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
    append_u64(output, std::bit_cast<std::uint64_t>(identity.birth_timestamp));
    append_u64(output, static_cast<std::uint64_t>(identity.ancestor_root_ids.size()));
    for (const auto& ancestor : identity.ancestor_root_ids) {
        append_string(output, ancestor);
    }
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
    identity.birth_timestamp = std::bit_cast<std::int64_t>(read_u64(bytes, offset));
    const auto ancestor_count = read_u64(bytes, offset);
    if (ancestor_count > kMaximumItems) {
        throw std::runtime_error("too many life-record lineage ancestors");
    }
    identity.ancestor_root_ids.reserve(static_cast<std::size_t>(ancestor_count));
    for (std::uint64_t index = 0U; index < ancestor_count; ++index) {
        identity.ancestor_root_ids.push_back(read_string(bytes, offset));
    }
    const auto origin = read_u64(bytes, offset);
    if (origin > static_cast<std::uint64_t>(OriginKind::child)) {
        throw std::runtime_error("invalid life-record lineage origin");
    }
    identity.origin = static_cast<OriginKind>(origin);
    return identity;
}

void append_entry(std::string& output, const LifeRecordEntry& entry) {
    append_string(output, entry.id);
    append_u64(output, static_cast<std::uint64_t>(entry.kind));
    append_u64(output, static_cast<std::uint64_t>(entry.evidence_class));
    append_u64(output, static_cast<std::uint64_t>(entry.visibility));
    append_u64(output, static_cast<std::uint64_t>(entry.disposition));
    append_string(output, entry.label);
    append_string(output, entry.value_digest);
    append_string(output, entry.evidence_digest);
    append_string(output, entry.authorization_evidence_digest);
    append_string(output, entry.source_entity_id);
    append_optional_string(output, entry.related_entity_id);
    append_string(output, entry.supersedes_entry_id);
    append_string(output, entry.continuity_event_id);
    append_string(output, entry.continuity_digest);
    append_u64(output, entry.sequence);
    append_u64(output, entry.effective_from);
    append_optional_time(output, entry.effective_until);
    append_u64(output, entry.recorded_at);
    append_string(output, entry.previous_entry_digest);
    append_string(output, entry.entry_digest);
}

LifeRecordEntry read_entry(std::string_view bytes, std::size_t& offset) {
    LifeRecordEntry entry;
    entry.id = read_string(bytes, offset);
    const auto kind = read_u64(bytes, offset);
    const auto evidence = read_u64(bytes, offset);
    const auto visibility = read_u64(bytes, offset);
    const auto disposition = read_u64(bytes, offset);
    if (kind > static_cast<std::uint64_t>(LifeRecordKind::custom)
        || evidence > static_cast<std::uint64_t>(LifeEvidenceClass::derived)
        || visibility > static_cast<std::uint64_t>(LifeVisibility::public_summary)
        || disposition > static_cast<std::uint64_t>(LifeEntryDisposition::retraction)) {
        throw std::runtime_error("invalid life record enum");
    }
    entry.kind = static_cast<LifeRecordKind>(kind);
    entry.evidence_class = static_cast<LifeEvidenceClass>(evidence);
    entry.visibility = static_cast<LifeVisibility>(visibility);
    entry.disposition = static_cast<LifeEntryDisposition>(disposition);
    entry.label = read_string(bytes, offset);
    entry.value_digest = read_string(bytes, offset);
    entry.evidence_digest = read_string(bytes, offset);
    entry.authorization_evidence_digest = read_string(bytes, offset);
    entry.source_entity_id = read_string(bytes, offset);
    entry.related_entity_id = read_optional_string(bytes, offset);
    entry.supersedes_entry_id = read_string(bytes, offset);
    entry.continuity_event_id = read_string(bytes, offset);
    entry.continuity_digest = read_string(bytes, offset);
    entry.sequence = read_u64(bytes, offset);
    entry.effective_from = read_u64(bytes, offset);
    entry.effective_until = read_optional_time(bytes, offset);
    entry.recorded_at = read_u64(bytes, offset);
    entry.previous_entry_digest = read_string(bytes, offset);
    entry.entry_digest = read_string(bytes, offset);
    return entry;
}

} // namespace

LifeRecordStore::LifeRecordStore(std::filesystem::path root,
                                 std::size_t maximum_record_bytes)
    : root_(std::move(root)), maximum_record_bytes_(maximum_record_bytes) {
    static_cast<void>(storage::ImmutableSnapshotFiles(root_, maximum_record_bytes_));
}

std::string LifeRecordStore::serialize(const DigitalLifeRecord& record) {
    if (!record.verify()) {
        throw std::invalid_argument("cannot serialize an invalid digital life record");
    }
    std::string output{kMagic};
    append_u64(output, kSchemaVersion);
    append_identity(output, record.identity_);
    append_string(output, record.lineage_anchor_digest_);
    append_string(output, record.entity_namespace_id_);
    append_string(output, record.organism_entity_id_);
    append_string(output, record.record_entity_id_);
    append_u64(output, static_cast<std::uint64_t>(record.entry_capacity_));
    append_u64(output, static_cast<std::uint64_t>(record.entries_.size()));
    for (const auto& entry : record.entries_) {
        append_entry(output, entry);
    }
    output += runtime::sha256(output);
    return output;
}

std::optional<DigitalLifeRecord> LifeRecordStore::deserialize(
    std::string_view bytes,
    LifeRecordStoreError* error) {
    try {
        if (bytes.size() < kMagic.size() + 8U + kChecksumBytes
            || bytes.substr(0U, kMagic.size()) != kMagic) {
            throw std::runtime_error("invalid digital life record magic");
        }
        const auto payload = bytes.substr(0U, bytes.size() - kChecksumBytes);
        if (runtime::sha256(payload) != bytes.substr(bytes.size() - kChecksumBytes)) {
            throw std::runtime_error("digital life record checksum mismatch");
        }
        std::size_t offset = kMagic.size();
        if (read_u64(payload, offset) != kSchemaVersion) {
            set_error(error,
                      LifeRecordStoreErrorCode::unsupported_schema,
                      "unsupported digital life record schema");
            return std::nullopt;
        }
        auto identity = read_identity(payload, offset);
        auto stored_anchor = read_string(payload, offset);
        auto entity_namespace = read_string(payload, offset);
        auto organism_entity = read_string(payload, offset);
        auto record_entity = read_string(payload, offset);
        const auto capacity = read_u64(payload, offset);
        if (capacity == 0U || capacity > kMaximumItems
            || capacity
                   > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            throw std::runtime_error("invalid digital life record capacity");
        }
        DigitalLifeRecord record(std::move(identity),
                                 std::move(entity_namespace),
                                 std::move(organism_entity),
                                 std::move(record_entity),
                                 static_cast<std::size_t>(capacity));
        if (record.lineage_anchor_digest_ != stored_anchor) {
            throw std::runtime_error("digital life record lineage anchor mismatch");
        }
        const auto entry_count = read_u64(payload, offset);
        if (entry_count > capacity) {
            throw std::runtime_error("digital life record entry count exceeds capacity");
        }
        record.entries_.reserve(static_cast<std::size_t>(entry_count));
        for (std::uint64_t index = 0U; index < entry_count; ++index) {
            record.entries_.push_back(read_entry(payload, offset));
        }
        if (offset != payload.size() || !record.rebuild_indexes() || !record.verify()) {
            throw std::runtime_error("trailing or inconsistent digital life record data");
        }
        set_error(error, LifeRecordStoreErrorCode::none, {});
        return record;
    } catch (const std::exception& exception) {
        set_error(error, LifeRecordStoreErrorCode::corrupt_record, exception.what());
        return std::nullopt;
    }
}

bool LifeRecordStore::write(const DigitalLifeRecord& record,
                            std::string_view version,
                            LifeRecordStoreError* error) const {
    try {
        const auto bytes = serialize(record);
        storage::ImmutableSnapshotFiles files(root_, maximum_record_bytes_);
        storage::ImmutableFileError file_error;
        if (!files.write(record.record_entity_id(), version, kFileSuffix, bytes, &file_error)) {
            map_file_error(file_error, true, error);
            return false;
        }
        set_error(error, LifeRecordStoreErrorCode::none, {});
        return true;
    } catch (const std::invalid_argument& exception) {
        set_error(error, LifeRecordStoreErrorCode::invalid_record, exception.what());
        return false;
    } catch (const std::exception& exception) {
        set_error(error, LifeRecordStoreErrorCode::io_error, exception.what());
        return false;
    }
}

std::optional<DigitalLifeRecord> LifeRecordStore::read(
    std::string_view record_entity_id,
    std::string_view organism_id,
    std::string_view lineage_anchor_digest,
    std::string_view version,
    LifeRecordStoreError* error) const {
    try {
        if (organism_id.empty() || organism_id.size() > 256U
            || !digest(lineage_anchor_digest)) {
            throw std::invalid_argument("invalid digital life record recovery identity");
        }
        storage::ImmutableSnapshotFiles files(root_, maximum_record_bytes_);
        storage::ImmutableFileError file_error;
        const auto bytes = files.read(record_entity_id, version, kFileSuffix, &file_error);
        if (!bytes.has_value()) {
            map_file_error(file_error, false, error);
            return std::nullopt;
        }
        auto record = deserialize(*bytes, error);
        if (record.has_value()
            && (record->record_entity_id() != record_entity_id
                || record->identity().organism_id != organism_id
                || record->lineage_anchor_digest() != lineage_anchor_digest)) {
            set_error(error,
                      LifeRecordStoreErrorCode::corrupt_record,
                      "digital life record identity, address or lineage binding mismatch");
            return std::nullopt;
        }
        return record;
    } catch (const std::invalid_argument& exception) {
        set_error(error, LifeRecordStoreErrorCode::invalid_identifier, exception.what());
        return std::nullopt;
    } catch (const std::exception& exception) {
        set_error(error, LifeRecordStoreErrorCode::io_error, exception.what());
        return std::nullopt;
    }
}

} // namespace genesis::identity
