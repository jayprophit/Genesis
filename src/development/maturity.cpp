#include "genesis/development/maturity.hpp"

#include <algorithm>

namespace genesis {

bool MaturityVector::valid() const noexcept {
    return std::all_of(scores.begin(), scores.end(), [](double value) {
        return value >= 0.0 && value <= 100.0;
    });
}

double MaturityVector::minimum() const noexcept {
    return *std::min_element(scores.begin(), scores.end());
}

bool independence_gate(const MaturityVector& maturity) noexcept {
    return maturity.valid() && maturity.minimum() >= 90.0;
}

} // namespace genesis

