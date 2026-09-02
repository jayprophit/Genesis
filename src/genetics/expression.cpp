#include "genesis/genetics/expression.hpp"

namespace genesis {

bool transition(Expression& expression, RnaState next_state) noexcept {
    bool allowed = false;
    switch (expression.state) {
    case RnaState::created:
        allowed = next_state == RnaState::expressing || next_state == RnaState::removed;
        break;
    case RnaState::expressing:
        allowed = next_state == RnaState::modified || next_state == RnaState::transported
            || next_state == RnaState::consumed || next_state == RnaState::decayed;
        break;
    case RnaState::modified:
        allowed = next_state == RnaState::expressing || next_state == RnaState::transported
            || next_state == RnaState::decayed;
        break;
    case RnaState::transported:
        allowed = next_state == RnaState::expressing || next_state == RnaState::consumed
            || next_state == RnaState::decayed;
        break;
    case RnaState::consumed:
    case RnaState::decayed:
        allowed = next_state == RnaState::removed;
        break;
    case RnaState::removed:
        break;
    }
    if (allowed) {
        expression.state = next_state;
    }
    return allowed;
}

} // namespace genesis

