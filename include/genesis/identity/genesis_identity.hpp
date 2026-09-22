#pragma once

#include "genesis/identity/entity_registry.hpp"
#include "genesis/identity/lineage.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::identity {

// P22 identity rule: ONE persistent Genesis identity. Models, providers,
// sessions, projects, workers, avatars and interface modes are runtime
// bindings that must never mutate, replace or fork the identity root.

// Binding kinds reference the persistent root; none of them IS the root.
enum class IdentityBindingKind : std::uint8_t {
    model,
    provider,
    session,
    project,
    worker,
    avatar,
    interface_mode,
};

[[nodiscard]] std::string_view to_string(IdentityBindingKind kind) noexcept;

// A binding record: subordinate runtime association. Applying a binding
// takes the root by const reference, so binding can never mutate identity.
struct IdentityBinding final {
    std::string binding_id;
    IdentityBindingKind kind{IdentityBindingKind::session};
    std::string organism_id;
    std::string binding_ref;
    std::string provenance_digest;
    std::uint64_t bound_at{};

    [[nodiscard]] bool operator==(const IdentityBinding&) const = default;
};

// Persistent root. identity_schema_version versions the SCHEMA; it never
// versions the person: migrating v1 to v2 must not mint a new identity.
struct GenesisIdentityRoot final {
    // No operator== by design: LineageIdentity has no value equality, and
    // identity comparison must go through explicit organism/genesis id
    // checks, never an accidental whole-struct compare.
    LineageIdentity lineage;
    std::string identity_schema_version{"1"};
    std::string bootstrap_provenance_digest;
};

// Explicit bootstrap is the ONLY path that mints a root. Returns error
// strings on failure; never silently generates an identity elsewhere.
[[nodiscard]] std::vector<std::string> validate_identity_root(const GenesisIdentityRoot& root);
[[nodiscard]] GenesisIdentityRoot bootstrap_genesis_identity(std::string organism_id,
                                                            std::string genesis_id,
                                                            std::string lineage_id,
                                                            std::string birth_event_id,
                                                            std::string bootstrap_provenance_digest);

// Loading distinguishes uninitialized / ready / corrupt. Callers must
// handle each case explicitly; there is no silent "generate if missing".
enum class IdentityStoreState : std::uint8_t {
    uninitialized,
    ready,
    corrupt,
};

[[nodiscard]] IdentityStoreState classify_identity_store(bool record_exists, bool record_valid) noexcept;

// Bind a runtime association to the root. The root is const: binding output
// always references the same organism_id the caller passed in.
[[nodiscard]] IdentityBinding bind_to_organism(const GenesisIdentityRoot& root,
                                              IdentityBindingKind kind,
                                              std::string binding_ref,
                                              std::string provenance_digest,
                                              std::uint64_t bound_at);

// Worker rule: a worker is a service-kind entity supervised by the organism.
// Only organism-kind entities resolve via organism_identity(), so a worker
// can never BE Genesis. Returns false (with reason) when the supervisor is
// not an organism or the worker id is already an organism identity.
[[nodiscard]] bool register_supervised_worker(EntityRegistry& registry,
                                             const GenesisIdentityRoot& root,
                                             std::string_view worker_local_key,
                                             std::string_view provenance_digest,
                                             std::uint64_t registered_at,
                                             std::string* error_message = nullptr);

// Owner separation: an owner must be a person/account entity, never the
// organism itself. Returns true when candidate correctly does NOT resolve
// to the organism identity.
[[nodiscard]] bool owner_is_distinct_from_organism(const EntityRegistry& registry,
                                                  const GenesisIdentityRoot& root,
                                                  std::string_view owner_entity_id) noexcept;

} // namespace genesis::identity
