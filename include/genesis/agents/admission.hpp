#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::agents {

/**
 * The admission gate: what may be called, and what may be spent.
 *
 * The programme's rule is free-first, and the gap analysis says plainly
 * what is missing: "Nothing refuses a paid or unknown endpoint. The
 * policy is described in the handoff and enforced nowhere in the kernel
 * or the runtime."
 *
 * The rule, in order of precedence:
 *
 *  1. AN UNKNOWN COST IS REFUSED. A guard that admits what it does not
 *     understand is not a guard. An endpoint whose cost is not known is
 *     refused, and the reason says so.
 *  2. A FREE ENDPOINT IS ADMITTED. A local model or a self-hosted
 *     service costs nothing, so it is admitted without question, and it
 *     is the preferred answer for everything.
 *  3. A FREE ALLOWANCE IS SPENT BEFORE ANY MONEY. A metered endpoint is
 *     admitted while it has a free use left, and the allowance is
 *     consumed. Only when it is exhausted does money come into it.
 *  4. MONEY IS REFUSED UNLESS IT IS BUDGETED. Spending is off by
 *     default. A caller may turn it on, and it may only be turned on
 *     with a stated number of paid uses, which is a ceiling rather than
 *     an intention.
 *
 * The gate spends nothing itself. It records what it admitted, and the
 * spend is zero until a paid endpoint is admitted and the budget
 * allows it.
 *
 * Every refusal carries its reason. A refusal that cannot be explained
 * is a refusal that cannot be audited, and an un-auditable refusal is
 * indistinguishable from a bug.
 */

/// What an endpoint costs to use.
enum class CostClass {
    /// No charge at all: a local model, a self-hosted service.
    free,
    /// Charged per use, with a free allowance that may run out.
    metered,
    /// Charged, with no free allowance.
    paid,
    /// The cost is not known.
    unknown,
};

[[nodiscard]] std::string_view to_string(CostClass cost) noexcept;

/// A model endpoint that may be admitted.
struct ModelEndpoint final {
    /// The endpoint's own identifier.
    std::string endpoint_id;
    /// Who serves it.
    std::string provider;
    /// Which model it serves.
    std::string model;
    /// What it costs.
    CostClass cost{CostClass::unknown};
    /// Whether it runs on this machine.
    bool local{false};
    /// How many free uses remain, for a metered endpoint.
    std::uint64_t free_allowance{0};
};

/// Why an endpoint was refused.
enum class AdmissionError {
    /// The endpoint was admitted.
    none,
    /// There is no such endpoint.
    unknown_endpoint,
    /// The endpoint costs money and no free use remains.
    paid_endpoint,
    /// The cost of the endpoint is not known.
    unknown_cost,
    /// Spending is disabled and the endpoint is not free.
    spending_disabled,
    /// The endpoint's free allowance is exhausted.
    allowance_exhausted,
    /// The budget for paid use is spent.
    budget_exhausted,
};

[[nodiscard]] std::string_view to_string(AdmissionError error) noexcept;

/// The outcome of an admission request.
struct AdmissionDecision final {
    /// Whether the endpoint may be used.
    bool admitted{false};
    /// Why, when it may not.
    AdmissionError error{AdmissionError::none};
    /// Whether the use consumed a free allowance.
    bool consumes_allowance{false};
    /// Whether the use was a paid one.
    bool paid{false};
    /// A short explanation, for the record.
    std::string reason;
};

/// The gate that decides what may be called.
class AdmissionGate final {
public:
    /// A gate that admits free endpoints and refuses everything else.
    [[nodiscard]] static AdmissionGate free_first();

    /**
     * Register an endpoint.
     *
     * Returns false when the endpoint is malformed, or when it is
     * already registered: two endpoints with one identifier would make
     * every admission ambiguous, and an ambiguous admission is a spend
     * nobody can account for.
     */
    [[nodiscard]] bool register_endpoint(ModelEndpoint endpoint);

    /// The endpoint, or null.
    [[nodiscard]] const ModelEndpoint* find(std::string_view endpoint_id) const noexcept;

    /// Every registered endpoint.
    [[nodiscard]] std::vector<ModelEndpoint> endpoints() const;

    /// How many endpoints are registered.
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * Admit an endpoint for one use.
     *
     * The decision is the only thing the caller acts on. A caller that
     * receives an admission may use the endpoint; a caller that does not
     * must not, and the reason is in the decision.
     */
    [[nodiscard]] AdmissionDecision admit(std::string_view endpoint_id);

    /// Whether spending is permitted at all. It is not, by default.
    [[nodiscard]] bool spending_enabled() const noexcept;

    /// How many paid uses are permitted. Zero by default.
    [[nodiscard]] std::uint64_t paid_budget() const noexcept;

    /**
     * Permit spending, up to a stated number of paid uses.
     *
     * A ceiling rather than an intention: the number is what the gate
     * enforces, and the count of paid uses already admitted is not
     * reset by raising it.
     */
    void enable_spending(std::uint64_t maximum_paid_uses) noexcept;

    /// How many paid uses have been admitted.
    [[nodiscard]] std::uint64_t paid_uses() const noexcept;

    /// How many free uses have been admitted.
    [[nodiscard]] std::uint64_t free_uses() const noexcept;

    /// How much has been spent, in paid uses.
    [[nodiscard]] std::uint64_t spend() const noexcept;

private:
    /// endpoint id -> endpoint
    std::map<std::string, ModelEndpoint, std::less<>> endpoints_;
    /// How many paid uses are permitted.
    std::uint64_t paid_budget_{0};
    /// How many paid uses have been admitted.
    std::uint64_t paid_uses_{0};
    /// How many free uses have been admitted.
    std::uint64_t free_uses_{0};
};

} // namespace genesis::agents
