#include "genesis/organism/anatomy.hpp"

#include "genesis/runtime/runtime.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace genesis::organism {
namespace {

constexpr std::size_t maximum_text_length = 256;
constexpr std::size_t maximum_ports = 256;

void append_field(std::string& target, std::string_view value) {
    target.append(std::to_string(value.size()));
    target.push_back(':');
    target.append(value);
}

bool valid_text(std::string_view value) {
    if (value.empty() || value.size() > maximum_text_length) {
        return false;
    }
    return std::none_of(value.begin(), value.end(), [](unsigned char character) {
        return character < 0x20U || character == 0x7fU;
    });
}

bool valid_digest(std::string_view value) {
    return value.size() == 64U
        && std::all_of(value.begin(), value.end(), [](unsigned char character) {
               return (character >= '0' && character <= '9')
                   || (character >= 'a' && character <= 'f')
                   || (character >= 'A' && character <= 'F');
           });
}

bool valid_ratio(double value) {
    return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

void validate_string_vector(const std::vector<std::string>& values,
                            std::string_view field) {
    if (values.size() > maximum_ports) {
        throw std::invalid_argument(std::string(field) + " exceeds its item limit");
    }
    std::unordered_set<std::string> unique;
    for (const auto& value : values) {
        if (!valid_text(value)) {
            throw std::invalid_argument(std::string(field) + " contains invalid text");
        }
        if (!unique.insert(value).second) {
            throw std::invalid_argument(std::string(field) + " contains a duplicate");
        }
    }
}

std::string resource_material(const ResourceCost& cost) {
    std::string result;
    append_field(result, std::to_string(cost.compute_units));
    append_field(result, std::to_string(cost.memory_bytes));
    append_field(result, std::to_string(cost.io_units));
    append_field(result, std::to_string(cost.energy_units));
    return result;
}

bool add_quantity(std::uint64_t left, std::uint64_t right, std::uint64_t& result) {
    if (right > std::numeric_limits<std::uint64_t>::max() - left) {
        return false;
    }
    result = left + right;
    return true;
}

bool add_cost(ResourceCost& total, const ResourceCost& value) {
    ResourceCost candidate;
    if (!add_quantity(total.compute_units, value.compute_units, candidate.compute_units)
        || !add_quantity(total.memory_bytes, value.memory_bytes, candidate.memory_bytes)
        || !add_quantity(total.io_units, value.io_units, candidate.io_units)
        || !add_quantity(total.energy_units, value.energy_units, candidate.energy_units)) {
        return false;
    }
    total = candidate;
    return true;
}

std::string cell_seed(const CellSpec& spec) {
    std::string material{"genesis.organism.cell.v1"};
    append_field(material, spec.cell_id);
    append_field(material, spec.genetic_profile_digest);
    append_field(material, spec.expression_state);
    append_field(material, resource_material(spec.resource_cost));
    append_field(material, std::to_string(spec.activation));
    append_field(material, std::to_string(spec.confidence));
    append_field(material, std::to_string(spec.health));
    append_field(material, std::to_string(spec.age_cycles));
    append_field(material, spec.provenance_digest);
    for (const auto& value : spec.inputs) {
        append_field(material, value);
    }
    for (const auto& value : spec.outputs) {
        append_field(material, value);
    }
    for (const auto& value : spec.connections) {
        append_field(material, value);
    }
    return runtime::sha256(material);
}

std::string cell_transition_digest(const CellTransitionRecord& record) {
    std::string material{"genesis.organism.cell.transition.v1"};
    append_field(material, std::to_string(record.sequence));
    append_field(material, to_string(record.from));
    append_field(material, to_string(record.to));
    append_field(material, to_string(record.cell_type));
    append_field(material, std::to_string(record.health));
    append_field(material, record.expression_state);
    append_field(material, record.reason);
    append_field(material, record.previous_digest);
    return runtime::sha256(material);
}

bool allowed_transition(CellState from, CellState to, CellType type) {
    if (to == CellState::recycled && from != CellState::recycled) {
        return true;
    }
    switch (from) {
    case CellState::created:
        return to == CellState::immature;
    case CellState::immature:
        return to == CellState::differentiating || to == CellState::damaged;
    case CellState::differentiating:
        return (to == CellState::active && type != CellType::generic)
            || to == CellState::damaged;
    case CellState::active:
        return (to == CellState::specialized && type != CellType::generic)
            || to == CellState::quiescent || to == CellState::damaged
            || to == CellState::senescent;
    case CellState::specialized:
        return to == CellState::quiescent || to == CellState::damaged
            || to == CellState::senescent;
    case CellState::quiescent:
        return to == CellState::active
            || (to == CellState::specialized && type != CellType::generic)
            || to == CellState::damaged || to == CellState::senescent;
    case CellState::damaged:
        return to == CellState::repairing;
    case CellState::repairing:
        return to == CellState::active
            || (to == CellState::specialized && type != CellType::generic)
            || to == CellState::damaged;
    case CellState::senescent:
    case CellState::recycled:
        return false;
    }
    return false;
}

void clear_cell_error(CellError* error) {
    if (error != nullptr) {
        *error = {};
    }
}

void set_cell_error(CellError* error, CellErrorCode code, std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->message = std::move(message);
    }
}

void clear_anatomy_error(AnatomyError* error) {
    if (error != nullptr) {
        *error = {};
    }
}

void set_anatomy_error(AnatomyError* error,
                       AnatomyErrorCode code,
                       std::string member_id,
                       std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->member_id = std::move(member_id);
        error->message = std::move(message);
    }
}

bool active_member_state(CellState state) {
    return state == CellState::active || state == CellState::specialized
        || state == CellState::quiescent;
}

bool tissue_type_allowed(TissueType value, const std::vector<TissueType>& allowed) {
    return allowed.empty() || std::find(allowed.begin(), allowed.end(), value) != allowed.end();
}

bool cell_type_allowed(CellType value, const std::vector<CellType>& allowed) {
    return allowed.empty() || std::find(allowed.begin(), allowed.end(), value) != allowed.end();
}

std::string_view repair_stage_name(RepairStage stage) noexcept {
    constexpr std::array names{
        std::string_view{"DETECTED"}, std::string_view{"ISOLATED"},
        std::string_view{"DIAGNOSED"}, std::string_view{"REPAIR_APPLIED"},
        std::string_view{"REBUILT"}, std::string_view{"RESTORED"},
        std::string_view{"VERIFIED"}, std::string_view{"REINTEGRATED"},
        std::string_view{"RECYCLED"}, std::string_view{"FAILED"},
    };
    const auto index = static_cast<std::size_t>(stage);
    return index < names.size() ? names[index] : std::string_view{"UNKNOWN"};
}

bool repair_transition_allowed(RepairStage from, RepairStage to) {
    switch (from) {
    case RepairStage::detected:
        return to == RepairStage::isolated || to == RepairStage::failed;
    case RepairStage::isolated:
        return to == RepairStage::diagnosed || to == RepairStage::failed;
    case RepairStage::diagnosed:
        return to == RepairStage::repair_applied || to == RepairStage::rebuilt
            || to == RepairStage::restored || to == RepairStage::recycled
            || to == RepairStage::failed;
    case RepairStage::repair_applied:
    case RepairStage::rebuilt:
    case RepairStage::restored:
        return to == RepairStage::verified || to == RepairStage::failed;
    case RepairStage::verified:
        return to == RepairStage::reintegrated || to == RepairStage::recycled
            || to == RepairStage::failed;
    case RepairStage::reintegrated:
    case RepairStage::recycled:
    case RepairStage::failed:
        return false;
    }
    return false;
}

std::string repair_record_digest(const RepairRecord& record) {
    std::string material{"genesis.organism.repair.v1"};
    append_field(material, std::to_string(record.sequence));
    append_field(material, repair_stage_name(record.from));
    append_field(material, repair_stage_name(record.to));
    append_field(material, record.evidence);
    append_field(material, record.previous_digest);
    return runtime::sha256(material);
}

} // namespace

std::string_view to_string(CellState state) noexcept {
    constexpr std::array values{
        std::string_view{"CREATED"}, std::string_view{"IMMATURE"},
        std::string_view{"DIFFERENTIATING"}, std::string_view{"ACTIVE"},
        std::string_view{"SPECIALIZED"}, std::string_view{"QUIESCENT"},
        std::string_view{"DAMAGED"}, std::string_view{"REPAIRING"},
        std::string_view{"SENESCENT"}, std::string_view{"RECYCLED"},
    };
    const auto index = static_cast<std::size_t>(state);
    return index < values.size() ? values[index] : std::string_view{"UNKNOWN"};
}

std::string_view to_string(CellType type) noexcept {
    constexpr std::array values{
        std::string_view{"GENERIC"}, std::string_view{"NEURAL"},
        std::string_view{"MEMORY"}, std::string_view{"SENSORY"},
        std::string_view{"REASONING"}, std::string_view{"AFFECTIVE"},
        std::string_view{"MOTOR"}, std::string_view{"REGULATORY"},
        std::string_view{"IMMUNE"}, std::string_view{"COMMUNICATION"},
        std::string_view{"SUPPORT"},
    };
    const auto index = static_cast<std::size_t>(type);
    return index < values.size() ? values[index] : std::string_view{"UNKNOWN"};
}

bool fits_within(const ResourceCost& value, const ResourceCost& limit) noexcept {
    return value.compute_units <= limit.compute_units
        && value.memory_bytes <= limit.memory_bytes
        && value.io_units <= limit.io_units
        && value.energy_units <= limit.energy_units;
}

ComputationalCell::ComputationalCell(CellSpec spec)
    : spec_(std::move(spec)) {
    if (!valid_text(spec_.cell_id) || !valid_digest(spec_.genetic_profile_digest)
        || !valid_digest(spec_.provenance_digest)
        || !valid_text(spec_.expression_state)) {
        throw std::invalid_argument("cell specification identity or digest is invalid");
    }
    if (!valid_ratio(spec_.activation) || !valid_ratio(spec_.confidence)
        || !valid_ratio(spec_.health)) {
        throw std::invalid_argument("cell activation, confidence and health must be finite in [0,1]");
    }
    validate_string_vector(spec_.inputs, "cell inputs");
    validate_string_vector(spec_.outputs, "cell outputs");
    validate_string_vector(spec_.connections, "cell connections");
    initial_digest_ = cell_seed(spec_);
}

bool ComputationalCell::transition(CellState target,
                                   std::string reason,
                                   CellError* error) {
    return apply_transition(target, std::move(reason), error);
}

bool ComputationalCell::differentiate(CellType target,
                                      std::string expression_state,
                                      CellError* error) {
    clear_cell_error(error);
    if (state_ == CellState::recycled) {
        set_cell_error(error, CellErrorCode::terminal_state, "recycled cells cannot differentiate");
        return false;
    }
    if (state_ != CellState::differentiating || target == CellType::generic
        || !valid_text(expression_state)) {
        set_cell_error(error, CellErrorCode::invalid_type,
                       "differentiation requires differentiating state, a specialized type and expression state");
        return false;
    }
    const auto previous_type = type_;
    const auto previous_expression = spec_.expression_state;
    type_ = target;
    spec_.expression_state = std::move(expression_state);
    if (!apply_transition(CellState::active, "differentiation_complete", error)) {
        type_ = previous_type;
        spec_.expression_state = previous_expression;
        return false;
    }
    return true;
}

bool ComputationalCell::update_health(double health, CellError* error) {
    clear_cell_error(error);
    if (state_ == CellState::recycled) {
        set_cell_error(error, CellErrorCode::terminal_state, "recycled cell health is immutable");
        return false;
    }
    if (!valid_ratio(health)) {
        set_cell_error(error, CellErrorCode::invalid_health, "cell health must be finite in [0,1]");
        return false;
    }
    spec_.health = health;
    CellTransitionRecord record;
    record.sequence = audit_.size() + 1U;
    record.from = state_;
    record.to = state_;
    record.cell_type = type_;
    record.health = spec_.health;
    record.expression_state = spec_.expression_state;
    record.reason = "health_update";
    record.previous_digest = state_digest();
    record.digest = cell_transition_digest(record);
    audit_.push_back(std::move(record));
    return true;
}

bool ComputationalCell::recycle(std::string reason, CellError* error) {
    return apply_transition(CellState::recycled, std::move(reason), error);
}

const CellSpec& ComputationalCell::spec() const noexcept {
    return spec_;
}

CellState ComputationalCell::state() const noexcept {
    return state_;
}

CellType ComputationalCell::type() const noexcept {
    return type_;
}

std::string ComputationalCell::state_digest() const {
    return audit_.empty() ? initial_digest_ : audit_.back().digest;
}

const std::vector<CellTransitionRecord>& ComputationalCell::audit() const noexcept {
    return audit_;
}

bool ComputationalCell::verify() const {
    auto previous_digest = initial_digest_;
    auto current_state = CellState::created;
    auto current_type = CellType::generic;
    auto current_expression = audit_.empty() ? spec_.expression_state : std::string{};
    double current_health = audit_.empty() ? spec_.health : 1.0;
    for (std::size_t index = 0; index < audit_.size(); ++index) {
        const auto& record = audit_[index];
        if (record.sequence != index + 1U || record.from != current_state
            || record.previous_digest != previous_digest || !valid_ratio(record.health)
            || !valid_text(record.reason) || !valid_text(record.expression_state)) {
            return false;
        }
        const bool health_update = record.from == record.to && record.reason == "health_update";
        if (!health_update && !allowed_transition(record.from, record.to, record.cell_type)) {
            return false;
        }
        if (cell_transition_digest(record) != record.digest) {
            return false;
        }
        current_state = record.to;
        current_type = record.cell_type;
        current_expression = record.expression_state;
        current_health = record.health;
        previous_digest = record.digest;
    }
    return current_state == state_ && current_type == type_
        && current_expression == spec_.expression_state && current_health == spec_.health;
}

bool ComputationalCell::apply_transition(CellState target,
                                         std::string reason,
                                         CellError* error) {
    clear_cell_error(error);
    if (state_ == CellState::recycled) {
        set_cell_error(error, CellErrorCode::terminal_state, "recycled cell state is terminal");
        return false;
    }
    if (!valid_text(reason)) {
        set_cell_error(error, CellErrorCode::invalid_transition, "transition reason is invalid");
        return false;
    }
    if (!allowed_transition(state_, target, type_)) {
        set_cell_error(error, CellErrorCode::invalid_transition,
                       "cell lifecycle transition is not allowed");
        return false;
    }
    CellTransitionRecord record;
    record.sequence = audit_.size() + 1U;
    record.from = state_;
    record.to = target;
    record.cell_type = type_;
    record.health = spec_.health;
    record.expression_state = spec_.expression_state;
    record.reason = std::move(reason);
    record.previous_digest = state_digest();
    record.digest = cell_transition_digest(record);
    audit_.push_back(std::move(record));
    state_ = target;
    return true;
}

std::string_view to_string(TissueType type) noexcept {
    constexpr std::array values{
        std::string_view{"NEURAL"}, std::string_view{"MEMORY"},
        std::string_view{"SENSORY"}, std::string_view{"REASONING"},
        std::string_view{"AFFECTIVE"}, std::string_view{"MOTOR"},
        std::string_view{"REGULATORY"}, std::string_view{"IMMUNE"},
        std::string_view{"COMMUNICATION"}, std::string_view{"CONNECTIVE"},
    };
    const auto index = static_cast<std::size_t>(type);
    return index < values.size() ? values[index] : std::string_view{"UNKNOWN"};
}

Tissue::Tissue(TissueSpec spec)
    : spec_(std::move(spec)) {
    if (!valid_text(spec_.tissue_id) || !valid_text(spec_.interface_version)
        || spec_.cell_capacity == 0U || spec_.allowed_cell_types.size() > maximum_ports) {
        throw std::invalid_argument("tissue specification is invalid");
    }
    std::unordered_set<int> unique;
    for (const auto type : spec_.allowed_cell_types) {
        if (type == CellType::generic || to_string(type) == "UNKNOWN"
            || !unique.insert(static_cast<int>(type)).second) {
            throw std::invalid_argument("tissue allowed cell types are invalid");
        }
    }
}

bool Tissue::add_cell(const ComputationalCell& cell, AnatomyError* error) {
    clear_anatomy_error(error);
    if (contains(cell.spec().cell_id)) {
        set_anatomy_error(error, AnatomyErrorCode::duplicate_member, cell.spec().cell_id,
                          "cell already belongs to tissue");
        return false;
    }
    if (members_.size() >= spec_.cell_capacity) {
        set_anatomy_error(error, AnatomyErrorCode::capacity_exceeded, cell.spec().cell_id,
                          "tissue cell capacity exceeded");
        return false;
    }
    if (!active_member_state(cell.state())
        || !cell_type_allowed(cell.type(), spec_.allowed_cell_types)) {
        set_anatomy_error(error, AnatomyErrorCode::incompatible_member, cell.spec().cell_id,
                          "cell state or type is incompatible with tissue");
        return false;
    }
    auto usage = resource_usage();
    if (!add_cost(usage, cell.spec().resource_cost) || !fits_within(usage, spec_.resource_budget)) {
        set_anatomy_error(error, AnatomyErrorCode::resource_budget_exceeded, cell.spec().cell_id,
                          "cell would exceed tissue resource budget");
        return false;
    }
    members_.push_back({cell.spec().cell_id, cell.type(), cell.state(), cell.spec().resource_cost,
                        cell.spec().health, cell.state_digest()});
    return true;
}

bool Tissue::refresh_cell(const ComputationalCell& cell, AnatomyError* error) {
    clear_anatomy_error(error);
    const auto found = std::find_if(members_.begin(), members_.end(), [&](const auto& member) {
        return member.cell_id == cell.spec().cell_id;
    });
    if (found == members_.end()) {
        set_anatomy_error(error, AnatomyErrorCode::missing_member, cell.spec().cell_id,
                          "cell is not a tissue member");
        return false;
    }
    if (cell.state() == CellState::recycled
        || !cell_type_allowed(cell.type(), spec_.allowed_cell_types)) {
        set_anatomy_error(error, AnatomyErrorCode::unhealthy_member, cell.spec().cell_id,
                          "recycled or incompatible cell cannot remain in tissue");
        return false;
    }
    const auto previous = *found;
    found->cell_type = cell.type();
    found->state = cell.state();
    found->resource_cost = cell.spec().resource_cost;
    found->health = cell.spec().health;
    found->cell_state_digest = cell.state_digest();
    if (!verify()) {
        *found = previous;
        set_anatomy_error(error, AnatomyErrorCode::resource_budget_exceeded,
                          cell.spec().cell_id, "refreshed cell would invalidate tissue");
        return false;
    }
    return true;
}

bool Tissue::remove_cell(std::string_view cell_id, AnatomyError* error) {
    clear_anatomy_error(error);
    const auto found = std::find_if(members_.begin(), members_.end(), [&](const auto& member) {
        return member.cell_id == cell_id;
    });
    if (found == members_.end()) {
        set_anatomy_error(error, AnatomyErrorCode::missing_member, std::string(cell_id),
                          "cell is not a tissue member");
        return false;
    }
    members_.erase(found);
    return true;
}

const TissueSpec& Tissue::spec() const noexcept {
    return spec_;
}

const std::vector<TissueMember>& Tissue::members() const noexcept {
    return members_;
}

ResourceCost Tissue::resource_usage() const noexcept {
    ResourceCost total;
    for (const auto& member : members_) {
        if (!add_cost(total, member.resource_cost)) {
            return {std::numeric_limits<std::uint64_t>::max(),
                    std::numeric_limits<std::uint64_t>::max(),
                    std::numeric_limits<std::uint64_t>::max(),
                    std::numeric_limits<std::uint64_t>::max()};
        }
    }
    return total;
}

double Tissue::health() const noexcept {
    if (members_.empty()) {
        return 0.0;
    }
    double total = 0.0;
    for (const auto& member : members_) {
        total += member.health;
    }
    return total / static_cast<double>(members_.size());
}

bool Tissue::contains(std::string_view cell_id) const noexcept {
    return std::any_of(members_.begin(), members_.end(), [&](const auto& member) {
        return member.cell_id == cell_id;
    });
}

bool Tissue::verify() const {
    if (members_.size() > spec_.cell_capacity || !fits_within(resource_usage(), spec_.resource_budget)) {
        return false;
    }
    std::unordered_set<std::string> ids;
    for (const auto& member : members_) {
        if (!valid_text(member.cell_id) || !ids.insert(member.cell_id).second
            || !cell_type_allowed(member.cell_type, spec_.allowed_cell_types)
            || member.state == CellState::recycled || !valid_ratio(member.health)
            || !valid_digest(member.cell_state_digest)) {
            return false;
        }
    }
    return true;
}

Organ::Organ(OrganSpec spec)
    : spec_(std::move(spec)) {
    if (!valid_text(spec_.organ_id) || !valid_text(spec_.interface_version)
        || spec_.tissue_capacity == 0U || spec_.capabilities.empty()) {
        throw std::invalid_argument("organ specification is invalid");
    }
    validate_string_vector(spec_.inputs, "organ inputs");
    validate_string_vector(spec_.outputs, "organ outputs");
    validate_string_vector(spec_.dependencies, "organ dependencies");
    validate_string_vector(spec_.capabilities, "organ capabilities");
    if (spec_.allowed_tissue_types.size() > maximum_ports) {
        throw std::invalid_argument("organ allowed tissue types exceed item limit");
    }
    std::unordered_set<int> unique;
    for (const auto type : spec_.allowed_tissue_types) {
        if (to_string(type) == "UNKNOWN" || !unique.insert(static_cast<int>(type)).second) {
            throw std::invalid_argument("organ allowed tissue types are invalid");
        }
    }
}

bool Organ::add_tissue(const Tissue& tissue, AnatomyError* error) {
    clear_anatomy_error(error);
    const auto duplicate = std::any_of(tissues_.begin(), tissues_.end(), [&](const auto& member) {
        return member.tissue_id == tissue.spec().tissue_id;
    });
    if (duplicate) {
        set_anatomy_error(error, AnatomyErrorCode::duplicate_member, tissue.spec().tissue_id,
                          "tissue already belongs to organ");
        return false;
    }
    if (tissues_.size() >= spec_.tissue_capacity) {
        set_anatomy_error(error, AnatomyErrorCode::capacity_exceeded, tissue.spec().tissue_id,
                          "organ tissue capacity exceeded");
        return false;
    }
    if (tissue.members().empty()
        || !tissue_type_allowed(tissue.spec().tissue_type, spec_.allowed_tissue_types)) {
        set_anatomy_error(error, AnatomyErrorCode::incompatible_member, tissue.spec().tissue_id,
                          "empty or incompatible tissue cannot join organ");
        return false;
    }
    auto usage = resource_usage();
    if (!add_cost(usage, tissue.resource_usage()) || !fits_within(usage, spec_.resource_budget)) {
        set_anatomy_error(error, AnatomyErrorCode::resource_budget_exceeded,
                          tissue.spec().tissue_id, "tissue would exceed organ resource budget");
        return false;
    }
    tissues_.push_back({tissue.spec().tissue_id, tissue.spec().tissue_type,
                        tissue.resource_usage(), tissue.health()});
    return true;
}

bool Organ::refresh_tissue(const Tissue& tissue, AnatomyError* error) {
    clear_anatomy_error(error);
    const auto found = std::find_if(tissues_.begin(), tissues_.end(), [&](const auto& member) {
        return member.tissue_id == tissue.spec().tissue_id;
    });
    if (found == tissues_.end()) {
        set_anatomy_error(error, AnatomyErrorCode::missing_member, tissue.spec().tissue_id,
                          "tissue is not an organ member");
        return false;
    }
    if (tissue.members().empty()
        || !tissue_type_allowed(tissue.spec().tissue_type, spec_.allowed_tissue_types)) {
        set_anatomy_error(error, AnatomyErrorCode::unhealthy_member, tissue.spec().tissue_id,
                          "empty or incompatible tissue cannot remain in organ");
        return false;
    }
    const auto previous = *found;
    found->tissue_type = tissue.spec().tissue_type;
    found->resource_usage = tissue.resource_usage();
    found->health = tissue.health();
    if (!verify()) {
        *found = previous;
        set_anatomy_error(error, AnatomyErrorCode::resource_budget_exceeded,
                          tissue.spec().tissue_id, "refreshed tissue would invalidate organ");
        return false;
    }
    return true;
}

bool Organ::remove_tissue(std::string_view tissue_id, AnatomyError* error) {
    clear_anatomy_error(error);
    const auto found = std::find_if(tissues_.begin(), tissues_.end(), [&](const auto& member) {
        return member.tissue_id == tissue_id;
    });
    if (found == tissues_.end()) {
        set_anatomy_error(error, AnatomyErrorCode::missing_member, std::string(tissue_id),
                          "tissue is not an organ member");
        return false;
    }
    tissues_.erase(found);
    return true;
}

const OrganSpec& Organ::spec() const noexcept {
    return spec_;
}

const std::vector<OrganTissue>& Organ::tissues() const noexcept {
    return tissues_;
}

ResourceCost Organ::resource_usage() const noexcept {
    ResourceCost total;
    for (const auto& tissue : tissues_) {
        if (!add_cost(total, tissue.resource_usage)) {
            return {std::numeric_limits<std::uint64_t>::max(),
                    std::numeric_limits<std::uint64_t>::max(),
                    std::numeric_limits<std::uint64_t>::max(),
                    std::numeric_limits<std::uint64_t>::max()};
        }
    }
    return total;
}

double Organ::health() const noexcept {
    if (tissues_.empty()) {
        return 0.0;
    }
    double total = 0.0;
    for (const auto& tissue : tissues_) {
        total += tissue.health;
    }
    return total / static_cast<double>(tissues_.size());
}

bool Organ::verify() const {
    if (tissues_.size() > spec_.tissue_capacity || !fits_within(resource_usage(), spec_.resource_budget)) {
        return false;
    }
    std::unordered_set<std::string> ids;
    for (const auto& tissue : tissues_) {
        if (!valid_text(tissue.tissue_id) || !ids.insert(tissue.tissue_id).second
            || !tissue_type_allowed(tissue.tissue_type, spec_.allowed_tissue_types)
            || !valid_ratio(tissue.health)) {
            return false;
        }
    }
    return true;
}

RepairWorkflow::RepairWorkflow(std::string workflow_id,
                               std::string subject_id,
                               std::string detection_evidence)
    : workflow_id_(std::move(workflow_id)), subject_id_(std::move(subject_id)) {
    if (!valid_text(workflow_id_) || !valid_text(subject_id_)
        || !valid_text(detection_evidence)) {
        throw std::invalid_argument("repair workflow identity or evidence is invalid");
    }
    std::string material{"genesis.organism.repair.seed.v1"};
    append_field(material, workflow_id_);
    append_field(material, subject_id_);
    append_field(material, detection_evidence);
    initial_digest_ = runtime::sha256(material);
}

bool RepairWorkflow::advance(RepairStage target,
                             std::string evidence,
                             std::string* error) {
    if (error != nullptr) {
        error->clear();
    }
    if (terminal()) {
        if (error != nullptr) {
            *error = "repair workflow is terminal";
        }
        return false;
    }
    if (!valid_text(evidence) || !repair_transition_allowed(stage_, target)) {
        if (error != nullptr) {
            *error = "repair transition or evidence is invalid";
        }
        return false;
    }
    RepairRecord record;
    record.sequence = audit_.size() + 1U;
    record.from = stage_;
    record.to = target;
    record.evidence = std::move(evidence);
    record.previous_digest = audit_.empty() ? initial_digest_ : audit_.back().digest;
    record.digest = repair_record_digest(record);
    audit_.push_back(std::move(record));
    stage_ = target;
    return true;
}

RepairStage RepairWorkflow::stage() const noexcept {
    return stage_;
}

const std::vector<RepairRecord>& RepairWorkflow::audit() const noexcept {
    return audit_;
}

bool RepairWorkflow::terminal() const noexcept {
    return stage_ == RepairStage::reintegrated || stage_ == RepairStage::recycled
        || stage_ == RepairStage::failed;
}

bool RepairWorkflow::verify() const {
    auto current = RepairStage::detected;
    auto previous_digest = initial_digest_;
    for (std::size_t index = 0; index < audit_.size(); ++index) {
        const auto& record = audit_[index];
        if (record.sequence != index + 1U || record.from != current
            || record.previous_digest != previous_digest || !valid_text(record.evidence)
            || !repair_transition_allowed(record.from, record.to)
            || repair_record_digest(record) != record.digest) {
            return false;
        }
        current = record.to;
        previous_digest = record.digest;
    }
    return current == stage_;
}

} // namespace genesis::organism
