#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace genesis::runtime {

using Sequence = std::uint64_t;
using LogicalTime = std::uint64_t;
using SubscriptionId = std::uint64_t;
using ReservationId = std::uint64_t;
using ResourceQuantity = std::uint64_t;

// Returns a lowercase, 64-character SHA-256 digest.
[[nodiscard]] std::string sha256(std::string_view value);

// A deterministic, process-local clock. It never reads wall-clock time.
class LogicalClock final {
public:
    explicit LogicalClock(LogicalTime initial_value = 0) noexcept;

    LogicalClock(const LogicalClock&) = delete;
    LogicalClock& operator=(const LogicalClock&) = delete;

    [[nodiscard]] LogicalTime value() const;
    [[nodiscard]] LogicalTime tick();
    [[nodiscard]] LogicalTime advance_to(LogicalTime value);

private:
    mutable std::mutex mutex_;
    LogicalTime value_;
};

// An envelope has no mutators and cannot be assigned after construction.
// Payload digests are SHA-256 values; the payload itself deliberately remains
// outside the deterministic runtime boundary.
class EventEnvelope final {
public:
    EventEnvelope(Sequence sequence,
                  LogicalTime logical_time,
                  std::string event_id,
                  std::string source_id,
                  std::string topic,
                  std::optional<std::string> causal_parent_id,
                  std::string payload_digest);

    EventEnvelope(const EventEnvelope&) = default;
    EventEnvelope(EventEnvelope&&) noexcept = default;
    EventEnvelope& operator=(const EventEnvelope&) = delete;
    EventEnvelope& operator=(EventEnvelope&&) = delete;

    [[nodiscard]] Sequence sequence() const noexcept;
    [[nodiscard]] LogicalTime logical_time() const noexcept;
    [[nodiscard]] const std::string& event_id() const noexcept;
    [[nodiscard]] const std::string& source_id() const noexcept;
    [[nodiscard]] const std::string& topic() const noexcept;
    [[nodiscard]] const std::optional<std::string>& causal_parent_id() const noexcept;
    [[nodiscard]] const std::string& payload_digest() const noexcept;
    [[nodiscard]] const std::string& envelope_digest() const noexcept;
    [[nodiscard]] std::string canonical_form() const;

    [[nodiscard]] bool operator==(const EventEnvelope& other) const noexcept;

private:
    Sequence sequence_;
    LogicalTime logical_time_;
    std::string event_id_;
    std::string source_id_;
    std::string topic_;
    std::optional<std::string> causal_parent_id_;
    std::string payload_digest_;
    std::string envelope_digest_;
};

struct EventDraft final {
    std::string event_id;
    std::string source_id;
    std::string topic;
    std::optional<std::string> causal_parent_id;
    std::string payload_digest;
};

enum class HandlerStatus {
    succeeded,
    failed,
};

struct HandlerOutcome final {
    SubscriptionId subscription_id{};
    HandlerStatus status{HandlerStatus::succeeded};
    std::string error;
};

enum class DispatchStatus {
    unhandled,
    succeeded,
    partially_failed,
    failed,
};

struct DispatchOutcome final {
    EventEnvelope event;
    std::vector<HandlerOutcome> handlers;
    DispatchStatus status{DispatchStatus::unhandled};

    [[nodiscard]] bool handled() const noexcept;
    [[nodiscard]] bool successful() const noexcept;
    [[nodiscard]] std::size_t failure_count() const noexcept;
};

struct SubscriptionOptions final {
    bool one_shot{false};
};

class DeterministicDispatcher final {
public:
    using Handler = std::function<void(const EventEnvelope&)>;

    explicit DeterministicDispatcher(std::size_t history_capacity = 1024,
                                     Sequence first_sequence = 1,
                                     LogicalTime initial_logical_time = 0);

    DeterministicDispatcher(const DeterministicDispatcher&) = delete;
    DeterministicDispatcher& operator=(const DeterministicDispatcher&) = delete;

    [[nodiscard]] SubscriptionId subscribe(std::string topic,
                                           Handler handler,
                                           SubscriptionOptions options = {});
    [[nodiscard]] bool unsubscribe(SubscriptionId subscription_id);

    // Handlers run in ascending subscription-ID order. Handler exceptions are
    // contained and represented in the returned outcome.
    [[nodiscard]] DispatchOutcome publish(EventDraft draft);

    // Replay preserves sequence, logical time, IDs, and causal links. The full
    // batch is structurally validated before the first handler is called.
    [[nodiscard]] std::vector<DispatchOutcome> replay(
        std::span<const EventEnvelope> events);

    [[nodiscard]] std::vector<DispatchOutcome> history_snapshot() const;
    [[nodiscard]] std::size_t history_size() const;
    [[nodiscard]] std::size_t history_capacity() const;
    [[nodiscard]] std::uint64_t history_eviction_count() const;
    void set_history_capacity(std::size_t capacity);

    [[nodiscard]] Sequence next_sequence() const;
    [[nodiscard]] LogicalTime logical_time() const;
    [[nodiscard]] bool has_seen(std::string_view event_id) const;

private:
    struct Subscription final {
        std::string topic;
        Handler handler;
        bool one_shot{};
    };

    [[nodiscard]] DispatchOutcome dispatch_validated(const EventEnvelope& event);
    void validate_causal_parent(const EventEnvelope& event) const;

    mutable std::mutex dispatch_mutex_;
    mutable std::mutex state_mutex_;
    LogicalClock clock_;
    Sequence next_sequence_;
    SubscriptionId next_subscription_id_{1};
    std::map<SubscriptionId, Subscription> subscriptions_;
    std::set<std::string, std::less<>> seen_event_ids_;
    std::deque<DispatchOutcome> history_;
    std::size_t history_capacity_;
    std::uint64_t history_evictions_{};
};

struct ResourceSnapshot final {
    ResourceQuantity capacity{};
    ResourceQuantity current{};
    ResourceQuantity reserved{};
    ResourceQuantity peak{};

    [[nodiscard]] ResourceQuantity available() const noexcept;
};

using ResourceRequest = std::map<std::string, ResourceQuantity, std::less<>>;
using ResourceSnapshotMap = std::map<std::string, ResourceSnapshot, std::less<>>;

enum class ReservationErrorCode {
    none,
    empty_request,
    invalid_quantity,
    unknown_resource,
    insufficient_capacity,
    identifier_exhausted,
    capacity_below_usage,
};

struct ReservationError final {
    ReservationErrorCode code{ReservationErrorCode::none};
    std::string resource;
    ResourceQuantity requested{};
    ResourceQuantity available{};
    std::string message;
};

enum class ReservationState {
    invalid,
    reserved,
    committed,
    released,
};

namespace detail {
struct ResourcePoolState;
}

// A move-only reservation. Destruction releases both pending and committed
// resources, making every successful reservation an exception-safe scope.
class ResourceReservation final {
public:
    ResourceReservation() noexcept = default;
    ~ResourceReservation() noexcept;

    ResourceReservation(const ResourceReservation&) = delete;
    ResourceReservation& operator=(const ResourceReservation&) = delete;
    ResourceReservation(ResourceReservation&& other) noexcept;
    ResourceReservation& operator=(ResourceReservation&& other) noexcept;

    [[nodiscard]] ReservationId id() const noexcept;
    [[nodiscard]] ReservationState state() const noexcept;
    [[nodiscard]] bool commit();
    [[nodiscard]] bool rollback();
    [[nodiscard]] bool release();
    [[nodiscard]] explicit operator bool() const noexcept;

private:
    friend class ResourceAccounts;
    ResourceReservation(std::shared_ptr<detail::ResourcePoolState> state,
                        ReservationId id) noexcept;
    void release_noexcept() noexcept;

    std::shared_ptr<detail::ResourcePoolState> state_;
    ReservationId id_{};
    ReservationState terminal_state_{ReservationState::invalid};
};

class ResourceAccounts final {
public:
    explicit ResourceAccounts(ResourceRequest capacities);

    ResourceAccounts(const ResourceAccounts&) = delete;
    ResourceAccounts& operator=(const ResourceAccounts&) = delete;
    ResourceAccounts(ResourceAccounts&&) noexcept = default;
    ResourceAccounts& operator=(ResourceAccounts&&) noexcept = default;

    [[nodiscard]] std::optional<ResourceReservation> try_reserve(
        const ResourceRequest& request,
        ReservationError* error = nullptr);
    [[nodiscard]] bool set_capacity(std::string_view resource,
                                    ResourceQuantity capacity,
                                    ReservationError* error = nullptr);
    [[nodiscard]] std::optional<ResourceSnapshot> snapshot(
        std::string_view resource) const;
    [[nodiscard]] ResourceSnapshotMap snapshot_all() const;
    [[nodiscard]] std::size_t active_reservations() const;

private:
    std::shared_ptr<detail::ResourcePoolState> state_;
};

struct StateTransitionRule final {
    std::string from_state;
    std::string event_topic;
    std::string to_state;
};

enum class StateTransitionStatus {
    applied,
    sequence_mismatch,
    causal_mismatch,
    duplicate_event,
    transition_not_allowed,
    sequence_exhausted,
};

struct StateTransitionOutcome final {
    StateTransitionStatus status{StateTransitionStatus::transition_not_allowed};
    Sequence event_sequence{};
    std::string event_id;
    std::string from_state;
    std::string to_state;
    std::string message;

    [[nodiscard]] bool applied() const noexcept;
};

struct StateTransitionRecord final {
    Sequence event_sequence{};
    std::string event_id;
    std::string event_topic;
    std::optional<std::string> causal_parent_id;
    std::string from_state;
    std::string to_state;
    std::string event_envelope_digest;
    std::string previous_digest;
    std::string digest;
};

// This is intentionally minimal: one deterministic rule per (state, topic),
// strict sequence continuity, and a direct causal-parent chain.
class CausalStateMachine final {
public:
    CausalStateMachine(std::string initial_state,
                       std::vector<StateTransitionRule> rules,
                       Sequence first_sequence = 1);

    CausalStateMachine(const CausalStateMachine&) = delete;
    CausalStateMachine& operator=(const CausalStateMachine&) = delete;

    [[nodiscard]] StateTransitionOutcome apply(const EventEnvelope& event);
    [[nodiscard]] std::vector<StateTransitionOutcome> replay(
        std::span<const EventEnvelope> events);

    [[nodiscard]] std::string current_state() const;
    [[nodiscard]] Sequence next_sequence() const;
    [[nodiscard]] std::string replay_digest() const;
    [[nodiscard]] std::vector<StateTransitionRecord> history_snapshot() const;
    [[nodiscard]] bool verify() const;

private:
    [[nodiscard]] StateTransitionOutcome apply_locked(const EventEnvelope& event);

    mutable std::mutex mutex_;
    std::string initial_state_;
    std::string current_state_;
    Sequence first_sequence_;
    Sequence next_sequence_;
    std::map<std::pair<std::string, std::string>, std::string> rules_;
    std::set<std::string, std::less<>> seen_event_ids_;
    std::optional<std::string> last_event_id_;
    std::string replay_digest_;
    std::vector<StateTransitionRecord> history_;
};

} // namespace genesis::runtime
