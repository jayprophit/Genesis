#include "genesis/agents/admission.hpp"

#include <utility>

namespace genesis::agents {

std::string_view to_string(CostClass cost) noexcept {
    switch (cost) {
        case CostClass::free:
            return "free";
        case CostClass::metered:
            return "metered";
        case CostClass::paid:
            return "paid";
        case CostClass::unknown:
            return "unknown";
    }
    return "unknown";
}

std::string_view to_string(AdmissionError error) noexcept {
    switch (error) {
        case AdmissionError::none:
            return "none";
        case AdmissionError::unknown_endpoint:
            return "unknown_endpoint";
        case AdmissionError::paid_endpoint:
            return "paid_endpoint";
        case AdmissionError::unknown_cost:
            return "unknown_cost";
        case AdmissionError::spending_disabled:
            return "spending_disabled";
        case AdmissionError::allowance_exhausted:
            return "allowance_exhausted";
        case AdmissionError::budget_exhausted:
            return "budget_exhausted";
    }
    return "none";
}

AdmissionGate AdmissionGate::free_first() {
    return AdmissionGate{};
}

bool AdmissionGate::register_endpoint(ModelEndpoint endpoint) {
    if (endpoint.endpoint_id.empty() || endpoint.endpoint_id.size() > 128) {
        return false;
    }
    if (endpoints_.find(endpoint.endpoint_id) != endpoints_.end()) {
        // Two endpoints with one identifier would make every admission
        // ambiguous, and an ambiguous admission is a spend nobody can
        // account for.
        return false;
    }
    endpoints_.emplace(endpoint.endpoint_id, std::move(endpoint));
    return true;
}

const ModelEndpoint* AdmissionGate::find(std::string_view endpoint_id) const noexcept {
    const auto found = endpoints_.find(endpoint_id);
    if (found == endpoints_.end()) {
        return nullptr;
    }
    return &found->second;
}

std::vector<ModelEndpoint> AdmissionGate::endpoints() const {
    std::vector<ModelEndpoint> out;
    out.reserve(endpoints_.size());
    for (const auto& entry : endpoints_) {
        out.push_back(entry.second);
    }
    return out;
}

std::size_t AdmissionGate::size() const noexcept {
    return endpoints_.size();
}

AdmissionDecision AdmissionGate::admit(std::string_view endpoint_id) {
    AdmissionDecision decision;

    const auto found = endpoints_.find(endpoint_id);
    if (found == endpoints_.end()) {
        decision.error = AdmissionError::unknown_endpoint;
        decision.reason = "there is no such endpoint";
        return decision;
    }
    ModelEndpoint& endpoint = found->second;

    if (endpoint.cost == CostClass::unknown) {
        // A guard that admits what it does not understand is not a guard.
        decision.error = AdmissionError::unknown_cost;
        decision.reason = "the cost of the endpoint is not known";
        return decision;
    }

    if (endpoint.cost == CostClass::free) {
        decision.admitted = true;
        decision.error = AdmissionError::none;
        decision.reason = "the endpoint is free";
        free_uses_ += 1;
        return decision;
    }

    if (endpoint.cost == CostClass::metered && endpoint.free_allowance > 0) {
        // The free allowance is spent before any money is.
        endpoint.free_allowance -= 1;
        decision.admitted = true;
        decision.error = AdmissionError::none;
        decision.consumes_allowance = true;
        decision.reason = "the endpoint has a free use left";
        free_uses_ += 1;
        return decision;
    }

    // Money. It is only permitted when it is budgeted.
    if (paid_budget_ == 0) {
        decision.error = endpoint.cost == CostClass::metered
                             ? AdmissionError::allowance_exhausted
                             : AdmissionError::paid_endpoint;
        decision.reason = "the endpoint costs money and spending is not permitted";
        return decision;
    }
    if (paid_uses_ >= paid_budget_) {
        decision.error = AdmissionError::budget_exhausted;
        decision.reason = "the budget for paid use is spent";
        return decision;
    }

    decision.admitted = true;
    decision.error = AdmissionError::none;
    decision.paid = true;
    decision.reason = "the endpoint costs money and the budget allows it";
    paid_uses_ += 1;
    return decision;
}

bool AdmissionGate::spending_enabled() const noexcept {
    return paid_budget_ > 0;
}

std::uint64_t AdmissionGate::paid_budget() const noexcept {
    return paid_budget_;
}

void AdmissionGate::enable_spending(std::uint64_t maximum_paid_uses) noexcept {
    // A ceiling, not an intention: raising the ceiling does not reset
    // the count of paid uses already admitted.
    paid_budget_ = maximum_paid_uses;
}

std::uint64_t AdmissionGate::paid_uses() const noexcept {
    return paid_uses_;
}

std::uint64_t AdmissionGate::free_uses() const noexcept {
    return free_uses_;
}

std::uint64_t AdmissionGate::spend() const noexcept {
    return paid_uses_;
}

} // namespace genesis::agents
