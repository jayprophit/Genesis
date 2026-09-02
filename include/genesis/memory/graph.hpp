#pragma once

#include "genesis/memory/origin.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::memory {

enum class MemoryKind { episode, fact, skill, pattern, concept_node, observation, prediction };
enum class ActivationState { available, dormant, archival, erasure_pending, hard_deleted };
enum class IdentityScope { organism_private, descendant_inherited, shared_validated, external_reference };

struct MemoryNode final {
    std::string id, owner_id, content_digest, provenance_digest, context;
    MemoryKind kind{MemoryKind::observation};
    Origin origin{Origin::direct_experience};
    IdentityScope scope{IdentityScope::organism_private};
    ActivationState state{ActivationState::available};
    double confidence{}, strength{1.0}, affect_valence{};
    std::uint64_t created_at{}, last_accessed{}, access_count{};
    std::vector<std::string> features;
};
struct MemoryEdge final { std::string from, to; double weight{}; std::string relation; };
struct ActivatedMemory final { std::string id; double score{}; };
struct OutcomePrediction final { std::string outcome; double probability{}, confidence{}; std::vector<std::string> evidence_ids; };

class MemoryGraph final {
public:
    MemoryGraph(std::string organism_id, std::size_t node_capacity, std::size_t edge_capacity);
    bool add(MemoryNode node, std::string* error = nullptr);
    bool connect(MemoryEdge edge, std::string* error = nullptr);
    [[nodiscard]] std::vector<ActivatedMemory> activate(const std::vector<std::string>& features, std::string_view context, std::size_t maximum) const;
    bool reinforce(std::string_view id, double amount, std::uint64_t now);
    bool decay(double declarative_rate, double procedural_rate);
    bool consolidate(std::string_view id, std::string* error = nullptr);
    [[nodiscard]] std::vector<OutcomePrediction> predict(const std::vector<std::string>& features, std::string_view context, std::size_t maximum) const;
    [[nodiscard]] const MemoryNode* find(std::string_view id) const noexcept;
    [[nodiscard]] bool verify() const;
    [[nodiscard]] std::size_t size() const noexcept;
private:
    std::string organism_id_;
    std::size_t node_capacity_{}, edge_capacity_{};
    std::map<std::string, MemoryNode> nodes_;
    std::vector<MemoryEdge> edges_;
};

} // namespace genesis::memory
