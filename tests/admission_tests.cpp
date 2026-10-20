#include "genesis/agents/admission.hpp"

#include "test_support.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using genesis::agents::AdmissionDecision;
using genesis::agents::AdmissionError;
using genesis::agents::AdmissionGate;
using genesis::agents::CostClass;
using genesis::agents::ModelEndpoint;

int& g_checks = genesis::testing::g_checks;

void require(bool condition, const char* message) {
    ++g_checks;
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_equal(AdmissionError actual, AdmissionError expected, const char* message) {
    ++g_checks;
    if (actual != expected) {
        std::cerr << "expected " << genesis::agents::to_string(expected) << " but got "
                  << genesis::agents::to_string(actual) << ": " << message << "\n";
        throw std::runtime_error(message);
    }
}

ModelEndpoint endpoint(const std::string& id, CostClass cost, bool local,
                        std::uint64_t allowance = 0) {
    ModelEndpoint e;
    e.endpoint_id = id;
    e.provider = "local";
    e.model = id;
    e.cost = cost;
    e.local = local;
    e.free_allowance = allowance;
    return e;
}

// ---------------------------------------------------------------------------
// Free first.
// ---------------------------------------------------------------------------

void test_a_free_endpoint_is_admitted() {
    AdmissionGate gate = AdmissionGate::free_first();
    require(gate.register_endpoint(endpoint("local-qwen", CostClass::free, true)),
            "the endpoint should be registered");
    const auto decision = gate.admit("local-qwen");
    require(decision.admitted, "a free endpoint should be admitted");
    require_equal(decision.error, AdmissionError::none, "the admission should be clean");
    require(!decision.paid, "a free endpoint is not a paid one");
    require(gate.free_uses() == 1, "the free use should be counted");
    require(gate.spend() == 0, "a free endpoint costs nothing");
}

void test_an_unknown_cost_is_refused() {
    AdmissionGate gate = AdmissionGate::free_first();
    require(gate.register_endpoint(endpoint("mystery", CostClass::unknown, false)),
            "the endpoint should be registered");
    const auto decision = gate.admit("mystery");
    require(!decision.admitted, "an endpoint of unknown cost must be refused");
    require_equal(decision.error, AdmissionError::unknown_cost, "the refusal must say why");
    require(!decision.reason.empty(), "a refusal must carry a reason");
}

void test_an_unknown_endpoint_is_refused() {
    AdmissionGate gate = AdmissionGate::free_first();
    const auto decision = gate.admit("nobody");
    require(!decision.admitted, "an unknown endpoint must be refused");
    require_equal(decision.error, AdmissionError::unknown_endpoint, "the refusal must say why");
}

void test_a_paid_endpoint_is_refused_by_default() {
    AdmissionGate gate = AdmissionGate::free_first();
    require(gate.register_endpoint(endpoint("paid-model", CostClass::paid, false)),
            "the endpoint should be registered");
    const auto decision = gate.admit("paid-model");
    require(!decision.admitted, "a paid endpoint must be refused by default");
    require_equal(decision.error, AdmissionError::paid_endpoint, "the refusal must say why");
    require(gate.spend() == 0, "nothing may be spent");
}

void test_spending_is_off_by_default() {
    AdmissionGate gate = AdmissionGate::free_first();
    require(!gate.spending_enabled(), "spending must be off by default");
    require(gate.paid_budget() == 0, "the budget must be zero by default");
    require(gate.spend() == 0, "the spend must be zero by default");
}

// ---------------------------------------------------------------------------
// The free allowance is spent before any money.
// ---------------------------------------------------------------------------

void test_a_free_allowance_is_spent_before_money() {
    AdmissionGate gate = AdmissionGate::free_first();
    require(gate.register_endpoint(endpoint("metered", CostClass::metered, false, 2)),
            "the endpoint should be registered");
    const auto first = gate.admit("metered");
    require(first.admitted, "the first use should be admitted");
    require(first.consumes_allowance, "it should consume the allowance");
    require(!first.paid, "it should not be a paid use");
    const auto second = gate.admit("metered");
    require(second.admitted, "the second use should be admitted");
    require(second.consumes_allowance, "it should consume the allowance");
    require(gate.spend() == 0, "the allowance is free, so nothing is spent");
}

void test_an_exhausted_allowance_is_refused() {
    AdmissionGate gate = AdmissionGate::free_first();
    require(gate.register_endpoint(endpoint("metered", CostClass::metered, false, 1)),
            "the endpoint should be registered");
    require(gate.admit("metered").admitted, "the first use should be admitted");
    const auto second = gate.admit("metered");
    require(!second.admitted, "an exhausted allowance must be refused");
    require_equal(second.error, AdmissionError::allowance_exhausted, "the refusal must say why");
    require(gate.spend() == 0, "nothing may be spent");
}

// ---------------------------------------------------------------------------
// Money is permitted only when it is budgeted.
// ---------------------------------------------------------------------------

void test_a_budget_permits_exactly_that_many_paid_uses() {
    AdmissionGate gate = AdmissionGate::free_first();
    require(gate.register_endpoint(endpoint("paid-model", CostClass::paid, false)),
            "the endpoint should be registered");
    gate.enable_spending(1);
    require(gate.spending_enabled(), "spending should now be permitted");
    const auto first = gate.admit("paid-model");
    require(first.admitted, "the first paid use should be admitted");
    require(first.paid, "it should be a paid use");
    require(gate.spend() == 1, "the spend should be counted");
    const auto second = gate.admit("paid-model");
    require(!second.admitted, "the second paid use must be refused");
    require_equal(second.error, AdmissionError::budget_exhausted, "the refusal must say why");
    require(gate.spend() == 1, "the spend must not exceed the budget");
}

void test_raising_the_budget_does_not_reset_the_count() {
    AdmissionGate gate = AdmissionGate::free_first();
    require(gate.register_endpoint(endpoint("paid-model", CostClass::paid, false)),
            "the endpoint should be registered");
    gate.enable_spending(1);
    require(gate.admit("paid-model").admitted, "the first paid use should be admitted");
    gate.enable_spending(5);
    require(gate.paid_uses() == 1, "raising the budget must not reset the count");
    require(gate.admit("paid-model").admitted, "the raised budget should allow another use");
    require(gate.spend() == 2, "the spend should be counted");
}

void test_a_free_endpoint_stays_free_when_money_is_allowed() {
    AdmissionGate gate = AdmissionGate::free_first();
    require(gate.register_endpoint(endpoint("local-qwen", CostClass::free, true)),
            "the endpoint should be registered");
    gate.enable_spending(10);
    const auto decision = gate.admit("local-qwen");
    require(decision.admitted, "a free endpoint should be admitted");
    require(!decision.paid, "a free endpoint is not a paid one");
    require(gate.spend() == 0, "a free endpoint costs nothing even when money is allowed");
}

// ---------------------------------------------------------------------------
// The registry itself.
// ---------------------------------------------------------------------------

void test_a_duplicate_endpoint_is_refused() {
    AdmissionGate gate = AdmissionGate::free_first();
    require(gate.register_endpoint(endpoint("local-qwen", CostClass::free, true)),
            "the first registration should be accepted");
    require(!gate.register_endpoint(endpoint("local-qwen", CostClass::paid, false)),
            "a duplicate endpoint must be refused");
    require(gate.size() == 1, "the gate must not have grown");
}

void test_a_malformed_endpoint_is_refused() {
    AdmissionGate gate = AdmissionGate::free_first();
    require(!gate.register_endpoint(endpoint("", CostClass::free, true)),
            "an endpoint with no identifier must be refused");
}

void test_the_spend_stays_zero_while_only_free_endpoints_are_used() {
    AdmissionGate gate = AdmissionGate::free_first();
    require(gate.register_endpoint(endpoint("local-a", CostClass::free, true)),
            "the endpoint should be registered");
    require(gate.register_endpoint(endpoint("local-b", CostClass::free, true)),
            "the endpoint should be registered");
    require(gate.register_endpoint(endpoint("metered", CostClass::metered, false, 1)),
            "the endpoint should be registered");
    for (int i = 0; i < 5; ++i) {
        require(gate.admit("local-a").admitted, "the free use should be admitted");
    }
    require(gate.admit("metered").admitted, "the free allowance should be admitted");
    require(gate.admit("metered").admitted == false, "the second use must be refused");
    require(gate.spend() == 0, "the spend must stay zero");
    require(gate.free_uses() == 6, "the free uses should be counted");
}

void test_the_endpoints_are_listed() {
    AdmissionGate gate = AdmissionGate::free_first();
    require(gate.register_endpoint(endpoint("local-a", CostClass::free, true)),
            "the endpoint should be registered");
    require(gate.register_endpoint(endpoint("local-b", CostClass::free, true)),
            "the endpoint should be registered");
    require(gate.endpoints().size() == 2, "both endpoints should be listed");
    require(gate.find("local-a") != nullptr, "the endpoint should be found");
    require(gate.find("nobody") == nullptr, "an unknown endpoint should not be found");
}

int run_suite() {
    test_a_free_endpoint_is_admitted();
    test_an_unknown_cost_is_refused();
    test_an_unknown_endpoint_is_refused();
    test_a_paid_endpoint_is_refused_by_default();
    test_spending_is_off_by_default();
    test_a_free_allowance_is_spent_before_money();
    test_an_exhausted_allowance_is_refused();
    test_a_budget_permits_exactly_that_many_paid_uses();
    test_raising_the_budget_does_not_reset_the_count();
    test_a_free_endpoint_stays_free_when_money_is_allowed();
    test_a_duplicate_endpoint_is_refused();
    test_a_malformed_endpoint_is_refused();
    test_the_spend_stays_zero_while_only_free_endpoints_are_used();
    test_the_endpoints_are_listed();
    return 0;
}

} // namespace

int main() {
    return genesis::testing::run(run_suite, "admission_tests");
}
