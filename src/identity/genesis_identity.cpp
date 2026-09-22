#include "genesis/identity/genesis_identity.hpp"

namespace genesis::identity {

std::string_view to_string(IdentityBindingKind kind) noexcept {
    switch (kind) {
        case IdentityBindingKind::model:
            return "model";
        case IdentityBindingKind::provider:
            return "provider";
        case IdentityBindingKind::session:
            return "session";
        case IdentityBindingKind::project:
            return "project";
        case IdentityBindingKind::worker:
            return "worker";
        case IdentityBindingKind::avatar:
            return "avatar";
        case IdentityBindingKind::interface_mode:
            return "interface_mode";
    }
    return "session";
}

std::vector<std::string> validate_identity_root(const GenesisIdentityRoot& root) {
    std::vector<std::string> errors = validate(root.lineage);
    if (root.identity_schema_version.empty()) {
        errors.emplace_back("identity_schema_version required");
    }
    if (root.bootstrap_provenance_digest.empty()) {
        errors.emplace_back("bootstrap_provenance_digest required");
    }
    return errors;
}

GenesisIdentityRoot bootstrap_genesis_identity(std::string organism_id,
                                              std::string genesis_id,
                                              std::string lineage_id,
                                              std::string birth_event_id,
                                              std::string bootstrap_provenance_digest) {
    GenesisIdentityRoot root;
    root.lineage.organism_id = std::move(organism_id);
    root.lineage.genesis_id = std::move(genesis_id);
    root.lineage.lineage_id = std::move(lineage_id);
    root.lineage.birth_event_id = std::move(birth_event_id);
    root.lineage.origin = OriginKind::genesis;
    root.identity_schema_version = "1";
    root.bootstrap_provenance_digest = std::move(bootstrap_provenance_digest);
    return root;
}

IdentityStoreState classify_identity_store(bool record_exists, bool record_valid) noexcept {
    if (!record_exists) {
        return IdentityStoreState::uninitialized;
    }
    return record_valid ? IdentityStoreState::ready : IdentityStoreState::corrupt;
}

IdentityBinding bind_to_organism(const GenesisIdentityRoot& root,
                                 IdentityBindingKind kind,
                                 std::string binding_ref,
                                 std::string provenance_digest,
                                 std::uint64_t bound_at) {
    IdentityBinding binding;
    binding.binding_id = root.lineage.organism_id + ":" + std::string(to_string(kind)) + ":"
        + binding_ref;
    binding.kind = kind;
    binding.organism_id = root.lineage.organism_id;
    binding.binding_ref = std::move(binding_ref);
    binding.provenance_digest = std::move(provenance_digest);
    binding.bound_at = bound_at;
    return binding;
}

bool register_supervised_worker(EntityRegistry& registry,
                                const GenesisIdentityRoot& root,
                                std::string_view worker_local_key,
                                std::string_view provenance_digest,
                                std::uint64_t registered_at,
                                std::string* error_message) {
    auto fail = [&](const std::string& message) {
        if (error_message != nullptr) {
            *error_message = message;
        }
        return false;
    };
    if (worker_local_key.empty()) {
        return fail("worker local key required");
    }
    const std::string& registry_namespace = registry.namespace_id();
    const std::string worker_id = derive_entity_id(registry_namespace, EntityKind::service,
                                                   worker_local_key);
    if (registry.organism_identity(worker_id).has_value()) {
        return fail("worker id already resolves to an organism identity");
    }
    EntityAddress worker = make_entity_address(registry_namespace, EntityKind::service,
                                               std::string(worker_local_key),
                                               std::string(provenance_digest), registered_at);
    EntityRegistryError error;
    if (!registry.register_entity(worker, &error)) {
        return fail("worker registration failed: " + error.message);
    }
    // The supervision subject is the organism ENTITY id (derived), never the
    // bare local key: relations reference registered entities.
    const std::string organism_entity_id =
        derive_entity_id(registry_namespace, EntityKind::organism, root.lineage.organism_id);
    EntityRelation supervision = make_entity_relation(
        registry_namespace, RelationKind::operates, organism_entity_id, worker.entity_id,
        registered_at, registered_at, std::string(provenance_digest));
    if (!registry.record_relation(supervision, &error)) {
        return fail("supervision relation failed: " + error.message);
    }
    // Postcondition: the worker must not resolve as an organism identity.
    if (registry.organism_identity(worker.entity_id).has_value()) {
        return fail("worker incorrectly resolves to an organism identity");
    }
    return true;
}

bool owner_is_distinct_from_organism(const EntityRegistry& registry,
                                     const GenesisIdentityRoot& root,
                                     std::string_view owner_entity_id) noexcept {
    const std::optional<std::string> resolved = registry.organism_identity(owner_entity_id);
    if (!resolved.has_value()) {
        return true;
    }
    return *resolved != root.lineage.organism_id;
}

} // namespace genesis::identity
