#include "genesis/genesis.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using genesis::identity::EntityAddress;
using genesis::identity::EntityKind;
using genesis::identity::EntityRegistry;
using genesis::identity::EntityRegistryError;
using genesis::identity::EntityRegistryErrorCode;
using genesis::identity::EntityRegistryStore;
using genesis::identity::EntityStoreError;
using genesis::identity::EntityStoreErrorCode;
using genesis::identity::RelationKind;
using genesis::identity::RelationState;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string digest(char character) {
    return std::string(64U, character);
}

struct Fixture final {
    EntityRegistry registry{"local.genesis", "organism-main", 64U, 128U};
    EntityAddress organism = genesis::identity::make_entity_address(
        "local.genesis", EntityKind::organism, "organism-main", digest('a'), 1U);
    EntityAddress shell = genesis::identity::make_entity_address(
        "local.genesis", EntityKind::shell, "shared-key", digest('b'), 2U);
    EntityAddress account = genesis::identity::make_entity_address(
        "local.genesis", EntityKind::account, "shared-key", digest('c'), 3U);
    EntityAddress credential = genesis::identity::make_entity_address(
        "local.genesis", EntityKind::credential, "credential-1", digest('d'), 4U);
    EntityAddress place = genesis::identity::make_entity_address(
        "local.genesis", EntityKind::place, "lab-1", digest('e'), 5U);
    EntityAddress organization = genesis::identity::make_entity_address(
        "local.genesis", EntityKind::organization, "issuer-1", digest('f'), 6U);

    Fixture() {
        require(registry.register_entity(organism), "organism registration failed");
        require(registry.register_entity(shell), "shell registration failed");
        require(registry.register_entity(account), "account registration failed");
        require(registry.register_entity(credential), "credential registration failed");
        require(registry.register_entity(place), "place registration failed");
        require(registry.register_entity(organization), "organization registration failed");
    }
};

void test_enum_codecs() {
    for (const auto kind : {EntityKind::organism,
                            EntityKind::shell,
                            EntityKind::component,
                            EntityKind::record,
                            EntityKind::place,
                            EntityKind::credential,
                            EntityKind::experiment,
                            EntityKind::account,
                            EntityKind::person,
                            EntityKind::organization,
                            EntityKind::dataset,
                            EntityKind::model,
                            EntityKind::service,
                            EntityKind::custom}) {
        EntityKind parsed{};
        require(genesis::identity::entity_kind_from_string(
                    genesis::identity::to_string(kind), parsed)
                    && parsed == kind,
                "entity kind codec failed");
    }
    for (const auto kind : {RelationKind::owns,
                            RelationKind::custodies,
                            RelationKind::operates,
                            RelationKind::embodies,
                            RelationKind::contains,
                            RelationKind::located_at,
                            RelationKind::issued_by,
                            RelationKind::subject_of,
                            RelationKind::derived_from,
                            RelationKind::supersedes,
                            RelationKind::associated_with,
                            RelationKind::custom}) {
        RelationKind parsed{};
        require(genesis::identity::relation_kind_from_string(
                    genesis::identity::to_string(kind), parsed)
                    && parsed == kind,
                "relation kind codec failed");
    }
    for (const auto state : {RelationState::active,
                             RelationState::ended,
                             RelationState::revoked,
                             RelationState::superseded}) {
        RelationState parsed{};
        require(genesis::identity::relation_state_from_string(
                    genesis::identity::to_string(state), parsed)
                    && parsed == state,
                "relation state codec failed");
    }
    EntityKind ignored_entity{};
    RelationKind ignored_relation{};
    RelationState ignored_state{};
    require(!genesis::identity::entity_kind_from_string("unknown", ignored_entity)
                && !genesis::identity::relation_kind_from_string("unknown", ignored_relation)
                && !genesis::identity::relation_state_from_string("unknown", ignored_state),
            "unknown enum name was accepted");
}

void test_typed_addresses_and_bounds() {
    Fixture fixture;
    EntityRegistryError error;

    require(fixture.organism.entity_id
                == genesis::identity::derive_entity_id(
                    "local.genesis", EntityKind::organism, "organism-main"),
            "entity derivation is not deterministic");
    require(fixture.shell.entity_id != fixture.account.entity_id,
            "different entity kinds collided for the same local key");
    require(fixture.registry.find_entity(fixture.shell.entity_id)->kind == EntityKind::shell,
            "registered entity kind changed");
    require(!fixture.registry.register_entity(fixture.shell, &error)
                && error.code == EntityRegistryErrorCode::duplicate_entity,
            "duplicate entity registration was accepted");

    auto retyped = fixture.shell;
    retyped.kind = EntityKind::organism;
    require(!fixture.registry.register_entity(retyped, &error)
                && error.code == EntityRegistryErrorCode::invalid_entity,
            "an entity identifier was rebound to another type");
    auto invalid_evidence = genesis::identity::make_entity_address(
        "local.genesis", EntityKind::record, "bad-evidence", digest('A'), 7U);
    require(!fixture.registry.register_entity(invalid_evidence, &error)
                && error.code == EntityRegistryErrorCode::invalid_entity,
            "non-canonical evidence digest was accepted");

    EntityRegistry bounded{"bounded", "organism-main", 1U, 1U};
    auto first = genesis::identity::make_entity_address(
        "bounded", EntityKind::record, "one", digest('1'), 1U);
    auto second = genesis::identity::make_entity_address(
        "bounded", EntityKind::record, "two", digest('2'), 2U);
    require(bounded.register_entity(first), "bounded registry rejected its first entity");
    require(!bounded.register_entity(second, &error)
                && error.code == EntityRegistryErrorCode::entity_capacity_exceeded,
            "entity capacity was not enforced");
    require(!bounded.verify(),
            "registry without its declared registrar organism verified as recoverable");
    bool excessive_capacity_rejected = false;
    try {
        EntityRegistry excessive{"bounded", "organism-main", 1'000'001U, 1U};
        static_cast<void>(excessive);
    } catch (const std::invalid_argument&) {
        excessive_capacity_rejected = true;
    }
    require(excessive_capacity_rejected,
            "unpersistable entity registry capacity was accepted");
    require(fixture.registry.verify(), "valid typed registry failed verification");
}

void test_versioned_relation_boundaries() {
    Fixture fixture;
    EntityRegistryError error;

    auto embodiment = genesis::identity::make_entity_relation(
        "local.genesis",
        RelationKind::embodies,
        fixture.organism.entity_id,
        fixture.shell.entity_id,
        10U,
        10U,
        digest('1'));
    require(fixture.registry.record_relation(embodiment, &error),
            "valid typed embodiment relation was rejected");
    require(fixture.registry.relation_count() == 1U
                && fixture.registry.relation_version_count() == 1U,
            "relation counts are incorrect");

    const auto shell_facts = fixture.registry.relations_for(fixture.shell.entity_id);
    require(shell_facts.facts.size() == 1U && !shell_facts.action_authorized
                && !shell_facts.organism_identity_reassigned,
            "a relation fact crossed the authorization or identity boundary");
    require(fixture.registry.organism_identity(fixture.organism.entity_id)
                    == std::optional<std::string>{"organism-main"}
                && !fixture.registry.organism_identity(fixture.shell.entity_id).has_value()
                && !fixture.registry.organism_identity(fixture.account.entity_id).has_value()
                && !fixture.registry.organism_identity(fixture.credential.entity_id).has_value(),
            "shell, account or credential replaced organism identity");

    auto ended = embodiment;
    ended.version = 2U;
    ended.state = RelationState::ended;
    ended.effective_until = 20U;
    ended.recorded_at = 21U;
    ended.evidence_digest = digest('2');
    require(fixture.registry.record_relation(ended, &error),
            "valid terminal relation revision was rejected");
    require(fixture.registry.latest_relation(embodiment.relation_id)->state
                == RelationState::ended
                && fixture.registry.relation_history(embodiment.relation_id).size() == 2U,
            "relation revision history was not preserved");

    auto rewrite = ended;
    rewrite.version = 3U;
    rewrite.state = RelationState::active;
    rewrite.effective_until.reset();
    rewrite.recorded_at = 22U;
    rewrite.evidence_digest = digest('3');
    require(!fixture.registry.record_relation(rewrite, &error)
                && error.code == EntityRegistryErrorCode::terminal_relation,
            "terminal relation history was rewritten");

    auto gap = genesis::identity::make_entity_relation(
        "local.genesis",
        RelationKind::owns,
        fixture.account.entity_id,
        fixture.shell.entity_id,
        30U,
        30U,
        digest('4'));
    gap.version = 2U;
    require(!fixture.registry.record_relation(gap, &error)
                && error.code == EntityRegistryErrorCode::relation_version_conflict,
            "relation history version gap was accepted");

    auto missing = genesis::identity::make_entity_relation(
        "local.genesis",
        RelationKind::associated_with,
        genesis::identity::derive_entity_id(
            "local.genesis", EntityKind::person, "not-registered"),
        fixture.account.entity_id,
        40U,
        40U,
        digest('5'));
    require(!fixture.registry.record_relation(missing, &error)
                && error.code == EntityRegistryErrorCode::missing_subject,
            "dangling relation subject was accepted");

    auto self_relation = embodiment;
    self_relation.object_id = self_relation.subject_id;
    require(!fixture.registry.record_relation(self_relation, &error)
                && error.code == EntityRegistryErrorCode::self_relation,
            "self relation was accepted");

    auto wrong_endpoints = genesis::identity::make_entity_relation(
        "local.genesis",
        RelationKind::embodies,
        fixture.account.entity_id,
        fixture.shell.entity_id,
        50U,
        50U,
        digest('6'));
    require(!fixture.registry.record_relation(wrong_endpoints, &error)
                && error.code == EntityRegistryErrorCode::endpoint_kind_mismatch,
            "typed relation accepted incompatible endpoint kinds");

    auto before_registration = genesis::identity::make_entity_relation(
        "local.genesis",
        RelationKind::associated_with,
        fixture.account.entity_id,
        fixture.credential.entity_id,
        0U,
        2U,
        digest('6'));
    require(!fixture.registry.record_relation(before_registration, &error)
                && error.code == EntityRegistryErrorCode::temporal_conflict,
            "relation recorded before endpoint registration was accepted");

    auto invalid_interval = genesis::identity::make_entity_relation(
        "local.genesis",
        RelationKind::issued_by,
        fixture.credential.entity_id,
        fixture.organization.entity_id,
        60U,
        60U,
        digest('7'));
    invalid_interval.state = RelationState::revoked;
    invalid_interval.effective_until = 60U;
    require(!fixture.registry.record_relation(invalid_interval, &error)
                && error.code == EntityRegistryErrorCode::invalid_relation,
            "empty or reversed relation interval was accepted");

    auto location = genesis::identity::make_entity_relation(
        "local.genesis",
        RelationKind::located_at,
        fixture.shell.entity_id,
        fixture.place.entity_id,
        70U,
        70U,
        digest('8'));
    require(fixture.registry.record_relation(location),
            "valid place-typed relation was rejected");

    EntityRegistry bounded{"relation.bound", "bound-owner", 2U, 1U};
    auto bound_owner = genesis::identity::make_entity_address(
        "relation.bound", EntityKind::organism, "bound-owner", digest('a'), 1U);
    auto bound_record = genesis::identity::make_entity_address(
        "relation.bound", EntityKind::record, "bound-record", digest('b'), 1U);
    require(bounded.register_entity(bound_owner) && bounded.register_entity(bound_record),
            "relation-capacity fixture registration failed");
    auto first_relation = genesis::identity::make_entity_relation(
        "relation.bound",
        RelationKind::associated_with,
        bound_owner.entity_id,
        bound_record.entity_id,
        1U,
        1U,
        digest('c'));
    auto second_relation = genesis::identity::make_entity_relation(
        "relation.bound",
        RelationKind::owns,
        bound_owner.entity_id,
        bound_record.entity_id,
        2U,
        2U,
        digest('d'));
    require(bounded.record_relation(first_relation),
            "relation-capacity fixture rejected first relation");
    require(!bounded.record_relation(second_relation, &error)
                && error.code == EntityRegistryErrorCode::relation_capacity_exceeded,
            "relation-version capacity was not enforced");
    require(fixture.registry.verify(), "versioned relation registry failed verification");
}

void test_persistence_and_recovery() {
    Fixture fixture;
    auto embodiment = genesis::identity::make_entity_relation(
        "local.genesis",
        RelationKind::embodies,
        fixture.organism.entity_id,
        fixture.shell.entity_id,
        10U,
        10U,
        digest('1'));
    require(fixture.registry.record_relation(embodiment), "fixture relation failed");
    auto ended = embodiment;
    ended.version = 2U;
    ended.state = RelationState::ended;
    ended.effective_until = 20U;
    ended.recorded_at = 21U;
    ended.evidence_digest = digest('2');
    require(fixture.registry.record_relation(ended), "fixture terminal revision failed");

    EntityStoreError error;
    const auto bytes = EntityRegistryStore::serialize(fixture.registry);
    const auto decoded = EntityRegistryStore::deserialize(bytes, &error);
    require(decoded.has_value() && error.code == EntityStoreErrorCode::none
                && decoded->verify()
                && EntityRegistryStore::serialize(*decoded) == bytes
                && decoded->relation_history(embodiment.relation_id) ==
                       fixture.registry.relation_history(embodiment.relation_id),
            "entity registry serialization round trip changed state");

    auto corrupt = bytes;
    corrupt[corrupt.size() / 2U] ^= 1;
    require(!EntityRegistryStore::deserialize(corrupt, &error).has_value()
                && error.code == EntityStoreErrorCode::corrupt_record,
            "corrupt entity registry record was accepted");

    auto future = bytes;
    future[std::string_view{"GENESIS-ENTITY-REGISTRY"}.size()] = 2;
    future.replace(future.size() - 64U,
                   64U,
                   genesis::runtime::sha256(
                       std::string_view{future}.substr(0U, future.size() - 64U)));
    require(!EntityRegistryStore::deserialize(future, &error).has_value()
                && error.code == EntityStoreErrorCode::unsupported_schema,
            "unsupported entity registry schema was accepted");

    const auto root = std::filesystem::temp_directory_path()
                      / "genesis-entity-registry-store-v1";
    std::error_code cleanup_error;
    std::filesystem::remove_all(root, cleanup_error);
    EntityRegistryStore store(root);
    require(store.write(fixture.registry, "1.0.0", &error)
                && store.write(fixture.registry, "1.0.0", &error),
            "entity registry immutable idempotent write failed");
    const auto restored = store.read("local.genesis", "organism-main", "1.0.0", &error);
    require(restored.has_value() && restored->verify()
                && EntityRegistryStore::serialize(*restored) == bytes,
            "entity registry file recovery changed state");
    require(!store.read("local.genesis", "other-organism", "1.0.0", &error).has_value()
                && error.code == EntityStoreErrorCode::corrupt_record,
            "entity registry registrar mismatch was accepted");
    require(!store.read("../escape", "organism-main", "1.0.0", &error).has_value()
                && error.code == EntityStoreErrorCode::invalid_identifier,
            "unsafe entity registry path identifier was accepted");
    require(!store.read("local.genesis", "organism\nmain", "1.0.0", &error).has_value()
                && error.code == EntityStoreErrorCode::invalid_identifier,
            "unsafe entity registry registrar identifier was accepted");
    require(!store.read("missing", "organism-main", "1.0.0", &error).has_value()
                && error.code == EntityStoreErrorCode::not_found,
            "missing entity registry version did not report not-found");

    const auto tiny_root = root / "tiny";
    EntityRegistryStore tiny_store(tiny_root, 1024U);
    require(bytes.size() > 1024U
                && !tiny_store.write(fixture.registry, "1.0.0", &error)
                && error.code == EntityStoreErrorCode::invalid_registry,
            "configured entity registry record-size bound was not enforced");

    auto conflicting = fixture.registry;
    auto extra = genesis::identity::make_entity_address(
        "local.genesis", EntityKind::record, "conflict", digest('9'), 100U);
    require(conflicting.register_entity(extra), "conflict fixture registration failed");
    require(!store.write(conflicting, "1.0.0", &error)
                && error.code == EntityStoreErrorCode::conflicting_version,
            "conflicting immutable entity registry version was accepted");

    std::filesystem::create_directories(root);
    {
        std::ofstream wrong_namespace(root / "other.1.0.0.entities", std::ios::binary);
        wrong_namespace.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    require(!store.read("other", "organism-main", "1.0.0", &error).has_value()
                && error.code == EntityStoreErrorCode::corrupt_record,
            "entity registry namespace binding mismatch was accepted");

    std::filesystem::remove_all(root, cleanup_error);
}

} // namespace

int main() {
    try {
        test_enum_codecs();
        test_typed_addresses_and_bounds();
        test_versioned_relation_boundaries();
        test_persistence_and_recovery();
        std::cout << "Genesis identity registry tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& exception) {
        std::cerr << "FAIL: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }
}
