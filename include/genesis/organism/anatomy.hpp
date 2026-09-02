#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::organism {

enum class CellState {
    created,
    immature,
    differentiating,
    active,
    specialized,
    quiescent,
    damaged,
    repairing,
    senescent,
    recycled,
};

enum class CellType {
    generic,
    neural,
    memory,
    sensory,
    reasoning,
    affective,
    motor,
    regulatory,
    immune,
    communication,
    support,
};

[[nodiscard]] std::string_view to_string(CellState state) noexcept;
[[nodiscard]] std::string_view to_string(CellType type) noexcept;

struct ResourceCost final {
    std::uint64_t compute_units{};
    std::uint64_t memory_bytes{};
    std::uint64_t io_units{};
    std::uint64_t energy_units{};
};

[[nodiscard]] bool fits_within(const ResourceCost& value,
                               const ResourceCost& limit) noexcept;

struct CellSpec final {
    std::string cell_id;
    std::string genetic_profile_digest;
    std::string expression_state;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    std::vector<std::string> connections;
    ResourceCost resource_cost;
    double activation{};
    double confidence{};
    double health{1.0};
    std::uint64_t age_cycles{};
    std::string provenance_digest;
};

enum class CellErrorCode {
    none,
    invalid_spec,
    invalid_transition,
    invalid_type,
    invalid_health,
    terminal_state,
};

struct CellError final {
    CellErrorCode code{CellErrorCode::none};
    std::string message;
};

struct CellTransitionRecord final {
    std::uint64_t sequence{};
    CellState from{CellState::created};
    CellState to{CellState::created};
    CellType cell_type{CellType::generic};
    double health{1.0};
    std::string expression_state;
    std::string reason;
    std::string previous_digest;
    std::string digest;
};

class ComputationalCell final {
public:
    explicit ComputationalCell(CellSpec spec);

    ComputationalCell(const ComputationalCell&) = delete;
    ComputationalCell& operator=(const ComputationalCell&) = delete;
    ComputationalCell(ComputationalCell&&) noexcept = default;
    ComputationalCell& operator=(ComputationalCell&&) noexcept = default;

    [[nodiscard]] bool transition(CellState target,
                                  std::string reason,
                                  CellError* error = nullptr);
    [[nodiscard]] bool differentiate(CellType target,
                                     std::string expression_state,
                                     CellError* error = nullptr);
    [[nodiscard]] bool update_health(double health,
                                     CellError* error = nullptr);
    [[nodiscard]] bool recycle(std::string reason,
                               CellError* error = nullptr);

    [[nodiscard]] const CellSpec& spec() const noexcept;
    [[nodiscard]] CellState state() const noexcept;
    [[nodiscard]] CellType type() const noexcept;
    [[nodiscard]] std::string state_digest() const;
    [[nodiscard]] const std::vector<CellTransitionRecord>& audit() const noexcept;
    [[nodiscard]] bool verify() const;

private:
    [[nodiscard]] bool apply_transition(CellState target,
                                        std::string reason,
                                        CellError* error);

    CellSpec spec_;
    CellState state_{CellState::created};
    CellType type_{CellType::generic};
    std::string initial_digest_;
    std::vector<CellTransitionRecord> audit_;
};

enum class TissueType {
    neural,
    memory,
    sensory,
    reasoning,
    affective,
    motor,
    regulatory,
    immune,
    communication,
    connective,
};

[[nodiscard]] std::string_view to_string(TissueType type) noexcept;

struct TissueSpec final {
    std::string tissue_id;
    std::string interface_version;
    TissueType tissue_type{TissueType::connective};
    std::vector<CellType> allowed_cell_types;
    std::size_t cell_capacity{};
    ResourceCost resource_budget;
};

struct TissueMember final {
    std::string cell_id;
    CellType cell_type{CellType::generic};
    CellState state{CellState::created};
    ResourceCost resource_cost;
    double health{};
    std::string cell_state_digest;
};

enum class AnatomyErrorCode {
    none,
    invalid_spec,
    duplicate_member,
    missing_member,
    incompatible_member,
    capacity_exceeded,
    resource_budget_exceeded,
    unhealthy_member,
};

struct AnatomyError final {
    AnatomyErrorCode code{AnatomyErrorCode::none};
    std::string member_id;
    std::string message;
};

class Tissue final {
public:
    explicit Tissue(TissueSpec spec);

    [[nodiscard]] bool add_cell(const ComputationalCell& cell,
                                AnatomyError* error = nullptr);
    [[nodiscard]] bool refresh_cell(const ComputationalCell& cell,
                                    AnatomyError* error = nullptr);
    [[nodiscard]] bool remove_cell(std::string_view cell_id,
                                   AnatomyError* error = nullptr);

    [[nodiscard]] const TissueSpec& spec() const noexcept;
    [[nodiscard]] const std::vector<TissueMember>& members() const noexcept;
    [[nodiscard]] ResourceCost resource_usage() const noexcept;
    [[nodiscard]] double health() const noexcept;
    [[nodiscard]] bool contains(std::string_view cell_id) const noexcept;
    [[nodiscard]] bool verify() const;

private:
    TissueSpec spec_;
    std::vector<TissueMember> members_;
};

struct OrganSpec final {
    std::string organ_id;
    std::string interface_version;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    std::vector<std::string> dependencies;
    std::vector<std::string> capabilities;
    std::vector<TissueType> allowed_tissue_types;
    std::size_t tissue_capacity{};
    ResourceCost resource_budget;
};

struct OrganTissue final {
    std::string tissue_id;
    TissueType tissue_type{TissueType::connective};
    ResourceCost resource_usage;
    double health{};
};

class Organ final {
public:
    explicit Organ(OrganSpec spec);

    [[nodiscard]] bool add_tissue(const Tissue& tissue,
                                  AnatomyError* error = nullptr);
    [[nodiscard]] bool refresh_tissue(const Tissue& tissue,
                                      AnatomyError* error = nullptr);
    [[nodiscard]] bool remove_tissue(std::string_view tissue_id,
                                     AnatomyError* error = nullptr);

    [[nodiscard]] const OrganSpec& spec() const noexcept;
    [[nodiscard]] const std::vector<OrganTissue>& tissues() const noexcept;
    [[nodiscard]] ResourceCost resource_usage() const noexcept;
    [[nodiscard]] double health() const noexcept;
    [[nodiscard]] bool verify() const;

private:
    OrganSpec spec_;
    std::vector<OrganTissue> tissues_;
};

enum class RepairStage {
    detected,
    isolated,
    diagnosed,
    repair_applied,
    rebuilt,
    restored,
    verified,
    reintegrated,
    recycled,
    failed,
};

struct RepairRecord final {
    std::uint64_t sequence{};
    RepairStage from{RepairStage::detected};
    RepairStage to{RepairStage::detected};
    std::string evidence;
    std::string previous_digest;
    std::string digest;
};

class RepairWorkflow final {
public:
    RepairWorkflow(std::string workflow_id,
                   std::string subject_id,
                   std::string detection_evidence);

    [[nodiscard]] bool advance(RepairStage target,
                               std::string evidence,
                               std::string* error = nullptr);
    [[nodiscard]] RepairStage stage() const noexcept;
    [[nodiscard]] const std::vector<RepairRecord>& audit() const noexcept;
    [[nodiscard]] bool terminal() const noexcept;
    [[nodiscard]] bool verify() const;

private:
    std::string workflow_id_;
    std::string subject_id_;
    std::string initial_digest_;
    RepairStage stage_{RepairStage::detected};
    std::vector<RepairRecord> audit_;
};

} // namespace genesis::organism
