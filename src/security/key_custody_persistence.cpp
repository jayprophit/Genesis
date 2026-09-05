#include "genesis/security/key_custody_persistence.hpp"

#include "genesis/common/immutable_snapshot.hpp"
#include "genesis/runtime/runtime.hpp"

#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace genesis::security {
namespace {

constexpr std::string_view kMagic = "GENESIS-KEY-CUSTODY-REGISTRY";
constexpr std::string_view kFileSuffix = "key-custody";
constexpr std::uint64_t kSchemaVersion = 1U;
constexpr std::uint64_t kMaximumItems = 1'000'000U;
constexpr std::uint64_t kMaximumUsages = 10U;
constexpr std::uint64_t kMaximumOperators = 64U;
constexpr std::uint64_t kMaximumFieldBytes = 16U * 1024U * 1024U;
constexpr std::size_t kChecksumBytes = 64U;

void set_error(KeyCustodyStoreError* error,
               KeyCustodyStoreErrorCode code,
               std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->message = std::move(message);
    }
}

void map_file_error(const storage::ImmutableFileError& source,
                    bool writing,
                    KeyCustodyStoreError* target) {
    switch (source.code) {
    case storage::ImmutableFileErrorCode::none:
        set_error(target, KeyCustodyStoreErrorCode::none, {});
        return;
    case storage::ImmutableFileErrorCode::invalid_identifier:
        set_error(target, KeyCustodyStoreErrorCode::invalid_identifier, source.message);
        return;
    case storage::ImmutableFileErrorCode::not_found:
        set_error(target, KeyCustodyStoreErrorCode::not_found, source.message);
        return;
    case storage::ImmutableFileErrorCode::conflicting_version:
        set_error(target, KeyCustodyStoreErrorCode::conflicting_version, source.message);
        return;
    case storage::ImmutableFileErrorCode::record_too_large:
        set_error(target,
                  writing ? KeyCustodyStoreErrorCode::invalid_registry
                          : KeyCustodyStoreErrorCode::corrupt_record,
                  source.message);
        return;
    case storage::ImmutableFileErrorCode::unsafe_file_type:
        set_error(target,
                  writing ? KeyCustodyStoreErrorCode::io_error
                          : KeyCustodyStoreErrorCode::corrupt_record,
                  source.message);
        return;
    case storage::ImmutableFileErrorCode::io_error:
        set_error(target, KeyCustodyStoreErrorCode::io_error, source.message);
        return;
    }
    set_error(target, KeyCustodyStoreErrorCode::io_error, "unknown immutable-file error");
}

void append_u64(std::string& output, std::uint64_t value) {
    for (unsigned shift = 0U; shift < 64U; shift += 8U) {
        output.push_back(static_cast<char>((value >> shift) & 0xffU));
    }
}

std::uint64_t read_u64(std::string_view bytes, std::size_t& offset) {
    if (offset > bytes.size() || bytes.size() - offset < 8U) {
        throw std::runtime_error("truncated key-custody integer");
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
        || length
               > static_cast<std::uint64_t>(
                   std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("invalid key-custody field length");
    }
    const auto size = static_cast<std::size_t>(length);
    std::string value(bytes.substr(offset, size));
    offset += size;
    return value;
}

std::uint64_t read_count(std::string_view bytes,
                         std::size_t& offset,
                         std::uint64_t maximum,
                         std::string_view label) {
    const auto count = read_u64(bytes, offset);
    if (count > maximum
        || count
               > static_cast<std::uint64_t>(
                   std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("invalid " + std::string(label) + " count");
    }
    return count;
}

template <typename Enum>
void append_enum(std::string& output, Enum value) {
    append_u64(output, static_cast<std::uint64_t>(value));
}

template <typename Enum>
Enum read_enum(std::string_view bytes,
               std::size_t& offset,
               Enum maximum,
               std::string_view label) {
    const auto value = read_u64(bytes, offset);
    if (value > static_cast<std::uint64_t>(maximum)) {
        throw std::runtime_error("invalid " + std::string(label));
    }
    return static_cast<Enum>(value);
}

std::size_t checked_capacity(std::string_view bytes,
                             std::size_t& offset,
                             std::string_view label) {
    const auto value = read_count(bytes, offset, kMaximumItems, label);
    if (value == 0U) {
        throw std::runtime_error("zero " + std::string(label) + " capacity");
    }
    return static_cast<std::size_t>(value);
}

void append_manifest(std::string& output, const KeyHandleManifest& manifest) {
    append_string(output, manifest.key_id);
    append_string(output, manifest.owner_entity_id);
    append_string(output, manifest.provider_id);
    append_string(output, manifest.algorithm_id);
    append_enum(output, manifest.function);
    append_string(output, manifest.implementation_route);
    append_string(output, manifest.implementation_digest);
    append_string(output, manifest.provider_locator_digest);
    append_enum(output, manifest.origin);
    append_enum(output, manifest.export_policy);
    append_u64(output, static_cast<std::uint64_t>(manifest.permitted_usages.size()));
    for (const auto usage : manifest.permitted_usages) {
        append_enum(output, usage);
    }
    append_string(output, manifest.custodian_entity_id);
    append_string(output, manifest.recovery_authority_entity_id);
    append_u64(output, static_cast<std::uint64_t>(manifest.operator_entity_ids.size()));
    for (const auto& operator_id : manifest.operator_entity_ids) {
        append_string(output, operator_id);
    }
    append_string(output, manifest.custody_policy_digest);
    append_string(output, manifest.creation_evidence_digest);
    append_string(output, manifest.attestation_evidence_digest);
    append_string(output, manifest.predecessor_key_id);
    append_u64(output, manifest.generation);
    append_u64(output, manifest.not_before);
    append_u64(output, manifest.not_after);
    append_u64(output, manifest.registered_at);
}

KeyHandleManifest read_manifest(std::string_view bytes, std::size_t& offset) {
    KeyHandleManifest manifest;
    manifest.key_id = read_string(bytes, offset);
    manifest.owner_entity_id = read_string(bytes, offset);
    manifest.provider_id = read_string(bytes, offset);
    manifest.algorithm_id = read_string(bytes, offset);
    manifest.function = read_enum(
        bytes, offset, CryptoFunction::key_wrapping, "cryptographic function");
    manifest.implementation_route = read_string(bytes, offset);
    manifest.implementation_digest = read_string(bytes, offset);
    manifest.provider_locator_digest = read_string(bytes, offset);
    manifest.origin = read_enum(bytes, offset, KeyOrigin::recovered, "key origin");
    manifest.export_policy = read_enum(
        bytes, offset, KeyExportPolicy::wrapped_export_only, "key export policy");
    const auto usage_count = read_count(bytes, offset, kMaximumUsages, "key usage");
    manifest.permitted_usages.reserve(static_cast<std::size_t>(usage_count));
    for (std::uint64_t index = 0U; index < usage_count; ++index) {
        manifest.permitted_usages.push_back(
            read_enum(bytes, offset, KeyUsage::unwrap, "key usage"));
    }
    manifest.custodian_entity_id = read_string(bytes, offset);
    manifest.recovery_authority_entity_id = read_string(bytes, offset);
    const auto operator_count =
        read_count(bytes, offset, kMaximumOperators, "key operator");
    manifest.operator_entity_ids.reserve(static_cast<std::size_t>(operator_count));
    for (std::uint64_t index = 0U; index < operator_count; ++index) {
        manifest.operator_entity_ids.push_back(read_string(bytes, offset));
    }
    manifest.custody_policy_digest = read_string(bytes, offset);
    manifest.creation_evidence_digest = read_string(bytes, offset);
    manifest.attestation_evidence_digest = read_string(bytes, offset);
    manifest.predecessor_key_id = read_string(bytes, offset);
    manifest.generation = read_u64(bytes, offset);
    manifest.not_before = read_u64(bytes, offset);
    manifest.not_after = read_u64(bytes, offset);
    manifest.registered_at = read_u64(bytes, offset);
    return manifest;
}

void append_transition(std::string& output, const KeyTransitionRecord& record) {
    append_string(output, record.id);
    append_string(output, record.key_id);
    append_enum(output, record.kind);
    append_enum(output, record.state_after);
    append_string(output, record.actor_entity_id);
    append_string(output, record.reason_digest);
    append_string(output, record.evidence_digest);
    append_u64(output, record.sequence);
    append_u64(output, record.occurred_at);
    append_u64(output, record.recorded_at);
    append_string(output, record.previous_transition_digest);
    append_string(output, record.transition_digest);
}

KeyTransitionRecord read_transition(std::string_view bytes, std::size_t& offset) {
    KeyTransitionRecord record;
    record.id = read_string(bytes, offset);
    record.key_id = read_string(bytes, offset);
    record.kind = read_enum(
        bytes, offset, KeyTransitionKind::destroy, "key transition kind");
    record.state_after = read_enum(
        bytes, offset, KeyLifecycleState::destroyed, "key lifecycle state");
    record.actor_entity_id = read_string(bytes, offset);
    record.reason_digest = read_string(bytes, offset);
    record.evidence_digest = read_string(bytes, offset);
    record.sequence = read_u64(bytes, offset);
    record.occurred_at = read_u64(bytes, offset);
    record.recorded_at = read_u64(bytes, offset);
    record.previous_transition_digest = read_string(bytes, offset);
    record.transition_digest = read_string(bytes, offset);
    return record;
}

void append_succession(std::string& output, const KeySuccessionRecord& record) {
    append_string(output, record.id);
    append_enum(output, record.kind);
    append_string(output, record.predecessor_key_id);
    append_string(output, record.successor_key_id);
    append_string(output, record.actor_entity_id);
    append_string(output, record.reason_digest);
    append_string(output, record.evidence_digest);
    append_u64(output, record.sequence);
    append_u64(output, record.occurred_at);
    append_u64(output, record.recorded_at);
    append_string(output, record.previous_succession_digest);
    append_string(output, record.succession_digest);
}

KeySuccessionRecord read_succession(std::string_view bytes, std::size_t& offset) {
    KeySuccessionRecord record;
    record.id = read_string(bytes, offset);
    record.kind = read_enum(
        bytes, offset, KeySuccessionKind::recovery, "key succession kind");
    record.predecessor_key_id = read_string(bytes, offset);
    record.successor_key_id = read_string(bytes, offset);
    record.actor_entity_id = read_string(bytes, offset);
    record.reason_digest = read_string(bytes, offset);
    record.evidence_digest = read_string(bytes, offset);
    record.sequence = read_u64(bytes, offset);
    record.occurred_at = read_u64(bytes, offset);
    record.recorded_at = read_u64(bytes, offset);
    record.previous_succession_digest = read_string(bytes, offset);
    record.succession_digest = read_string(bytes, offset);
    return record;
}

} // namespace

KeyCustodyStore::KeyCustodyStore(std::filesystem::path root,
                                 std::size_t maximum_record_bytes)
    : root_(std::move(root)), maximum_record_bytes_(maximum_record_bytes) {}

bool KeyCustodyStore::write(std::string_view owner_entity_id,
                            std::string_view version,
                            const KeyCustodyRegistry& registry,
                            KeyCustodyStoreError* error) const {
    if (owner_entity_id != registry.owner_entity_id()) {
        set_error(error,
                  KeyCustodyStoreErrorCode::owner_binding_mismatch,
                  "key-custody snapshot owner does not match the registry owner");
        return false;
    }
    try {
        const auto bytes = serialize(registry);
        storage::ImmutableSnapshotFiles files(root_, maximum_record_bytes_);
        storage::ImmutableFileError file_error;
        if (!files.write(owner_entity_id, version, kFileSuffix, bytes, &file_error)) {
            map_file_error(file_error, true, error);
            return false;
        }
        set_error(error, KeyCustodyStoreErrorCode::none, {});
        return true;
    } catch (const std::invalid_argument& exception) {
        set_error(error, KeyCustodyStoreErrorCode::invalid_registry, exception.what());
        return false;
    } catch (const std::exception& exception) {
        set_error(error, KeyCustodyStoreErrorCode::io_error, exception.what());
        return false;
    }
}

std::optional<KeyCustodyRegistry> KeyCustodyStore::read(
    std::string_view owner_entity_id,
    std::string_view version,
    KeyCustodyStoreError* error) const {
    storage::ImmutableSnapshotFiles files(root_, maximum_record_bytes_);
    storage::ImmutableFileError file_error;
    const auto bytes = files.read(owner_entity_id, version, kFileSuffix, &file_error);
    if (!bytes.has_value()) {
        map_file_error(file_error, false, error);
        return std::nullopt;
    }
    auto registry = deserialize(*bytes, error);
    if (registry.has_value() && registry->owner_entity_id() != owner_entity_id) {
        set_error(error,
                  KeyCustodyStoreErrorCode::owner_binding_mismatch,
                  "restored key-custody registry belongs to a different owner");
        return std::nullopt;
    }
    return registry;
}

std::string KeyCustodyStore::serialize(const KeyCustodyRegistry& registry) {
    if (!registry.verify()) {
        throw std::invalid_argument("cannot serialize an invalid key-custody registry");
    }
    std::string output(kMagic);
    append_u64(output, kSchemaVersion);
    append_string(output, registry.registry_id_);
    append_string(output, registry.entity_namespace_id_);
    append_string(output, registry.owner_entity_id_);
    append_u64(output, static_cast<std::uint64_t>(registry.key_capacity_));
    append_u64(output, static_cast<std::uint64_t>(registry.transition_capacity_));
    append_u64(output, static_cast<std::uint64_t>(registry.succession_capacity_));
    append_u64(output, static_cast<std::uint64_t>(registry.keys_.size()));
    for (const auto& [unused, manifest] : registry.keys_) {
        static_cast<void>(unused);
        append_manifest(output, manifest);
    }
    append_u64(output, static_cast<std::uint64_t>(registry.transitions_.size()));
    for (const auto& [key_id, history] : registry.transitions_) {
        append_string(output, key_id);
        append_u64(output, static_cast<std::uint64_t>(history.size()));
        for (const auto& record : history) {
            append_transition(output, record);
        }
    }
    append_u64(output, static_cast<std::uint64_t>(registry.successions_.size()));
    for (const auto& record : registry.successions_) {
        append_succession(output, record);
    }
    output += runtime::sha256(output);
    return output;
}

std::optional<KeyCustodyRegistry> KeyCustodyStore::deserialize(
    std::string_view bytes,
    KeyCustodyStoreError* error) {
    try {
        if (bytes.size() < kMagic.size() + sizeof(std::uint64_t) + kChecksumBytes
            || bytes.substr(0U, kMagic.size()) != kMagic) {
            throw std::runtime_error("invalid key-custody snapshot magic");
        }
        const auto payload = bytes.substr(0U, bytes.size() - kChecksumBytes);
        const auto checksum = bytes.substr(bytes.size() - kChecksumBytes);
        if (runtime::sha256(payload) != checksum) {
            throw std::runtime_error("key-custody snapshot checksum mismatch");
        }
        std::size_t offset = kMagic.size();
        if (read_u64(payload, offset) != kSchemaVersion) {
            set_error(error,
                      KeyCustodyStoreErrorCode::unsupported_schema,
                      "unsupported key-custody snapshot schema");
            return std::nullopt;
        }
        auto registry_id = read_string(payload, offset);
        auto entity_namespace_id = read_string(payload, offset);
        auto owner_entity_id = read_string(payload, offset);
        const auto key_capacity = checked_capacity(payload, offset, "key");
        const auto transition_capacity =
            checked_capacity(payload, offset, "key transition");
        const auto succession_capacity =
            checked_capacity(payload, offset, "key succession");
        KeyCustodyRegistry registry(std::move(registry_id),
                                    std::move(entity_namespace_id),
                                    std::move(owner_entity_id),
                                    key_capacity,
                                    transition_capacity,
                                    succession_capacity);
        const auto key_count = read_count(payload, offset, key_capacity, "key");
        for (std::uint64_t index = 0U; index < key_count; ++index) {
            auto manifest = read_manifest(payload, offset);
            const auto key_id = manifest.key_id;
            if (!registry.keys_.emplace(key_id, std::move(manifest)).second) {
                throw std::runtime_error("duplicate key in key-custody snapshot");
            }
        }
        const auto history_count = read_count(payload, offset, key_count, "key history");
        std::uint64_t total_transitions = 0U;
        for (std::uint64_t history_index = 0U; history_index < history_count;
             ++history_index) {
            auto key_id = read_string(payload, offset);
            const auto record_count =
                read_count(payload, offset, transition_capacity, "key transition");
            if (record_count == 0U || total_transitions > transition_capacity - record_count) {
                throw std::runtime_error("invalid key transition history size");
            }
            auto& history = registry.transitions_[key_id];
            if (!history.empty()) {
                throw std::runtime_error("duplicate key transition history");
            }
            history.reserve(static_cast<std::size_t>(record_count));
            for (std::uint64_t index = 0U; index < record_count; ++index) {
                history.push_back(read_transition(payload, offset));
            }
            total_transitions += record_count;
        }
        const auto succession_count =
            read_count(payload, offset, succession_capacity, "key succession");
        registry.successions_.reserve(static_cast<std::size_t>(succession_count));
        for (std::uint64_t index = 0U; index < succession_count; ++index) {
            registry.successions_.push_back(read_succession(payload, offset));
        }
        if (offset != payload.size() || !registry.rebuild_indexes()
            || !registry.verify()) {
            throw std::runtime_error("key-custody snapshot is noncanonical or invalid");
        }
        set_error(error, KeyCustodyStoreErrorCode::none, {});
        return registry;
    } catch (const std::exception& exception) {
        set_error(error, KeyCustodyStoreErrorCode::corrupt_record, exception.what());
        return std::nullopt;
    }
}

const std::filesystem::path& KeyCustodyStore::root() const noexcept {
    return root_;
}

std::size_t KeyCustodyStore::maximum_record_bytes() const noexcept {
    return maximum_record_bytes_;
}

} // namespace genesis::security
