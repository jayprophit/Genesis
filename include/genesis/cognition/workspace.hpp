#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::cognition {

enum class ModelKind { world, self, other_agent };
struct Belief { std::string id, proposition, evidence_digest; ModelKind model{ModelKind::world}; double confidence{}, uncertainty{1.0}; std::uint64_t observed_at{}; };
class BeliefModel final {
public:
    BeliefModel(std::string owner_id, std::size_t capacity);
    bool update(Belief belief);
    [[nodiscard]] const Belief* find(std::string_view id) const noexcept;
    [[nodiscard]] std::vector<Belief> contradictions(std::string_view proposition) const;
private:
    std::string owner_id_; std::size_t capacity_{}; std::vector<Belief> beliefs_;
};

struct WorkspaceItem { std::string id, source, payload_digest; double salience{}, confidence{}; bool inhibited{}; std::uint64_t timestamp{}; };
struct IntrospectionReport { std::size_t candidates{}, admitted{}, inhibited{}; double mean_confidence{}, uncertainty{}; };
class ConsciousWorkspace final {
public:
    explicit ConsciousWorkspace(std::size_t capacity);
    bool submit(WorkspaceItem item);
    [[nodiscard]] std::vector<WorkspaceItem> focus() const;
    [[nodiscard]] IntrospectionReport introspect() const;
    void clear() noexcept;
private:
    std::size_t capacity_{}; std::vector<WorkspaceItem> items_;
};

} // namespace genesis::cognition
