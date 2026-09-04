#include "genesis/genesis.hpp"

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

using genesis::identity::DigitalLifeRecord;
using genesis::identity::EntityAddress;
using genesis::identity::EntityKind;
using genesis::identity::EntityRegistry;
using genesis::identity::LifeEntryDisposition;
using genesis::identity::LifeEvidenceClass;
using genesis::identity::LifeRecordDraft;
using genesis::identity::LifeRecordError;
using genesis::identity::LifeRecordErrorCode;
using genesis::identity::LifeRecordKind;
using genesis::identity::LifeRecordStore;
using genesis::identity::LifeRecordStoreError;
using genesis::identity::LifeRecordStoreErrorCode;
using genesis::identity::LifeVisibility;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string digest(char character) {
    return std::string(64U, character);
}

genesis::LineageIdentity make_identity(std::string organism_id = "organism-main") {
    genesis::LineageIdentity identity;
    identity.organism_id = std::move(organism_id);
    identity.genesis_id = "genesis-root";
    identity.lineage_id = "lineage-main";
    identity.birth_event_id = "birth-main";
    identity.genome_hash = digest('1');
    identity.inherited_state_hash = digest('2');
    identity.birth_snapshot_hash = digest('3');
    identity.identity_seed = "identity-seed";
    identity.lineage_signature = "PENDING_CRYPTOGRAPHIC_PROVIDER";
    identity.cryptographic_provenance = "UNAVAILABLE:provider-required";
    identity.generation = 0U;
    identity.birth_timestamp = 10;
    identity.ancestor_root_ids = {"genesis-root"};
    identity.origin = genesis::OriginKind::genesis;
    return identity;
}

struct Fixture final {
    genesis::LineageIdentity identity{make_identity()};
    EntityRegistry registry{"life.test", identity.organism_id, 32U, 32U};
    EntityAddress organism = genesis::identity::make_entity_address(
        "life.test", EntityKind::organism, identity.organism_id, digest('a'), 1U);
    EntityAddress record_entity = genesis::identity::make_entity_address(
        "life.test",
        EntityKind::record,
        genesis::identity::life_record_local_key(identity.organism_id),
        digest('b'),
        2U);
    EntityAddress source = genesis::identity::make_entity_address(
        "life.test", EntityKind::person, "source-person", digest('c'), 3U);
    EntityAddress shell = genesis::identity::make_entity_address(
        "life.test", EntityKind::shell, "shell-1", digest('d'), 4U);
    EntityAddress place = genesis::identity::make_entity_address(
        "life.test", EntityKind::place, "place-1", digest('e'), 5U);
    EntityAddress credential = genesis::identity::make_entity_address(
        "life.test", EntityKind::credential, "credential-1", digest('f'), 6U);
    genesis::cognition::AutobiographicalContinuity continuity{identity, {}, 64U, 8U};
    DigitalLifeRecord record{identity,
                             "life.test",
                             organism.entity_id,
                             record_entity.entity_id,
                             64U};

    Fixture() {
        require(registry.register_entity(organism), "fixture organism registration failed");
        require(registry.register_entity(record_entity), "fixture record registration failed");
        require(registry.register_entity(source), "fixture source registration failed");
        require(registry.register_entity(shell), "fixture shell registration failed");
        require(registry.register_entity(place), "fixture place registration failed");
        require(registry.register_entity(credential), "fixture credential registration failed");
        require(registry.verify(), "fixture entity registry is invalid");
    }

    std::string add_continuity_event(std::string event_id,
                                     std::uint64_t sequence,
                                     std::uint64_t logical_time,
                                     char content_character) {
        genesis::memory::MemoryNode memory;
        memory.id = "memory-" + event_id;
        memory.owner_id = identity.organism_id;
        memory.content_digest = digest(content_character);
        memory.provenance_digest = digest('9');
        memory.context = "life-record";
        memory.kind = genesis::memory::MemoryKind::episode;
        memory.origin = genesis::memory::Origin::direct_experience;
        memory.scope = genesis::memory::IdentityScope::organism_private;
        memory.state = genesis::memory::ActivationState::available;
        memory.confidence = 0.9;
        memory.strength = 1.0;
        memory.created_at = logical_time;
        memory.last_accessed = logical_time;
        memory.access_count = 1U;
        memory.features = {"life-record"};
        require(continuity.append(event_id, memory, sequence, logical_time),
                "fixture continuity append failed");
        return continuity.events().back().event_digest;
    }

    LifeRecordDraft birth() const {
        LifeRecordDraft draft;
        draft.id = identity.birth_event_id;
        draft.kind = LifeRecordKind::birth;
        draft.evidence_class = LifeEvidenceClass::official_record;
        draft.visibility = LifeVisibility::private_record;
        draft.disposition = LifeEntryDisposition::assertion;
        draft.label = "Genesis birth";
        draft.value_digest = identity.birth_snapshot_hash;
        draft.evidence_digest = digest('4');
        draft.authorization_evidence_digest = digest('5');
        draft.source_entity_id = organism.entity_id;
        draft.continuity_event_id = identity.birth_event_id;
        draft.continuity_digest = identity.birth_snapshot_hash;
        draft.effective_from = static_cast<std::uint64_t>(identity.birth_timestamp);
        draft.recorded_at = static_cast<std::uint64_t>(identity.birth_timestamp);
        return draft;
    }

    LifeRecordDraft assertion(std::string id,
                              LifeRecordKind kind,
                              std::string label,
                              std::string continuity_event_id,
                              std::string continuity_digest,
                              std::uint64_t at) const {
        LifeRecordDraft draft;
        draft.id = std::move(id);
        draft.kind = kind;
        draft.evidence_class = LifeEvidenceClass::observed;
        draft.visibility = LifeVisibility::private_record;
        draft.disposition = LifeEntryDisposition::assertion;
        draft.label = std::move(label);
        draft.value_digest = genesis::runtime::sha256("value:" + draft.id);
        draft.evidence_digest = genesis::runtime::sha256("evidence:" + draft.id);
        draft.authorization_evidence_digest = genesis::runtime::sha256("decision:" + draft.id);
        draft.source_entity_id = source.entity_id;
        draft.continuity_event_id = std::move(continuity_event_id);
        draft.continuity_digest = std::move(continuity_digest);
        draft.effective_from = at;
        draft.recorded_at = at;
        return draft;
    }
};

void test_enum_codecs() {
    for (const auto kind : {LifeRecordKind::birth,
                            LifeRecordKind::name,
                            LifeRecordKind::milestone,
                            LifeRecordKind::development,
                            LifeRecordKind::education,
                            LifeRecordKind::training,
                            LifeRecordKind::skill,
                            LifeRecordKind::competence,
                            LifeRecordKind::employment,
                            LifeRecordKind::project,
                            LifeRecordKind::research,
                            LifeRecordKind::achievement,
                            LifeRecordKind::certification,
                            LifeRecordKind::license,
                            LifeRecordKind::interest,
                            LifeRecordKind::community_role,
                            LifeRecordKind::association,
                            LifeRecordKind::residence,
                            LifeRecordKind::embodiment,
                            LifeRecordKind::lifecycle,
                            LifeRecordKind::retirement,
                            LifeRecordKind::legacy,
                            LifeRecordKind::custom}) {
        LifeRecordKind parsed{};
        require(genesis::identity::life_record_kind_from_string(
                    genesis::identity::to_string(kind), parsed)
                    && parsed == kind,
                "life record kind codec failed");
    }
    for (const auto evidence : {LifeEvidenceClass::self_claimed,
                                LifeEvidenceClass::taught,
                                LifeEvidenceClass::observed,
                                LifeEvidenceClass::tested,
                                LifeEvidenceClass::certified,
                                LifeEvidenceClass::official_record,
                                LifeEvidenceClass::derived}) {
        LifeEvidenceClass parsed{};
        require(genesis::identity::life_evidence_class_from_string(
                    genesis::identity::to_string(evidence), parsed)
                    && parsed == evidence,
                "life evidence codec failed");
    }
    for (const auto visibility : {LifeVisibility::private_record,
                                  LifeVisibility::trusted_shared,
                                  LifeVisibility::public_summary}) {
        LifeVisibility parsed{};
        require(genesis::identity::life_visibility_from_string(
                    genesis::identity::to_string(visibility), parsed)
                    && parsed == visibility,
                "life visibility codec failed");
    }
    for (const auto disposition : {LifeEntryDisposition::assertion,
                                   LifeEntryDisposition::supersession,
                                   LifeEntryDisposition::retraction}) {
        LifeEntryDisposition parsed{};
        require(genesis::identity::life_disposition_from_string(
                    genesis::identity::to_string(disposition), parsed)
                    && parsed == disposition,
                "life disposition codec failed");
    }
}

void test_birth_and_immutable_identity() {
    Fixture fixture;
    LifeRecordError error;
    require(!fixture.record.verify(), "life record without birth verified");

    const auto premature_digest = fixture.add_continuity_event("premature", 1U, 11U, 'a');
    auto premature_name = fixture.assertion(
        "name-premature", LifeRecordKind::name, "Premature", "premature", premature_digest, 11U);
    require(!fixture.record.append(premature_name, fixture.registry, &error)
                && error.code == LifeRecordErrorCode::birth_required,
            "non-birth entry was accepted before birth");

    auto wrong_birth = fixture.birth();
    wrong_birth.value_digest = digest('8');
    require(!fixture.record.append(wrong_birth, fixture.registry, &error)
                && error.code == LifeRecordErrorCode::invalid_entry,
            "birth entry not bound to the birth snapshot was accepted");
    require(fixture.record.append(fixture.birth(), fixture.registry, &error),
            "valid immutable birth entry was rejected");
    require(fixture.record.verify() && fixture.record.entries().size() == 1U,
            "life record birth chain did not verify");

    premature_name.effective_from = 9U;
    require(!fixture.record.append(premature_name, fixture.registry, &error)
                && error.code == LifeRecordErrorCode::temporal_conflict,
            "life record accepted an effective date before birth");

    auto second_birth = fixture.birth();
    second_birth.id = "second-birth";
    require(!fixture.record.append(second_birth, fixture.registry, &error)
                && error.code == LifeRecordErrorCode::duplicate_birth,
            "second birth entry was accepted");
    const auto birth_view = fixture.record.current(LifeRecordKind::birth, 100U);
    require(birth_view.facts.size() == 1U && !birth_view.action_authorized
                && !birth_view.organism_identity_reassigned
                && !birth_view.credential_cryptographically_verified
                && fixture.record.identity().organism_id == fixture.identity.organism_id,
            "life record fact changed identity, authority or credential state");
}

void test_name_history_and_supersession() {
    Fixture fixture;
    require(fixture.record.append(fixture.birth(), fixture.registry), "birth append failed");
    const auto first_digest = fixture.add_continuity_event("name-event-1", 1U, 20U, 'a');
    auto first_name = fixture.assertion(
        "name-1", LifeRecordKind::name, "Genesis", "name-event-1", first_digest, 20U);
    require(fixture.record.append(first_name, fixture.registry), "initial name append failed");

    const auto second_digest = fixture.add_continuity_event("name-event-2", 2U, 30U, 'b');
    auto second_name = fixture.assertion(
        "name-2", LifeRecordKind::name, "Nova", "name-event-2", second_digest, 30U);
    second_name.disposition = LifeEntryDisposition::supersession;
    second_name.supersedes_entry_id = first_name.id;
    require(fixture.record.append(second_name, fixture.registry), "name change append failed");
    require(fixture.record.original_name() == std::optional<std::string>{"Genesis"}
                && fixture.record.current_name(25U) == std::optional<std::string>{"Genesis"}
                && fixture.record.current_name(35U) == std::optional<std::string>{"Nova"}
                && fixture.record.history(LifeRecordKind::name).size() == 2U,
            "name history or effective-time view is incorrect");

    LifeRecordError error;
    const auto invalid_digest = fixture.add_continuity_event("name-event-invalid", 3U, 35U, 'c');
    auto parallel_name = fixture.assertion(
        "name-parallel", LifeRecordKind::name, "Parallel", "name-event-invalid", invalid_digest, 35U);
    require(!fixture.record.append(parallel_name, fixture.registry, &error)
                && error.code == LifeRecordErrorCode::name_history_conflict,
            "parallel recognized name assertion was accepted");

    auto resupersede = parallel_name;
    resupersede.disposition = LifeEntryDisposition::supersession;
    resupersede.supersedes_entry_id = first_name.id;
    require(!fixture.record.append(resupersede, fixture.registry, &error)
                && error.code == LifeRecordErrorCode::entry_already_superseded,
            "historical name received a second supersession");

    const auto retract_digest = fixture.add_continuity_event("name-event-3", 4U, 40U, 'd');
    auto retraction = fixture.assertion(
        "name-retract", LifeRecordKind::name, "Name retracted", "name-event-3", retract_digest, 40U);
    retraction.disposition = LifeEntryDisposition::retraction;
    retraction.supersedes_entry_id = second_name.id;
    require(fixture.record.append(retraction, fixture.registry), "name retraction failed");
    require(!fixture.record.current_name(45U).has_value(),
            "retracted recognized name remained current");

    const auto restart_digest = fixture.add_continuity_event("name-event-4", 5U, 50U, 'e');
    auto restarted = fixture.assertion(
        "name-3", LifeRecordKind::name, "Aster", "name-event-4", restart_digest, 50U);
    require(fixture.record.append(restarted, fixture.registry)
                && fixture.record.current_name(55U) == std::optional<std::string>{"Aster"}
                && fixture.record.verify(),
            "name history did not restart cleanly after retraction");
}

void test_reference_and_continuity_audits() {
    Fixture fixture;
    require(fixture.record.append(fixture.birth(), fixture.registry), "birth append failed");
    const auto embodiment_digest = fixture.add_continuity_event(
        "embodiment-event", 1U, 20U, 'a');
    auto embodiment = fixture.assertion("embodiment-1",
                                        LifeRecordKind::embodiment,
                                        "Primary shell",
                                        "embodiment-event",
                                        embodiment_digest,
                                        20U);
    embodiment.related_entity_id = fixture.shell.entity_id;
    require(fixture.record.append(embodiment, fixture.registry),
            "valid shell embodiment record was rejected");

    LifeRecordError error;
    auto wrong_kind = embodiment;
    wrong_kind.id = "embodiment-wrong-kind";
    wrong_kind.related_entity_id = fixture.place.entity_id;
    wrong_kind.value_digest = digest('6');
    wrong_kind.evidence_digest = digest('7');
    wrong_kind.authorization_evidence_digest = digest('8');
    require(!fixture.record.append(wrong_kind, fixture.registry, &error)
                && error.code == LifeRecordErrorCode::incompatible_related_kind,
            "embodiment record accepted a non-shell related entity");

    auto missing_source = fixture.assertion("missing-source",
                                            LifeRecordKind::milestone,
                                            "Missing source",
                                            "embodiment-event",
                                            embodiment_digest,
                                            21U);
    missing_source.source_entity_id = genesis::identity::derive_entity_id(
        "life.test", EntityKind::person, "absent");
    require(!fixture.record.append(missing_source, fixture.registry, &error)
                && error.code == LifeRecordErrorCode::missing_source,
            "life record accepted an unregistered evidence source");

    auto no_place = fixture.assertion("residence-no-place",
                                      LifeRecordKind::residence,
                                      "No place",
                                      "embodiment-event",
                                      embodiment_digest,
                                      22U);
    require(!fixture.record.append(no_place, fixture.registry, &error)
                && error.code == LifeRecordErrorCode::incompatible_related_kind,
            "residence record without a place was accepted");

    require(fixture.record.audit_references(fixture.registry).clean()
                && fixture.record.audit_continuity(fixture.continuity).clean(),
            "valid life record failed registry or continuity audit");

    auto late_organism = fixture.organism;
    auto late_record = fixture.record_entity;
    late_organism.registered_at = 25U;
    late_record.registered_at = 25U;
    EntityRegistry late_anchors{"life.test", fixture.identity.organism_id, 8U, 8U};
    require(late_anchors.register_entity(late_organism)
                && late_anchors.register_entity(late_record)
                && late_anchors.register_entity(fixture.source)
                && late_anchors.register_entity(fixture.shell)
                && late_anchors.verify(),
            "late-anchor audit fixture registration failed");
    const auto temporal_audit = fixture.record.audit_references(late_anchors);
    require(temporal_audit.temporal_mismatches >= 5U && !temporal_audit.clean(),
            "registry anchor registration after life entries was not detected");

    EntityRegistry incomplete{"life.test", fixture.identity.organism_id, 8U, 8U};
    require(incomplete.register_entity(fixture.organism)
                && incomplete.register_entity(fixture.record_entity)
                && incomplete.register_entity(fixture.source),
            "incomplete audit fixture registration failed");
    const auto reference_audit = fixture.record.audit_references(incomplete);
    require(reference_audit.missing_related_entities == 1U && !reference_audit.clean(),
            "missing related entity was not detected during audit");

    genesis::cognition::AutobiographicalContinuity missing_journal{
        fixture.identity, {}, 8U, 2U};
    const auto continuity_audit = fixture.record.audit_continuity(missing_journal);
    require(continuity_audit.missing_events == 1U && !continuity_audit.clean(),
            "missing autobiographical event was not detected during audit");
}

void test_capacity_and_persistence() {
    Fixture fixture;
    require(fixture.record.append(fixture.birth(), fixture.registry), "birth append failed");
    const auto name_digest = fixture.add_continuity_event("name-event", 1U, 20U, 'a');
    auto name = fixture.assertion(
        "name-1", LifeRecordKind::name, "Genesis", "name-event", name_digest, 20U);
    require(fixture.record.append(name, fixture.registry), "name append failed");

    DigitalLifeRecord bounded{fixture.identity,
                              "life.test",
                              fixture.organism.entity_id,
                              fixture.record_entity.entity_id,
                              1U};
    require(bounded.append(fixture.birth(), fixture.registry), "bounded birth failed");
    LifeRecordError record_error;
    require(!bounded.append(name, fixture.registry, &record_error)
                && record_error.code == LifeRecordErrorCode::capacity_exceeded,
            "life record capacity was not enforced");

    LifeRecordStoreError error;
    const auto bytes = LifeRecordStore::serialize(fixture.record);
    const auto decoded = LifeRecordStore::deserialize(bytes, &error);
    require(decoded.has_value() && error.code == LifeRecordStoreErrorCode::none
                && decoded->verify() && decoded->entries() == fixture.record.entries()
                && LifeRecordStore::serialize(*decoded) == bytes,
            "digital life record serialization round trip changed state");

    auto corrupt = bytes;
    corrupt[corrupt.size() / 2U] ^= 1;
    require(!LifeRecordStore::deserialize(corrupt, &error).has_value()
                && error.code == LifeRecordStoreErrorCode::corrupt_record,
            "corrupt digital life record was accepted");
    auto future = bytes;
    future[std::string_view{"GENESIS-LIFE-RECORD"}.size()] = 2;
    future.replace(future.size() - 64U,
                   64U,
                   genesis::runtime::sha256(
                       std::string_view{future}.substr(0U, future.size() - 64U)));
    require(!LifeRecordStore::deserialize(future, &error).has_value()
                && error.code == LifeRecordStoreErrorCode::unsupported_schema,
            "unsupported digital life record schema was accepted");

    const auto root = std::filesystem::temp_directory_path() / "genesis-life-record-store-v1";
    std::error_code cleanup_error;
    std::filesystem::remove_all(root, cleanup_error);
    LifeRecordStore store(root);
    require(store.write(fixture.record, "1.0.0", &error)
                && store.write(fixture.record, "1.0.0", &error),
            "digital life record idempotent immutable write failed");
    const auto restored = store.read(fixture.record.record_entity_id(),
                                     fixture.identity.organism_id,
                                     fixture.record.lineage_anchor_digest(),
                                     "1.0.0",
                                     &error);
    require(restored.has_value() && restored->verify()
                && LifeRecordStore::serialize(*restored) == bytes,
            "digital life record file recovery changed state");
    require(!store.read(fixture.record.record_entity_id(),
                        "different-organism",
                        fixture.record.lineage_anchor_digest(),
                        "1.0.0",
                        &error)
                && error.code == LifeRecordStoreErrorCode::corrupt_record,
            "digital life record organism mismatch was accepted");
    require(!store.read(fixture.record.record_entity_id(),
                        fixture.identity.organism_id,
                        digest('f'),
                        "1.0.0",
                        &error)
                && error.code == LifeRecordStoreErrorCode::corrupt_record,
            "digital life record lineage mismatch was accepted");
    require(!store.read("../escape",
                        fixture.identity.organism_id,
                        fixture.record.lineage_anchor_digest(),
                        "1.0.0",
                        &error)
                && error.code == LifeRecordStoreErrorCode::invalid_identifier,
            "unsafe digital life record path was accepted");

    auto conflicting = fixture.record;
    const auto milestone_digest = fixture.add_continuity_event("milestone-event", 2U, 30U, 'b');
    auto milestone = fixture.assertion("milestone-1",
                                       LifeRecordKind::milestone,
                                       "First milestone",
                                       "milestone-event",
                                       milestone_digest,
                                       30U);
    require(conflicting.append(milestone, fixture.registry), "conflict fixture append failed");
    require(!store.write(conflicting, "1.0.0", &error)
                && error.code == LifeRecordStoreErrorCode::conflicting_version,
            "conflicting immutable digital life record version was accepted");

    const auto tiny_root = root / "tiny";
    LifeRecordStore tiny_store(tiny_root, 1024U);
    require(bytes.size() > 1024U && !tiny_store.write(fixture.record, "1.0.0", &error)
                && error.code == LifeRecordStoreErrorCode::invalid_record,
            "digital life record size bound was not enforced");
    std::filesystem::remove_all(root, cleanup_error);
}

void test_shared_immutable_snapshot_files() {
    using genesis::storage::ImmutableFileError;
    using genesis::storage::ImmutableFileErrorCode;
    using genesis::storage::ImmutableSnapshotFiles;

    const auto root = std::filesystem::temp_directory_path()
                      / "genesis-immutable-snapshot-files-v1";
    std::error_code cleanup_error;
    std::filesystem::remove_all(root, cleanup_error);
    ImmutableSnapshotFiles files(root);
    ImmutableFileError error;

    require(files.write("owner", "1.0", "snap", "alpha", &error)
                && files.write("owner", "1.0", "snap", "alpha", &error),
            "shared immutable store idempotent write failed");
    require(!files.write("owner", "1.0", "snap", "beta", &error)
                && error.code == ImmutableFileErrorCode::conflicting_version,
            "shared immutable store accepted conflicting bytes");
    require(!files.read("../escape", "1.0", "snap", &error).has_value()
                && error.code == ImmutableFileErrorCode::invalid_identifier,
            "shared immutable store accepted an unsafe owner path");

    const auto unsafe_target = root / "directory.1.0.snap";
    std::filesystem::create_directories(unsafe_target);
    require(!files.read("directory", "1.0", "snap", &error).has_value()
                && error.code == ImmutableFileErrorCode::unsafe_file_type,
            "shared immutable store accepted a non-regular target");

    std::atomic<bool> start{false};
    bool left_written = false;
    bool right_written = false;
    ImmutableFileError left_error;
    ImmutableFileError right_error;
    std::thread left([&] {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        left_written = files.write("race", "1.0", "snap", "left", &left_error);
    });
    std::thread right([&] {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        right_written = files.write("race", "1.0", "snap", "right", &right_error);
    });
    start.store(true, std::memory_order_release);
    left.join();
    right.join();
    require(left_written != right_written,
            "concurrent immutable writers did not produce exactly one winner");
    require((left_written || left_error.code == ImmutableFileErrorCode::conflicting_version)
                && (right_written
                    || right_error.code == ImmutableFileErrorCode::conflicting_version),
            "concurrent immutable writer failure was not classified as a conflict");
    const auto winner = files.read("race", "1.0", "snap", &error);
    require(winner.has_value()
                && *winner == (left_written ? std::string{"left"}
                                            : std::string{"right"}),
            "concurrent immutable winner was not recovered exactly");

    std::filesystem::remove_all(root, cleanup_error);
}

} // namespace

int main() {
    try {
        test_enum_codecs();
        test_birth_and_immutable_identity();
        test_name_history_and_supersession();
        test_reference_and_continuity_audits();
        test_capacity_and_persistence();
        test_shared_immutable_snapshot_files();
        std::cout << "Genesis digital life record tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& exception) {
        std::cerr << "FAIL: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }
}
