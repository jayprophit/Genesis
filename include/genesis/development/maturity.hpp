#pragma once

#include <array>
#include <cstddef>

namespace genesis {

enum class Dimension : std::size_t {
    cognitive,
    linguistic,
    social,
    emotional,
    epistemic,
    operational,
    security,
    resource_management,
    tool_use,
    self_maintenance,
    planning,
    risk_assessment,
    world_model,
    self_model,
    count,
};

struct MaturityVector final {
    std::array<double, static_cast<std::size_t>(Dimension::count)> scores{};

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] double minimum() const noexcept;
};

[[nodiscard]] bool independence_gate(const MaturityVector& maturity) noexcept;

} // namespace genesis

