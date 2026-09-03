#include "genesis/genesis.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main() {
    using namespace genesis::identity;
    constexpr std::size_t entity_total = 10'000U;
    constexpr std::size_t relation_total = 20'000U;

    EntityRegistry registry{
        "benchmark.identity", "benchmark-organism", entity_total, relation_total};
    std::vector<std::string> entity_ids;
    entity_ids.reserve(entity_total);

    const auto build_start = std::chrono::steady_clock::now();
    for (std::size_t index = 0U; index < entity_total; ++index) {
        const auto kind = index == 0U ? EntityKind::organism : EntityKind::component;
        const auto local_key = index == 0U ? std::string{"benchmark-organism"}
                                           : "component-" + std::to_string(index);
        auto entity = make_entity_address(
            "benchmark.identity",
            kind,
            local_key,
            genesis::runtime::sha256("entity-evidence-" + std::to_string(index)),
            static_cast<std::uint64_t>(index));
        entity_ids.push_back(entity.entity_id);
        if (!registry.register_entity(std::move(entity))) {
            std::cerr << "benchmark entity registration failed\n";
            return 1;
        }
    }

    for (std::size_t index = 0U; index < relation_total; ++index) {
        const auto subject_index = index % entity_total;
        auto object_index = (index * 7U + index / entity_total + 1U) % entity_total;
        if (object_index == subject_index) {
            object_index = (object_index + 1U) % entity_total;
        }
        auto relation = make_entity_relation(
            "benchmark.identity",
            RelationKind::associated_with,
            entity_ids[subject_index],
            entity_ids[object_index],
            static_cast<std::uint64_t>(index),
            static_cast<std::uint64_t>(entity_total + index),
            genesis::runtime::sha256("relation-evidence-" + std::to_string(index)));
        if (!registry.record_relation(std::move(relation))) {
            std::cerr << "benchmark relation registration failed at " << index << '\n';
            return 1;
        }
    }
    const auto build_end = std::chrono::steady_clock::now();

    const auto root = std::filesystem::temp_directory_path()
                      / "genesis-identity-registry-benchmark";
    std::error_code filesystem_error;
    std::filesystem::remove_all(root, filesystem_error);
    EntityRegistryStore store(root);
    EntityStoreError error;
    const auto snapshot = EntityRegistryStore::serialize(registry);
    const auto roundtrip_start = std::chrono::steady_clock::now();
    const auto written = store.write(registry, "1.0.0", &error);
    const auto restored = store.read(
        "benchmark.identity", "benchmark-organism", "1.0.0", &error);
    const auto roundtrip_end = std::chrono::steady_clock::now();

    const auto verified = written && restored.has_value() && restored->verify()
                          && restored->entity_count() == entity_total
                          && restored->relation_version_count() == relation_total
                          && EntityRegistryStore::serialize(*restored) == snapshot;
    std::cout << "identity_entities=" << entity_total
              << " relation_versions=" << relation_total
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
