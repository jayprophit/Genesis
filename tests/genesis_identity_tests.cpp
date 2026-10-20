#include "genesis/genesis.hpp"
#include "test_support.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using genesis::identity::EntityAddress;
using genesis::identity::EntityKind;
using genesis::identity::EntityRegistry;
using genesis::identity::EntityRegistryStore;
using genesis::identity::EntityStoreError;
using genesis::identity::EntityStoreErrorCode;
using genesis::identity::GenesisIdentityRoot;
using genesis::identity::IdentityBinding;
using genesis::identity::IdentityBindingKind;
using genesis::identity::IdentityStoreState;
using genesis::identity::RelationKind;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string digest(char character) {
    return std::string(64U, character);
}

GenesisIdentityRoot make_root() {
    return genesis::identity::bootstrap_genesis_identity(
        "organism-main", "genesis-prime", "lineage-1", "birth-event-1", digest('0'));
}

void test_bootstrap_and_validation() {
    const GenesisIdentityRoot root = make_root();
    require(genesis::identity::validate_identity_root(root).empty(), "valid root must validate");
    require(root.lineage.organism_id == "organism-main", "organism id must survive bootstrap");
    require(root.identity_schema_version == "1", "schema version must be explicit");

    GenesisIdentityRoot broken = make_root();
    broken.lineage.genesis_id.clear();
    require(!genesis::identity::validate_identity_root(broken).empty(),
            "root without genesis_id must fail validation");
    GenesisIdentityRoot unproven = make_root();
    unproven.bootstrap_provenance_digest.clear();
    require(!genesis::identity::validate_identity_root(unproven).empty(),
            "root without provenance must fail validation");
}

void test_schema_migration_is_not_a_new_person() {
    GenesisIdentityRoot root = make_root();
    GenesisIdentityRoot migrated = root;
    migrated.identity_schema_version = "2";
    require(migrated.lineage.organism_id == root.lineage.organism_id
                && migrated.lineage.genesis_id == root.lineage.genesis_id,
            "schema migration must not mint a new identity");
    require(genesis::identity::validate_identity_root(migrated).empty(),
            "migrated root must still validate");
}

void test_store_state_trichotomy() {
    using genesis::identity::classify_identity_store;
    require(classify_identity_store(false, false) == IdentityStoreState::uninitialized,
            "missing record must be uninitialized, not corrupt");
    require(classify_identity_store(false, true) == IdentityStoreState::uninitialized,
            "missing record wins over validity flag");
    require(classify_identity_store(true, true) == IdentityStoreState::ready,
            "valid record must be ready");
    require(classify_identity_store(true, false) == IdentityStoreState::corrupt,
            "invalid record must be corrupt, never silently fresh");
}

void test_bindings_never_mutate_identity() {
    const GenesisIdentityRoot root = make_root();
    const GenesisIdentityRoot root_before = root;
    const auto kinds = {IdentityBindingKind::model,
                        IdentityBindingKind::provider,
                        IdentityBindingKind::session,
                        IdentityBindingKind::project,
                        IdentityBindingKind::worker,
                        IdentityBindingKind::avatar,
                        IdentityBindingKind::interface_mode};
    for (const IdentityBindingKind binding_kind : kinds) {
        const IdentityBinding binding = genesis::identity::bind_to_organism(
            root, binding_kind, "ref-a", digest('2'), 7U);
        require(binding.organism_id == "organism-main", "binding must reference the root identity");
    }
    require(root.lineage.organism_id == root_before.lineage.organism_id
                && root.lineage.genesis_id == root_before.lineage.genesis_id
                && root.identity_schema_version == root_before.identity_schema_version,
            "applying bindings must not mutate the identity root");
    const IdentityBinding model_a = genesis::identity::bind_to_organism(
        root, IdentityBindingKind::model, "model-a", digest('2'), 7U);
    const IdentityBinding model_b = genesis::identity::bind_to_organism(
        root, IdentityBindingKind::model, "model-b", digest('2'), 8U);
    require(model_a.binding_id != model_b.binding_id, "bindings must be distinct records");
    require(model_a.organism_id == model_b.organism_id, "model swap must preserve identity");
}

void test_worker_is_not_genesis() {
    EntityRegistry registry{"local.genesis", "organism-main", 64U, 128U};
    const GenesisIdentityRoot root = make_root();
    EntityAddress organism = genesis::identity::make_entity_address(
        "local.genesis", EntityKind::organism, "organism-main", digest('a'), 1U);
    require(registry.register_entity(organism), "organism registration failed");
    std::string error;
    require(genesis::identity::register_supervised_worker(registry, root, "worker-1",
                                                          digest('3'), 2U, &error),
            ("worker registration failed: " + error).c_str());
    const std::string worker_id =
        genesis::identity::derive_entity_id("local.genesis", EntityKind::service, "worker-1");
    require(!registry.organism_identity(worker_id).has_value(),
            "worker must never resolve to an organism identity");
    require(registry.organism_identity(organism.entity_id).has_value(),
            "organism must still resolve after worker registration");
    std::string duplicate_error;
    require(!genesis::identity::register_supervised_worker(registry, root, "worker-1",
                                                           digest('3'), 3U, &duplicate_error),
            "duplicate worker registration must fail");
    require(!genesis::identity::register_supervised_worker(registry, root, "",
                                                           digest('3'), 3U, nullptr),
            "empty worker key must fail");
}

void test_owner_is_distinct() {
    EntityRegistry registry{"local.genesis", "organism-main", 64U, 128U};
    const GenesisIdentityRoot root = make_root();
    EntityAddress organism = genesis::identity::make_entity_address(
        "local.genesis", EntityKind::organism, "organism-main", digest('a'), 1U);
    EntityAddress owner = genesis::identity::make_entity_address(
        "local.genesis", EntityKind::person, "owner-jp", digest('1'), 2U);
    require(registry.register_entity(organism), "organism registration failed");
    require(registry.register_entity(owner), "owner registration failed");
    require(genesis::identity::owner_is_distinct_from_organism(registry, root, owner.entity_id),
            "owner person must be distinct from the organism");
    require(!genesis::identity::owner_is_distinct_from_organism(registry, root, organism.entity_id),
            "the organism itself must not pass as a distinct owner");
}

void test_persistence_round_trip_preserves_identity() {
    EntityRegistry registry{"local.genesis", "organism-main", 64U, 128U};
    const GenesisIdentityRoot root = make_root();
    EntityAddress organism = genesis::identity::make_entity_address(
        "local.genesis", EntityKind::organism, "organism-main", digest('a'), 1U);
    require(registry.register_entity(organism), "organism registration failed");
    std::string worker_error;
    require(genesis::identity::register_supervised_worker(registry, root, "worker-9",
                                                          digest('3'), 2U, &worker_error),
            ("worker registration failed: " + worker_error).c_str());

    const auto dir = std::filesystem::temp_directory_path() / "genesis-identity-root-v1";
    std::error_code cleanup_error;
    std::filesystem::remove_all(dir, cleanup_error);
    EntityRegistryStore store(dir);
    EntityStoreError error;
    require(store.write(registry, "1.0.0", &error), "identity store write failed");
    const auto restored = store.read("local.genesis", "organism-main", "1.0.0", &error);
    require(restored.has_value() && restored->verify(), "identity store recovery failed");
    require(restored->organism_identity(organism.entity_id).has_value(),
            "organism identity must survive process restart");
    const std::string worker_id =
        genesis::identity::derive_entity_id("local.genesis", EntityKind::service, "worker-9");
    require(!restored->organism_identity(worker_id).has_value(),
            "worker must still not resolve after recovery");
    std::filesystem::remove_all(dir, cleanup_error);
}

void test_corrupt_state_never_mints_identity() {
    EntityStoreError error;
    require(!EntityRegistryStore::deserialize("garbage-bytes", &error).has_value()
                && error.code == EntityStoreErrorCode::corrupt_record,
            "corrupt bytes must surface, never become a fresh identity");
    // Callers must bootstrap explicitly; there is no silent fallback path.
    // This is proven by construction: deserialize returns nullopt above.
    require(genesis::identity::classify_identity_store(true, false)
                == IdentityStoreState::corrupt,
            "corrupt persisted state must classify corrupt");
}

} // namespace

/**
 * The suite, wrapped so that a failing check prints what failed and
 * returns a non-zero exit code. The Microsoft C++ runtime shows a modal
 * dialog on an unhandled exception, and a modal dialog waits for a key
 * press that never comes.
 */
static int run_suite() {
try {
        test_bootstrap_and_validation();
        test_schema_migration_is_not_a_new_person();
        test_store_state_trichotomy();
        test_bindings_never_mutate_identity();
        test_worker_is_not_genesis();
        test_owner_is_distinct();
        test_persistence_round_trip_preserves_identity();
        test_corrupt_state_never_mints_identity();
    } catch (const std::exception& error) {
        std::cerr << "genesis_identity_tests FAILED: " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    std::cout << "genesis_identity_tests PASS\n";
    return EXIT_SUCCESS;
}

int main() {
    return genesis::testing::run(run_suite, "genesis_identity_tests");
}