#pragma once

#include "genesis/cognition/continuity.hpp"
#include "genesis/identity/entity_registry.hpp"
#include "genesis/identity/lineage.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::identity {

class LifeRecordStore;

// Serialized enums are append-only. Existing numeric values are migration
// boundaries and must never be reordered or repurposed.
enum class LifeRecordKind : std::uint8_t {
    birth,
    name,
    milestone,
    development,
    education,
    training,
    skill,
    competence,
    employment,
    project,
    research,
    achievement,
    certification,
    license,
    interest,
    community_role,
    association,
    residence,
    embodiment,
    lifecycle,
    retirement,
    legacy,
    custom,
};

// This classifies where an assertion came from. It is not a trust score and
// does not turn a digest into proof.
enum class LifeEvidenceClass : std::uint8_t {
    self_claimed,
    taught,
    observed,
    tested,
    certified,
    official_record,
    derived,
};

enum class LifeVisibility : std::uint8_t {
    private_record,
    trusted_shared,
    public_summary,
};

enum class LifeEntryDisposition : std::uint8_t {
    assertion,
    supersession,
    retraction,
};

[[nodiscard]] std::string_view to_string(LifeRecordKind kind) noexcept;
[[nodiscard]] std::string_view to_string(LifeEvidenceClass evidence) noexcept;
[[nodiscard]] std::string_view to_string(LifeVisibility visibility) noexcept;
[[nodiscard]] std::string_view to_string(LifeEntryDisposition disposition) noexcept;
[[nodiscard]] bool life_record_kind_from_string(std::string_view text,
                                                LifeRecordKind& kind) noexcept;
[[nodiscard]] bool life_evidence_class_from_string(
    std::string_view text,
    LifeEvidenceClass& evidence) noexcept;
[[nodiscard]] bool life_visibility_from_string(std::string_view text,
                                               LifeVisibility& visibility) noexcept;
[[nodiscard]] bool life_disposition_from_string(
    std::string_view text,
    LifeEntryDisposition& disposition) noexcept;

struct LifeRecordDraft final {
    std::string id;
    LifeRecordKind kind{LifeRecordKind::custom};
    LifeEvidenceClass evidence_class{LifeEvidenceClass::self_claimed};
    LifeVisibility visibility{LifeVisibility::private_record};
    LifeEntryDisposition disposition{LifeEntryDisposition::assertion};
    std::string label;
    std::string value_digest;
    std::string evidence_digest;
    std::string authorization_evidence_digest;
    std::string source_entity_id;
    std::optional<std::string> related_entity_id;
    std::string supersedes_entry_id;
    std::string continuity_event_id;
    std::string continuity_digest;
    std::uint64_t effective_from{};
    std::optional<std::uint64_t> effective_until;
    std::uint64_t recorded_at{};
};

struct LifeRecordEntry final {
    std::string id;
    LifeRecordKind kind{LifeRecordKind::custom};
    LifeEvidenceClass evidence_class{LifeEvidenceClass::self_claimed};
    LifeVisibility visibility{LifeVisibility::private_record};
    LifeEntryDisposition disposition{LifeEntryDisposition::assertion};
    std::string label;
    std::string value_digest;
    std::string evidence_digest;
    std::string authorization_evidence_digest;
    std::string source_entity_id;
    std::optional<std::string> related_entity_id;
    std::string supersedes_entry_id;
    std::string continuity_event_id;
    std::string continuity_digest;
    std::uint64_t sequence{};
    std::uint64_t effective_from{};
    std::optional<std::uint64_t> effective_until;
    std::uint64_t recorded_at{};
    std::string previous_entry_digest;
    std::string entry_digest;

    [[nodiscard]] bool operator==(const LifeRecordEntry&) const = default;
};

struct LifeRecordView final {
    std::vector<LifeRecordEntry> facts;
    bool action_authorized{false};
    bool organism_identity_reassigned{false};
    bool credential_cryptographically_verified{false};
};

struct LifeRecordReferenceAudit final {
    std::size_t registry_errors{};
    std::size_t anchor_mismatches{};
    std::size_t missing_sources{};
    std::size_t missing_related_entities{};
    std::size_t incompatible_related_kinds{};
    std::size_t temporal_mismatches{};

    [[nodiscard]] bool clean() const noexcept;
};

struct LifeRecordContinuityAudit final {
    std::size_t journal_errors{};
    std::size_t identity_mismatches{};
    std::size_t missing_events{};
    std::size_t digest_mismatches{};
    std::size_t temporal_mismatches{};

    [[nodiscard]] bool clean() const noexcept;
};

enum class LifeRecordErrorCode : std::uint8_t {
    none,
    invalid_entry,
    capacity_exceeded,
    duplicate_entry,
    sequence_exhausted,
    birth_required,
    duplicate_birth,
    temporal_conflict,
    missing_source,
    missing_related_entity,
    incompatible_related_kind,
    missing_superseded_entry,
    supersession_kind_mismatch,
    entry_already_superseded,
    name_history_conflict,
    registry_anchor_mismatch,
};

struct LifeRecordError final {
    LifeRecordErrorCode code{LifeRecordErrorCode::none};
    std::string message;
};

[[nodiscard]] std::string derive_lineage_anchor_digest(const LineageIdentity& identity);
[[nodiscard]] std::string life_record_local_key(std::string_view organism_id);

class DigitalLifeRecord final {
public:
    DigitalLifeRecord(LineageIdentity identity,
                      std::string entity_namespace_id,
                      std::string organism_entity_id,
                      std::string record_entity_id,
                      std::size_t entry_capacity);

    [[nodiscard]] bool append(LifeRecordDraft draft,
                              const EntityRegistry& registry,
                              LifeRecordError* error = nullptr);

    [[nodiscard]] const LifeRecordEntry* find(std::string_view entry_id) const;
    [[nodiscard]] std::vector<LifeRecordEntry> history(LifeRecordKind kind) const;
    [[nodiscard]] LifeRecordView current(LifeRecordKind kind,
                                         std::uint64_t effective_at) const;
    [[nodiscard]] std::optional<std::string> original_name() const;
    [[nodiscard]] std::optional<std::string> current_name(
        std::uint64_t effective_at) const;

    [[nodiscard]] LifeRecordReferenceAudit audit_references(
        const EntityRegistry& registry) const;
    [[nodiscard]] LifeRecordContinuityAudit audit_continuity(
        const cognition::AutobiographicalContinuity& continuity) const;
    [[nodiscard]] bool verify() const;

    [[nodiscard]] const LineageIdentity& identity() const noexcept;
    [[nodiscard]] const std::string& lineage_anchor_digest() const noexcept;
    [[nodiscard]] const std::string& entity_namespace_id() const noexcept;
    [[nodiscard]] const std::string& organism_entity_id() const noexcept;
    [[nodiscard]] const std::string& record_entity_id() const noexcept;
    [[nodiscard]] std::size_t entry_capacity() const noexcept;
    [[nodiscard]] const std::vector<LifeRecordEntry>& entries() const noexcept;

private:
    friend class LifeRecordStore;

    [[nodiscard]] bool rebuild_indexes();

    LineageIdentity identity_;
    std::string lineage_anchor_digest_;
    std::string entity_namespace_id_;
    std::string organism_entity_id_;
    std::string record_entity_id_;
    std::size_t entry_capacity_{};
    std::vector<LifeRecordEntry> entries_;
    std::map<std::string, std::size_t, std::less<>> entry_index_;
    std::map<std::string, std::string, std::less<>> superseded_by_;
    std::optional<std::string> open_name_entry_id_;
};

} // namespace genesis::identity
