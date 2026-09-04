#include "genesis/genesis.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

genesis::LineageIdentity make_identity() {
    genesis::LineageIdentity identity;
    identity.organism_id = "life-benchmark";
    identity.genesis_id = "genesis-root";
    identity.lineage_id = "life-benchmark-lineage";
    identity.birth_event_id = "life-benchmark-birth";
    identity.genome_hash = genesis::runtime::sha256("benchmark-genome");
    identity.inherited_state_hash = genesis::runtime::sha256("benchmark-inherited");
    identity.birth_snapshot_hash = genesis::runtime::sha256("benchmark-birth");
    identity.identity_seed = "benchmark-seed";
    identity.lineage_signature = "PENDING_CRYPTOGRAPHIC_PROVIDER";
    identity.cryptographic_provenance = "UNAVAILABLE:provider-required";
    identity.birth_timestamp = 1;
    identity.ancestor_root_ids = {"genesis-root"};
    identity.origin = genesis::OriginKind::genesis;
    return identity;
}

} // namespace

int main() {
    using namespace genesis::identity;
    constexpr std::size_t entry_total = 10'000U;
    const auto identity = make_identity();
    EntityRegistry registry{"life.benchmark", identity.organism_id, 3U, 1U};
    const auto organism = make_entity_address("life.benchmark",
                                              EntityKind::organism,
                                              identity.organism_id,
                                              genesis::runtime::sha256("organism evidence"),
                                              1U);
    const auto record_entity = make_entity_address(
        "life.benchmark",
        EntityKind::record,
        life_record_local_key(identity.organism_id),
        genesis::runtime::sha256("record evidence"),
        1U);
    const auto source = make_entity_address("life.benchmark",
                                            EntityKind::person,
                                            "benchmark-source",
                                            genesis::runtime::sha256("source evidence"),
                                            1U);
    if (!registry.register_entity(organism) || !registry.register_entity(record_entity)
        || !registry.register_entity(source)) {
        std::cerr << "life benchmark registry setup failed\n";
        return 1;
    }

    genesis::cognition::AutobiographicalContinuity continuity{identity, {}, 2U, 1U};
    genesis::memory::MemoryNode memory;
    memory.id = "benchmark-life-memory";
    memory.owner_id = identity.organism_id;
    memory.content_digest = genesis::runtime::sha256("benchmark life memory");
    memory.provenance_digest = genesis::runtime::sha256("benchmark memory provenance");
    memory.context = "life-benchmark";
    memory.kind = genesis::memory::MemoryKind::episode;
    memory.origin = genesis::memory::Origin::direct_experience;
    memory.scope = genesis::memory::IdentityScope::organism_private;
    memory.state = genesis::memory::ActivationState::available;
    memory.confidence = 1.0;
    memory.strength = 1.0;
    memory.created_at = 2U;
    memory.last_accessed = 2U;
    memory.access_count = 1U;
    memory.features = {"life-benchmark"};
    if (!continuity.append("benchmark-evidence-event", memory, 1U, 2U)) {
        std::cerr << "life benchmark continuity setup failed\n";
        return 1;
    }
    const auto continuity_digest = continuity.events().back().event_digest;

    DigitalLifeRecord record{identity,
                             "life.benchmark",
                             organism.entity_id,
                             record_entity.entity_id,
                             entry_total};
    LifeRecordDraft birth;
    birth.id = identity.birth_event_id;
    birth.kind = LifeRecordKind::birth;
    birth.evidence_class = LifeEvidenceClass::official_record;
    birth.label = "Benchmark birth";
    birth.value_digest = identity.birth_snapshot_hash;
    birth.evidence_digest = genesis::runtime::sha256("birth evidence");
    birth.authorization_evidence_digest = genesis::runtime::sha256("birth admission");
    birth.source_entity_id = organism.entity_id;
    birth.continuity_event_id = identity.birth_event_id;
    birth.continuity_digest = identity.birth_snapshot_hash;
    birth.effective_from = 1U;
    birth.recorded_at = 1U;

    const auto build_start = std::chrono::steady_clock::now();
    if (!record.append(std::move(birth), registry)) {
        std::cerr << "life benchmark birth append failed\n";
        return 1;
    }
    for (std::size_t index = 1U; index < entry_total; ++index) {
        LifeRecordDraft draft;
        draft.id = "milestone-" + std::to_string(index);
        draft.kind = LifeRecordKind::milestone;
        draft.evidence_class = LifeEvidenceClass::observed;
        draft.label = "Benchmark milestone " + std::to_string(index);
        draft.value_digest = genesis::runtime::sha256("value-" + std::to_string(index));
        draft.evidence_digest = genesis::runtime::sha256(
            "evidence-" + std::to_string(index));
        draft.authorization_evidence_digest = genesis::runtime::sha256(
            "admission-" + std::to_string(index));
        draft.source_entity_id = source.entity_id;
        draft.continuity_event_id = "benchmark-evidence-event";
        draft.continuity_digest = continuity_digest;
        draft.effective_from = static_cast<std::uint64_t>(index + 1U);
        draft.recorded_at = static_cast<std::uint64_t>(index + 1U);
        if (!record.append(std::move(draft), registry)) {
            std::cerr << "life benchmark append failed at " << index << '\n';
            return 1;
        }
    }
    const auto build_end = std::chrono::steady_clock::now();

    const auto snapshot = LifeRecordStore::serialize(record);
    const auto root = std::filesystem::temp_directory_path()
                      / "genesis-life-record-benchmark";
    std::error_code filesystem_error;
    std::filesystem::remove_all(root, filesystem_error);
    LifeRecordStore store(root);
    LifeRecordStoreError error;
    const auto roundtrip_start = std::chrono::steady_clock::now();
    const auto written = store.write(record, "1.0.0", &error);
    const auto restored = store.read(record.record_entity_id(),
                                     identity.organism_id,
                                     record.lineage_anchor_digest(),
                                     "1.0.0",
                                     &error);
    const auto roundtrip_end = std::chrono::steady_clock::now();

    const auto verified = written && restored.has_value() && restored->verify()
                          && restored->entries().size() == entry_total
                          && restored->audit_references(registry).clean()
                          && restored->audit_continuity(continuity).clean()
                          && LifeRecordStore::serialize(*restored) == snapshot;
    std::cout << "life_record_entries=" << entry_total
              << " snapshot_bytes=" << snapshot.size()
              << " build_ms="
              << std::chrono::duration_cast<std::chrono::milliseconds>(build_end - build_start)
                     .count()
              << " roundtrip_ms="
              << std::chrono::duration_cast<std::chrono::milliseconds>(roundtrip_end
                                                                        - roundtrip_start)
                     .count()
              << " snapshot_digest=" << genesis::runtime::sha256(snapshot)
              << " verified=" << (verified ? 1 : 0) << '\n';
    std::filesystem::remove_all(root, filesystem_error);
    return verified ? 0 : 1;
}
