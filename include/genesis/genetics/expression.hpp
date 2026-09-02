#pragma once

#include <string>

namespace genesis {

enum class RnaState { created, expressing, modified, transported, consumed, decayed, removed };

struct Expression final {
    std::string expression_id;
    std::string source_gene;
    std::string target;
    std::string provenance;
    double activation_strength{};
    double confidence{};
    RnaState state{RnaState::created};
};

[[nodiscard]] bool transition(Expression& expression, RnaState next_state) noexcept;

} // namespace genesis

