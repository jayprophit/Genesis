#include "genesis/identity/life_record.hpp"

#include "genesis/runtime/runtime.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace genesis::identity {
namespace {

constexpr std::size_t kMaximumEntries = 1'000'000U;
constexpr std::size_t kMaximumIdLength = 256U;
constexpr std::size_t kMaximumLabelLength = 512U;
constexpr std::size_t kMaximumLocalKeyLength = 1024U;
constexpr std::size_t kMaximumIdentityFieldLength = 4096U;

constexpr std::array<std::string_view, 23> kKindNames{
    "birth",       "name",          "milestone",     "development",
    "education",   "training",      "skill",         "competence",
    "employment",  "project",       "research",      "achievement",
    "certification", "license",     "interest",      "community_role",
    "association", "residence",     "embodiment",    "lifecycle",
    "retirement",  "legacy",        "custom",
};

constexpr std::array<std::string_view, 7> kEvidenceNames{
    "self_claimed", "taught", "observed", "tested", "certified",
    "official_record", "derived",
};

constexpr std::array<std::string_view, 3> kVisibilityNames{
    "private_record", "trusted_shared", "public_summary",
};

constexpr std::array<std::string_view, 3> kDispositionNames{
    "assertion", "supersession", "retraction",
};

template <typename Enum, std::size_t Size>
bool parse_enum(std::string_view text,
                const std::array<std::string_view, Size>& names,
                Enum& result) noexcept {
    const auto found = std::find(names.begin(), names.end(), text);
    if (found == names.end()) {
        return false;
    }
    result = static_cast<Enum>(std::distance(names.begin(), found));
    return true;
}

template <typename Enum>
bool enum_in_range(Enum value, Enum maximum) noexcept {
    return static_cast<std::uint64_t>(value) <= static_cast<std::uint64_t>(maximum);
}

bool bounded_text(std::string_view value, std::size_t maximum) {
    if (value.empty() || value.size() > maximum) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return character >= 0x20U && character != 0x7fU;
    });
}

bool optional_bounded_text(const std::optional<std::string>& value,
                           std::size_t maximum) {
    return !value.has_value() || bounded_text(*value, maximum);
}

bool digest(std::string_view value) {
    return value.size() == 64U
           && std::all_of(value.begin(), value.end(), [](unsigned char character) {
                  return (character >= '0' && character <= '9')
                         || (character >= 'a' && character <= 'f');
              });
}

bool optional_digest(std::string_view value) {
    return value.empty() || digest(value);
}

bool valid_identity_shape(const LineageIdentity& identity) {
    if (!validate(identity).empty() || identity.birth_timestamp < 0
        || !enum_in_range(identity.origin, OriginKind::child)
        || !bounded_text(identity.organism_id, kMaximumIdLength)
        || !bounded_text(identity.genesis_id, kMaximumIdentityFieldLength)
        || !bounded_text(identity.lineage_id, kMaximumIdentityFieldLength)
        || !bounded_text(identity.birth_event_id, kMaximumIdLength)
        || (!identity.parent_a_id.empty()
            && !bounded_text(identity.parent_a_id, kMaximumIdentityFieldLength))
        || (!identity.parent_b_id.empty()
            && !bounded_text(identity.parent_b_id, kMaximumIdentityFieldLength))
        || !optional_digest(identity.genome_hash)
        || !optional_digest(identity.inherited_state_hash)
        || !digest(identity.birth_snapshot_hash)
        || (!identity.identity_seed.empty()
            && !bounded_text(identity.identity_seed, kMaximumIdentityFieldLength))
        || (!identity.lineage_signature.empty()
            && !bounded_text(identity.lineage_signature, kMaximumIdentityFieldLength))
        || (!identity.cryptographic_provenance.empty()
            && !bounded_text(identity.cryptographic_provenance,
                             kMaximumIdentityFieldLength))
        || identity.ancestor_root_ids.size() > kMaximumEntries) {
        return false;
    }
    return std::all_of(identity.ancestor_root_ids.begin(),
                       identity.ancestor_root_ids.end(),
                       [](const auto& ancestor) {
                           return bounded_text(ancestor, kMaximumIdentityFieldLength);
                       });
}

void append_material(std::string& output, std::string_view value) {
    output.append(std::to_string(value.size()));
    output.push_back(':');
    output.append(value);
}

void append_optional(std::string& output, const std::optional<std::string>& value) {
    output.push_back(value.has_value() ? '1' : '0');
    append_material(output, value.value_or(""));
}

void append_optional_time(std::string& output,
                          const std::optional<std::uint64_t>& value) {
    output.push_back(value.has_value() ? '1' : '0');
    append_material(output, std::to_string(value.value_or(0U)));
}

void set_error(LifeRecordError* error, LifeRecordErrorCode code, std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->message = std::move(message);
    }
}

bool identity_equal(const LineageIdentity& left, const LineageIdentity& right) {
    return std::tie(left.organism_id,
                    left.genesis_id,
                    left.lineage_id,
                    left.birth_event_id,
                    left.parent_a_id,
                    left.parent_b_id,
                    left.genome_hash,
                    left.inherited_state_hash,
                    left.birth_snapshot_hash,
                    left.identity_seed,
                    left.lineage_signature,
                    left.cryptographic_provenance,
                    left.generation,
                    left.birth_timestamp,
                    left.ancestor_root_ids,
                    left.origin)
           == std::tie(right.organism_id,
                       right.genesis_id,
                       right.lineage_id,
                       right.birth_event_id,
                       right.parent_a_id,
                       right.parent_b_id,
                       right.genome_hash,
                       right.inherited_state_hash,
                       right.birth_snapshot_hash,
                       right.identity_seed,
                       right.lineage_signature,
                       right.cryptographic_provenance,
                       right.generation,
                       right.birth_timestamp,
                       right.ancestor_root_ids,
                       right.origin);
}

bool related_kind_matches(LifeRecordKind kind,
                          const std::optional<EntityKind>& related_kind) noexcept {
    switch (kind) {
    case LifeRecordKind::birth:
    case LifeRecordKind::name:
        return !related_kind.has_value();
    case LifeRecordKind::association:
        return related_kind.has_value();
    case LifeRecordKind::embodiment:
        return related_kind == EntityKind::shell;
    case LifeRecordKind::residence:
        return related_kind == EntityKind::place;
    case LifeRecordKind::certification:
    case LifeRecordKind::license:
        return related_kind == EntityKind::credential;
    default:
        return true;
    }
}

bool valid_draft_shape(const LifeRecordDraft& draft) {
    return enum_in_range(draft.kind, LifeRecordKind::custom)
           && enum_in_range(draft.evidence_class, LifeEvidenceClass::derived)
           && enum_in_range(draft.visibility, LifeVisibility::public_summary)
           && enum_in_range(draft.disposition, LifeEntryDisposition::retraction)
           && bounded_text(draft.id, kMaximumIdLength)
           && bounded_text(draft.label, kMaximumLabelLength)
           && digest(draft.value_digest) && digest(draft.evidence_digest)
           && digest(draft.authorization_evidence_digest)
           && bounded_text(draft.source_entity_id, kMaximumIdLength)
           && optional_bounded_text(draft.related_entity_id, kMaximumIdLength)
           && digest(draft.continuity_digest)
           && bounded_text(draft.continuity_event_id, kMaximumIdLength)
           && (!draft.effective_until.has_value()
               || *draft.effective_until > draft.effective_from)
           && (draft.disposition != LifeEntryDisposition::assertion
                   ? bounded_text(draft.supersedes_entry_id, kMaximumIdLength)
                   : draft.supersedes_entry_id.empty())
           && (draft.disposition != LifeEntryDisposition::retraction
               || !draft.effective_until.has_value());
}

std::string entry_digest(std::string_view anchor, const LifeRecordEntry& entry) {
    std::string material{"genesis.life-record.entry.v1"};
    append_material(material, anchor);
    append_material(material, entry.id);
    append_material(material, to_string(entry.kind));
    append_material(material, to_string(entry.evidence_class));
    append_material(material, to_string(entry.visibility));
    append_material(material, to_string(entry.disposition));
    append_material(material, entry.label);
    append_material(material, entry.value_digest);
    append_material(material, entry.evidence_digest);
    append_material(material, entry.authorization_evidence_digest);
    append_material(material, entry.source_entity_id);
    append_optional(material, entry.related_entity_id);
    append_material(material, entry.supersedes_entry_id);
    append_material(material, entry.continuity_event_id);
    append_material(material, entry.continuity_digest);
    append_material(material, std::to_string(entry.sequence));
    append_material(material, std::to_string(entry.effective_from));
    append_optional_time(material, entry.effective_until);
    append_material(material, std::to_string(entry.recorded_at));
    append_material(material, entry.previous_entry_digest);
    return runtime::sha256(material);
}

LifeRecordDraft draft_from_entry(const LifeRecordEntry& entry) {
    return {entry.id,
            entry.kind,
            entry.evidence_class,
            entry.visibility,
            entry.disposition,
            entry.label,
            entry.value_digest,
            entry.evidence_digest,
            entry.authorization_evidence_digest,
            entry.source_entity_id,
            entry.related_entity_id,
            entry.supersedes_entry_id,
            entry.continuity_event_id,
            entry.continuity_digest,
            entry.effective_from,
            entry.effective_until,
            entry.recorded_at};
}

} // namespace

std::string_view to_string(LifeRecordKind kind) noexcept {
    const auto index = static_cast<std::size_t>(kind);
    return index < kKindNames.size() ? kKindNames[index] : "unknown";
}

std::string_view to_string(LifeEvidenceClass evidence) noexcept {
    const auto index = static_cast<std::size_t>(evidence);
    return index < kEvidenceNames.size() ? kEvidenceNames[index] : "unknown";
}

std::string_view to_string(LifeVisibility visibility) noexcept {
    const auto index = static_cast<std::size_t>(visibility);
    return index < kVisibilityNames.size() ? kVisibilityNames[index] : "unknown";
}

std::string_view to_string(LifeEntryDisposition disposition) noexcept {
    const auto index = static_cast<std::size_t>(disposition);
    return index < kDispositionNames.size() ? kDispositionNames[index] : "unknown";
}

bool life_record_kind_from_string(std::string_view text, LifeRecordKind& kind) noexcept {
    return parse_enum(text, kKindNames, kind);
}

bool life_evidence_class_from_string(std::string_view text,
                                     LifeEvidenceClass& evidence) noexcept {
    return parse_enum(text, kEvidenceNames, evidence);
}

bool life_visibility_from_string(std::string_view text,
                                 LifeVisibility& visibility) noexcept {
    return parse_enum(text, kVisibilityNames, visibility);
}

bool life_disposition_from_string(std::string_view text,
                                  LifeEntryDisposition& disposition) noexcept {
    return parse_enum(text, kDispositionNames, disposition);
}

bool LifeRecordReferenceAudit::clean() const noexcept {
    return registry_errors == 0U && anchor_mismatches == 0U && missing_sources == 0U
           && missing_related_entities == 0U && incompatible_related_kinds == 0U
           && temporal_mismatches == 0U;
}

bool LifeRecordContinuityAudit::clean() const noexcept {
    return journal_errors == 0U && identity_mismatches == 0U && missing_events == 0U
           && digest_mismatches == 0U && temporal_mismatches == 0U;
}

std::string derive_lineage_anchor_digest(const LineageIdentity& identity) {
    if (!valid_identity_shape(identity)) {
        throw std::invalid_argument("life record requires a complete lineage birth anchor");
    }
    std::string material{"genesis.life-record.lineage-anchor.v1"};
    append_material(material, identity.organism_id);
    append_material(material, identity.genesis_id);
    append_material(material, identity.lineage_id);
    append_material(material, identity.birth_event_id);
    append_material(material, identity.parent_a_id);
    append_material(material, identity.parent_b_id);
    append_material(material, identity.genome_hash);
    append_material(material, identity.inherited_state_hash);
    append_material(material, identity.birth_snapshot_hash);
    append_material(material, identity.identity_seed);
    append_material(material, identity.lineage_signature);
    append_material(material, identity.cryptographic_provenance);
    append_material(material, std::to_string(identity.generation));
    append_material(material, std::to_string(identity.birth_timestamp));
    append_material(material, std::to_string(identity.ancestor_root_ids.size()));
    for (const auto& ancestor : identity.ancestor_root_ids) {
        append_material(material, ancestor);
    }
    append_material(material, std::to_string(static_cast<std::uint64_t>(identity.origin)));
    return runtime::sha256(material);
}

std::string life_record_local_key(std::string_view organism_id) {
    if (!bounded_text(organism_id, kMaximumLocalKeyLength - 12U)) {
        throw std::invalid_argument("invalid organism identity for life-record address");
    }
    return "life-record:" + std::string(organism_id);
}

DigitalLifeRecord::DigitalLifeRecord(LineageIdentity identity,
                                     std::string entity_namespace_id,
                                     std::string organism_entity_id,
                                     std::string record_entity_id,
                                     std::size_t entry_capacity)
    : identity_(std::move(identity)),
      lineage_anchor_digest_(derive_lineage_anchor_digest(identity_)),
      entity_namespace_id_(std::move(entity_namespace_id)),
      organism_entity_id_(std::move(organism_entity_id)),
      record_entity_id_(std::move(record_entity_id)),
      entry_capacity_(entry_capacity) {
    if (entry_capacity_ == 0U || entry_capacity_ > kMaximumEntries
        || organism_entity_id_
               != derive_entity_id(
                   entity_namespace_id_, EntityKind::organism, identity_.organism_id)
        || record_entity_id_
               != derive_entity_id(entity_namespace_id_,
                                   EntityKind::record,
                                   life_record_local_key(identity_.organism_id))) {
        throw std::invalid_argument("invalid digital life record configuration");
    }
    entries_.reserve(std::min<std::size_t>(entry_capacity_, 4096U));
}

bool DigitalLifeRecord::append(LifeRecordDraft draft,
                               const EntityRegistry& registry,
                               LifeRecordError* error) {
    set_error(error, LifeRecordErrorCode::none, {});
    if (entries_.size() >= entry_capacity_) {
        set_error(error, LifeRecordErrorCode::capacity_exceeded, "life record capacity exceeded");
        return false;
    }
    if (entries_.size() == std::numeric_limits<std::uint64_t>::max()) {
        set_error(error,
                  LifeRecordErrorCode::sequence_exhausted,
                  "life record sequence exhausted");
        return false;
    }
    if (!valid_draft_shape(draft)) {
        set_error(error,
                  LifeRecordErrorCode::invalid_entry,
                  "life record entry fields, enums, digests or interval are invalid");
        return false;
    }
    if (entry_index_.contains(draft.id)) {
        set_error(error, LifeRecordErrorCode::duplicate_entry, "duplicate life record entry ID");
        return false;
    }

    const auto* organism = registry.find_entity(organism_entity_id_);
    const auto* record = registry.find_entity(record_entity_id_);
    if (!registry.verify() || registry.namespace_id() != entity_namespace_id_
        || organism == nullptr
        || record == nullptr || organism->kind != EntityKind::organism
        || organism->local_key != identity_.organism_id || record->kind != EntityKind::record
        || record->local_key != life_record_local_key(identity_.organism_id)) {
        set_error(error,
                  LifeRecordErrorCode::registry_anchor_mismatch,
                  "life record identity or record address is not bound in this registry");
        return false;
    }
    const auto* source = registry.find_entity(draft.source_entity_id);
    if (source == nullptr) {
        set_error(error,
                  LifeRecordErrorCode::missing_source,
                  "life record source entity is not registered");
        return false;
    }
    const EntityAddress* related = nullptr;
    if (draft.related_entity_id.has_value()) {
        related = registry.find_entity(*draft.related_entity_id);
        if (related == nullptr) {
            set_error(error,
                      LifeRecordErrorCode::missing_related_entity,
                      "life record related entity is not registered");
            return false;
        }
    }
    if (!related_kind_matches(
            draft.kind, related == nullptr ? std::nullopt
                                           : std::optional<EntityKind>{related->kind})) {
        set_error(error,
                  LifeRecordErrorCode::incompatible_related_kind,
                  "life record kind has a missing or incompatible related entity type");
        return false;
    }
    const auto birth_time = static_cast<std::uint64_t>(identity_.birth_timestamp);
    if (draft.effective_from < birth_time
        || draft.recorded_at < organism->registered_at
        || draft.recorded_at < record->registered_at
        || draft.recorded_at < source->registered_at
        || (related != nullptr && draft.recorded_at < related->registered_at)
        || draft.recorded_at < birth_time
        || (!entries_.empty() && draft.recorded_at < entries_.back().recorded_at)) {
        set_error(error,
                  LifeRecordErrorCode::temporal_conflict,
                  "life record time precedes birth, a referenced entity or the previous entry");
        return false;
    }

    if (entries_.empty()) {
        if (draft.kind != LifeRecordKind::birth) {
            set_error(error,
                      LifeRecordErrorCode::birth_required,
                      "the immutable birth entry must be first");
            return false;
        }
        if (draft.id != identity_.birth_event_id
            || draft.disposition != LifeEntryDisposition::assertion
            || draft.related_entity_id.has_value() || !draft.supersedes_entry_id.empty()
            || draft.value_digest != identity_.birth_snapshot_hash
            || draft.continuity_event_id != identity_.birth_event_id
            || draft.continuity_digest != identity_.birth_snapshot_hash
            || draft.effective_from
                   != static_cast<std::uint64_t>(identity_.birth_timestamp)
            || draft.effective_until.has_value()) {
            set_error(error,
                      LifeRecordErrorCode::invalid_entry,
                      "birth entry does not match the immutable lineage birth anchor");
            return false;
        }
    } else if (draft.kind == LifeRecordKind::birth) {
        set_error(error,
                  LifeRecordErrorCode::duplicate_birth,
                  "a life record has exactly one immutable birth entry");
        return false;
    }

    const LifeRecordEntry* target = nullptr;
    if (draft.disposition != LifeEntryDisposition::assertion) {
        target = find(draft.supersedes_entry_id);
        if (target == nullptr) {
            set_error(error,
                      LifeRecordErrorCode::missing_superseded_entry,
                      "superseded or retracted entry does not exist");
            return false;
        }
        if (target->kind != draft.kind) {
            set_error(error,
                      LifeRecordErrorCode::supersession_kind_mismatch,
                      "a life entry can only supersede the same record kind");
            return false;
        }
        if (superseded_by_.contains(target->id)) {
            set_error(error,
                      LifeRecordErrorCode::entry_already_superseded,
                      "life record entry already has a supersession or retraction");
            return false;
        }
        if (draft.effective_from < target->effective_from) {
            set_error(error,
                      LifeRecordErrorCode::temporal_conflict,
                      "supersession cannot take effect before its target");
            return false;
        }
    }

    if (draft.kind == LifeRecordKind::name) {
        if (!open_name_entry_id_.has_value()) {
            if (draft.disposition != LifeEntryDisposition::assertion) {
                set_error(error,
                          LifeRecordErrorCode::name_history_conflict,
                          "a name history without an open name must restart with an assertion");
                return false;
            }
            const auto last_name = std::find_if(
                entries_.rbegin(), entries_.rend(), [](const auto& entry) {
                    return entry.kind == LifeRecordKind::name;
                });
            if (last_name != entries_.rend()
                && draft.effective_from < last_name->effective_from) {
                set_error(error,
                          LifeRecordErrorCode::temporal_conflict,
                          "a restarted name cannot predate the prior name transition");
                return false;
            }
        } else if (draft.disposition == LifeEntryDisposition::assertion
                   || draft.supersedes_entry_id != *open_name_entry_id_) {
            set_error(error,
                      LifeRecordErrorCode::name_history_conflict,
                      "a name change must supersede or retract the current recognized name");
            return false;
        }
    }

    LifeRecordEntry entry{draft.id,
                          draft.kind,
                          draft.evidence_class,
                          draft.visibility,
                          draft.disposition,
                          draft.label,
                          draft.value_digest,
                          draft.evidence_digest,
                          draft.authorization_evidence_digest,
                          draft.source_entity_id,
                          draft.related_entity_id,
                          draft.supersedes_entry_id,
                          draft.continuity_event_id,
                          draft.continuity_digest,
                          static_cast<std::uint64_t>(entries_.size() + 1U),
                          draft.effective_from,
                          draft.effective_until,
                          draft.recorded_at,
                          entries_.empty() ? lineage_anchor_digest_
                                           : entries_.back().entry_digest,
                          {}};
    entry.entry_digest = entry_digest(lineage_anchor_digest_, entry);

    const auto index = entries_.size();
    auto entry_id = entry.id;
    entries_.push_back(std::move(entry));
    entry_index_.emplace(entry_id, index);
    if (draft.disposition != LifeEntryDisposition::assertion) {
        superseded_by_.emplace(draft.supersedes_entry_id, entry_id);
    }
    if (draft.kind == LifeRecordKind::name) {
        if (draft.disposition == LifeEntryDisposition::retraction) {
            open_name_entry_id_.reset();
        } else {
            open_name_entry_id_ = std::move(entry_id);
        }
    }
    return true;
}

const LifeRecordEntry* DigitalLifeRecord::find(std::string_view entry_id) const {
    const auto found = entry_index_.find(entry_id);
    return found == entry_index_.end() ? nullptr : &entries_[found->second];
}

std::vector<LifeRecordEntry> DigitalLifeRecord::history(LifeRecordKind kind) const {
    std::vector<LifeRecordEntry> result;
    for (const auto& entry : entries_) {
        if (entry.kind == kind) {
            result.push_back(entry);
        }
    }
    return result;
}

LifeRecordView DigitalLifeRecord::current(LifeRecordKind kind,
                                          std::uint64_t effective_at) const {
    std::map<std::string, LifeRecordEntry, std::less<>> active;
    for (const auto& entry : entries_) {
        if (entry.kind != kind || entry.effective_from > effective_at) {
            continue;
        }
        if (entry.disposition != LifeEntryDisposition::assertion) {
            active.erase(entry.supersedes_entry_id);
        }
        if (entry.disposition != LifeEntryDisposition::retraction
            && (!entry.effective_until.has_value()
                || effective_at < *entry.effective_until)) {
            active.emplace(entry.id, entry);
        }
    }
    LifeRecordView view;
    for (const auto& [unused_id, entry] : active) {
        static_cast<void>(unused_id);
        view.facts.push_back(entry);
    }
    std::sort(view.facts.begin(), view.facts.end(), [](const auto& left, const auto& right) {
        return left.sequence < right.sequence;
    });
    return view;
}

std::optional<std::string> DigitalLifeRecord::original_name() const {
    for (const auto& entry : entries_) {
        if (entry.kind == LifeRecordKind::name
            && entry.disposition == LifeEntryDisposition::assertion) {
            return entry.label;
        }
    }
    return std::nullopt;
}

std::optional<std::string> DigitalLifeRecord::current_name(
    std::uint64_t effective_at) const {
    const auto view = current(LifeRecordKind::name, effective_at);
    return view.facts.empty() ? std::nullopt
                              : std::optional<std::string>{view.facts.back().label};
}

LifeRecordReferenceAudit DigitalLifeRecord::audit_references(
    const EntityRegistry& registry) const {
    LifeRecordReferenceAudit audit;
    if (!registry.verify()) {
        ++audit.registry_errors;
    }
    const auto* organism = registry.find_entity(organism_entity_id_);
    const auto* record = registry.find_entity(record_entity_id_);
    if (registry.namespace_id() != entity_namespace_id_ || organism == nullptr
        || record == nullptr || organism->kind != EntityKind::organism
        || organism->local_key != identity_.organism_id || record->kind != EntityKind::record
        || record->local_key != life_record_local_key(identity_.organism_id)) {
        ++audit.anchor_mismatches;
    }
    for (const auto& entry : entries_) {
        if (organism != nullptr && entry.recorded_at < organism->registered_at) {
            ++audit.temporal_mismatches;
        }
        if (record != nullptr && entry.recorded_at < record->registered_at) {
            ++audit.temporal_mismatches;
        }
        const auto* source = registry.find_entity(entry.source_entity_id);
        if (source == nullptr) {
            ++audit.missing_sources;
        } else if (entry.recorded_at < source->registered_at) {
            ++audit.temporal_mismatches;
        }
        std::optional<EntityKind> related_kind;
        if (entry.related_entity_id.has_value()) {
            const auto* related = registry.find_entity(*entry.related_entity_id);
            if (related == nullptr) {
                ++audit.missing_related_entities;
            } else {
                related_kind = related->kind;
                if (entry.recorded_at < related->registered_at) {
                    ++audit.temporal_mismatches;
                }
            }
        }
        if (!related_kind_matches(entry.kind, related_kind)
            && !(entry.related_entity_id.has_value() && !related_kind.has_value())) {
            ++audit.incompatible_related_kinds;
        }
    }
    return audit;
}

LifeRecordContinuityAudit DigitalLifeRecord::audit_continuity(
    const cognition::AutobiographicalContinuity& continuity) const {
    LifeRecordContinuityAudit audit;
    if (!continuity.verify()) {
        ++audit.journal_errors;
    }
    if (!identity_equal(identity_, continuity.identity())) {
        ++audit.identity_mismatches;
    }
    std::map<std::string, const cognition::LifeEvent*, std::less<>> events;
    for (const auto& event : continuity.events()) {
        events.emplace(event.id, &event);
    }
    for (const auto& entry : entries_) {
        if (entry.kind == LifeRecordKind::birth) {
            if (entry.continuity_event_id != identity_.birth_event_id
                || entry.continuity_digest != identity_.birth_snapshot_hash) {
                ++audit.digest_mismatches;
            }
            continue;
        }
        const auto found = events.find(entry.continuity_event_id);
        if (found == events.end()) {
            ++audit.missing_events;
            continue;
        }
        if (found->second->event_digest != entry.continuity_digest) {
            ++audit.digest_mismatches;
        }
        if (found->second->logical_time > entry.recorded_at) {
            ++audit.temporal_mismatches;
        }
    }
    return audit;
}

bool DigitalLifeRecord::verify() const {
    try {
        if (lineage_anchor_digest_ != derive_lineage_anchor_digest(identity_)
            || entry_capacity_ == 0U || entry_capacity_ > kMaximumEntries
            || entries_.empty() || entries_.size() > entry_capacity_
            || organism_entity_id_
                   != derive_entity_id(
                       entity_namespace_id_, EntityKind::organism, identity_.organism_id)
            || record_entity_id_
                   != derive_entity_id(entity_namespace_id_,
                                       EntityKind::record,
                                       life_record_local_key(identity_.organism_id))) {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    std::map<std::string, std::size_t, std::less<>> expected_index;
    std::map<std::string, std::string, std::less<>> expected_superseded;
    std::optional<std::string> expected_open_name;
    std::optional<std::uint64_t> last_name_transition_time;
    std::string previous = lineage_anchor_digest_;
    std::uint64_t previous_recorded_at = static_cast<std::uint64_t>(identity_.birth_timestamp);
    std::size_t birth_count = 0U;

    for (std::size_t index = 0U; index < entries_.size(); ++index) {
        const auto& entry = entries_[index];
        const auto draft = draft_from_entry(entry);
        if (!valid_draft_shape(draft)
            || entry.effective_from
                   < static_cast<std::uint64_t>(identity_.birth_timestamp)
            || entry.sequence != index + 1U
            || entry.previous_entry_digest != previous
            || entry.entry_digest != entry_digest(lineage_anchor_digest_, entry)
            || entry.recorded_at < previous_recorded_at
            || !expected_index.emplace(entry.id, index).second) {
            return false;
        }
        if (entry.kind == LifeRecordKind::birth) {
            ++birth_count;
            if (index != 0U || entry.id != identity_.birth_event_id
                || entry.disposition != LifeEntryDisposition::assertion
                || entry.related_entity_id.has_value() || !entry.supersedes_entry_id.empty()
                || entry.value_digest != identity_.birth_snapshot_hash
                || entry.continuity_event_id != identity_.birth_event_id
                || entry.continuity_digest != identity_.birth_snapshot_hash
                || entry.effective_from
                       != static_cast<std::uint64_t>(identity_.birth_timestamp)
                || entry.effective_until.has_value()) {
                return false;
            }
        } else if (index == 0U) {
            return false;
        }

        if (entry.disposition != LifeEntryDisposition::assertion) {
            const auto target = expected_index.find(entry.supersedes_entry_id);
            if (target == expected_index.end()
                || entries_[target->second].kind != entry.kind
                || entry.effective_from < entries_[target->second].effective_from
                || !expected_superseded.emplace(entry.supersedes_entry_id, entry.id).second) {
                return false;
            }
        }
        if (entry.kind == LifeRecordKind::name) {
            if (!expected_open_name.has_value()) {
                if (entry.disposition != LifeEntryDisposition::assertion) {
                    return false;
                }
                if (last_name_transition_time.has_value()
                    && entry.effective_from < *last_name_transition_time) {
                    return false;
                }
            } else if (entry.disposition == LifeEntryDisposition::assertion
                       || entry.supersedes_entry_id != *expected_open_name) {
                return false;
            }
            if (entry.disposition == LifeEntryDisposition::retraction) {
                expected_open_name.reset();
            } else {
                expected_open_name = entry.id;
            }
            last_name_transition_time = entry.effective_from;
        }
        previous = entry.entry_digest;
        previous_recorded_at = entry.recorded_at;
    }
    return birth_count == 1U && expected_index == entry_index_
           && expected_superseded == superseded_by_
           && expected_open_name == open_name_entry_id_;
}

bool DigitalLifeRecord::rebuild_indexes() {
    entry_index_.clear();
    superseded_by_.clear();
    open_name_entry_id_.reset();
    for (std::size_t index = 0U; index < entries_.size(); ++index) {
        const auto& entry = entries_[index];
        if (!entry_index_.emplace(entry.id, index).second) {
            return false;
        }
        if (entry.disposition != LifeEntryDisposition::assertion
            && !superseded_by_.emplace(entry.supersedes_entry_id, entry.id).second) {
            return false;
        }
        if (entry.kind == LifeRecordKind::name) {
            if (entry.disposition == LifeEntryDisposition::retraction) {
                open_name_entry_id_.reset();
            } else {
                open_name_entry_id_ = entry.id;
            }
        }
    }
    return true;
}

const LineageIdentity& DigitalLifeRecord::identity() const noexcept {
    return identity_;
}

const std::string& DigitalLifeRecord::lineage_anchor_digest() const noexcept {
    return lineage_anchor_digest_;
}

const std::string& DigitalLifeRecord::entity_namespace_id() const noexcept {
    return entity_namespace_id_;
}

const std::string& DigitalLifeRecord::organism_entity_id() const noexcept {
    return organism_entity_id_;
}

const std::string& DigitalLifeRecord::record_entity_id() const noexcept {
    return record_entity_id_;
}

std::size_t DigitalLifeRecord::entry_capacity() const noexcept {
    return entry_capacity_;
}

const std::vector<LifeRecordEntry>& DigitalLifeRecord::entries() const noexcept {
    return entries_;
}

} // namespace genesis::identity
