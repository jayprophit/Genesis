#include "genesis/identity/lineage.hpp"

namespace genesis {

std::vector<std::string> validate(const LineageIdentity& identity) {
    std::vector<std::string> errors;
    if (identity.organism_id.empty()) {
        errors.emplace_back("organism_id required");
    }
    if (identity.genesis_id.empty()) {
        errors.emplace_back("genesis_id required");
    }
    if (identity.lineage_id.empty()) {
        errors.emplace_back("lineage_id required");
    }
    if (identity.birth_event_id.empty()) {
        errors.emplace_back("birth_event_id required");
    }
    if (identity.origin == OriginKind::child
        && (identity.parent_a_id.empty() || identity.parent_b_id.empty())) {
        errors.emplace_back("child requires two parents");
    }
    if (identity.origin == OriginKind::child && identity.generation == 0) {
        errors.emplace_back("child generation must be positive");
    }
    if (identity.origin == OriginKind::child
        && identity.parent_a_id == identity.parent_b_id) {
        errors.emplace_back("child parents must be distinct identities");
    }
    return errors;
}

bool represents_continuation(OriginKind kind) noexcept {
    return kind == OriginKind::genesis || kind == OriginKind::restore;
}

} // namespace genesis

