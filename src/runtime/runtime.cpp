#include "genesis/runtime/runtime.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <unordered_map>

namespace genesis::runtime {
namespace {

constexpr std::size_t kMaximumIdentifierLength = 256;
constexpr std::size_t kMaximumTopicLength = 256;
constexpr std::size_t kMaximumResourceNameLength = 128;

void validate_text(std::string_view value,
                   std::string_view field,
                   std::size_t maximum_length) {
    if (value.empty()) {
        throw std::invalid_argument(std::string(field) + " cannot be empty");
    }
    if (value.size() > maximum_length) {
        throw std::invalid_argument(std::string(field) + " exceeds its length limit");
    }
    const auto control = std::find_if(value.begin(), value.end(), [](unsigned char character) {
        return character < 0x20U || character == 0x7fU;
    });
    if (control != value.end()) {
        throw std::invalid_argument(std::string(field) + " contains a control character");
    }
}

std::string normalize_digest(std::string value) {
    if (value.size() != 64) {
        throw std::invalid_argument("payload_digest must be a 64-character SHA-256 value");
    }
    for (char& character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (std::isxdigit(byte) == 0) {
            throw std::invalid_argument("payload_digest contains a non-hexadecimal character");
        }
        character = static_cast<char>(std::tolower(byte));
    }
    return value;
}

void append_field(std::string& target, std::string_view value) {
    target.append(std::to_string(value.size()));
    target.push_back(':');
    target.append(value);
}

std::string event_material(Sequence sequence,
                           LogicalTime logical_time,
                           std::string_view event_id,
                           std::string_view source_id,
                           std::string_view topic,
                           const std::optional<std::string>& causal_parent_id,
                           std::string_view payload_digest) {
    std::string result{"genesis.runtime.event.v1"};
    append_field(result, std::to_string(sequence));
    append_field(result, std::to_string(logical_time));
    append_field(result, event_id);
    append_field(result, source_id);
    append_field(result, topic);
    result.push_back(causal_parent_id.has_value() ? '1' : '0');
    append_field(result, causal_parent_id.value_or(""));
    append_field(result, payload_digest);
    return result;
}

std::string state_seed(std::string_view initial_state, Sequence first_sequence) {
    std::string material{"genesis.runtime.state.v1"};
    append_field(material, initial_state);
    append_field(material, std::to_string(first_sequence));
    return sha256(material);
}

std::string transition_digest(const StateTransitionRecord& record) {
    std::string material{"genesis.runtime.transition.v1"};
    append_field(material, std::to_string(record.event_sequence));
    append_field(material, record.event_id);
    append_field(material, record.event_topic);
    material.push_back(record.causal_parent_id.has_value() ? '1' : '0');
    append_field(material, record.causal_parent_id.value_or(""));
    append_field(material, record.from_state);
    append_field(material, record.to_state);
    append_field(material, record.event_envelope_digest);
    append_field(material, record.previous_digest);
    return sha256(material);
}

thread_local std::vector<const DeterministicDispatcher*> dispatch_stack;

class DispatchScope final {
public:
    explicit DispatchScope(const DeterministicDispatcher* dispatcher)
        : dispatcher_(dispatcher) {
        if (std::find(dispatch_stack.begin(), dispatch_stack.end(), dispatcher_) !=
            dispatch_stack.end()) {
            throw std::logic_error("reentrant publishing on the same dispatcher is not supported");
        }
        dispatch_stack.push_back(dispatcher_);
    }

    ~DispatchScope() {
        dispatch_stack.pop_back();
    }

    DispatchScope(const DispatchScope&) = delete;
    DispatchScope& operator=(const DispatchScope&) = delete;

private:
    const DeterministicDispatcher* dispatcher_;
};

void clear_error(ReservationError* error) {
    if (error != nullptr) {
        *error = {};
    }
}

void set_error(ReservationError* error,
               ReservationErrorCode code,
               std::string resource,
               ResourceQuantity requested,
               ResourceQuantity available,
               std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->resource = std::move(resource);
        error->requested = requested;
        error->available = available;
        error->message = std::move(message);
    }
}

} // namespace

std::string sha256(std::string_view value) {
    static constexpr std::array<std::uint32_t, 64> round_constants{
        0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
        0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
        0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
        0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
        0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
        0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
        0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
        0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
        0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
        0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
        0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
        0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
        0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
        0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
        0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
        0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
    };

    if (value.size() > std::numeric_limits<std::uint64_t>::max() / 8U) {
        throw std::length_error("input is too large for SHA-256");
    }

    std::vector<std::uint8_t> message(value.begin(), value.end());
    const auto bit_length = static_cast<std::uint64_t>(value.size()) * 8U;
    message.push_back(0x80U);
    while ((message.size() % 64U) != 56U) {
        message.push_back(0U);
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
        message.push_back(static_cast<std::uint8_t>(bit_length >> shift));
    }

    std::array<std::uint32_t, 8> hash{
        0x6a09e667U,
        0xbb67ae85U,
        0x3c6ef372U,
        0xa54ff53aU,
        0x510e527fU,
        0x9b05688cU,
        0x1f83d9abU,
        0x5be0cd19U,
    };

    for (std::size_t offset = 0; offset < message.size(); offset += 64U) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t index = 0; index < 16U; ++index) {
            const auto base = offset + index * 4U;
            words[index] = (static_cast<std::uint32_t>(message[base]) << 24U) |
                           (static_cast<std::uint32_t>(message[base + 1U]) << 16U) |
                           (static_cast<std::uint32_t>(message[base + 2U]) << 8U) |
                           static_cast<std::uint32_t>(message[base + 3U]);
        }
        for (std::size_t index = 16U; index < words.size(); ++index) {
            const auto sigma0 = std::rotr(words[index - 15U], 7) ^
                                std::rotr(words[index - 15U], 18) ^
                                (words[index - 15U] >> 3U);
            const auto sigma1 = std::rotr(words[index - 2U], 17) ^
                                std::rotr(words[index - 2U], 19) ^
                                (words[index - 2U] >> 10U);
            words[index] = words[index - 16U] + sigma0 + words[index - 7U] + sigma1;
        }

        auto a = hash[0];
        auto b = hash[1];
        auto c = hash[2];
        auto d = hash[3];
        auto e = hash[4];
        auto f = hash[5];
        auto g = hash[6];
        auto h = hash[7];

        for (std::size_t index = 0; index < words.size(); ++index) {
            const auto sum1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
            const auto choose = (e & f) ^ ((~e) & g);
            const auto temporary1 = h + sum1 + choose + round_constants[index] + words[index];
            const auto sum0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
            const auto majority = (a & b) ^ (a & c) ^ (b & c);
            const auto temporary2 = sum0 + majority;

            h = g;
            g = f;
            f = e;
            e = d + temporary1;
            d = c;
            c = b;
            b = a;
            a = temporary1 + temporary2;
        }

        hash[0] += a;
        hash[1] += b;
        hash[2] += c;
        hash[3] += d;
        hash[4] += e;
        hash[5] += f;
        hash[6] += g;
        hash[7] += h;
    }

    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (const auto word : hash) {
        result << std::setw(8) << word;
    }
    return result.str();
}

LogicalClock::LogicalClock(LogicalTime initial_value) noexcept
    : value_(initial_value) {}

LogicalTime LogicalClock::value() const {
    const std::lock_guard lock(mutex_);
    return value_;
}

LogicalTime LogicalClock::tick() {
    const std::lock_guard lock(mutex_);
    if (value_ == std::numeric_limits<LogicalTime>::max()) {
        throw std::overflow_error("logical clock is exhausted");
    }
    return ++value_;
}

LogicalTime LogicalClock::advance_to(LogicalTime value) {
    const std::lock_guard lock(mutex_);
    if (value <= value_) {
        throw std::invalid_argument("logical clock can only advance");
    }
    value_ = value;
    return value_;
}

EventEnvelope::EventEnvelope(Sequence sequence,
                             LogicalTime logical_time,
                             std::string event_id,
                             std::string source_id,
                             std::string topic,
                             std::optional<std::string> causal_parent_id,
                             std::string payload_digest)
    : sequence_(sequence),
      logical_time_(logical_time),
      event_id_(std::move(event_id)),
      source_id_(std::move(source_id)),
      topic_(std::move(topic)),
      causal_parent_id_(std::move(causal_parent_id)),
      payload_digest_(normalize_digest(std::move(payload_digest))) {
    validate_text(event_id_, "event_id", kMaximumIdentifierLength);
    validate_text(source_id_, "source_id", kMaximumIdentifierLength);
    validate_text(topic_, "topic", kMaximumTopicLength);
    if (causal_parent_id_.has_value()) {
        validate_text(*causal_parent_id_, "causal_parent_id", kMaximumIdentifierLength);
        if (*causal_parent_id_ == event_id_) {
            throw std::invalid_argument("an event cannot be its own causal parent");
        }
    }
    envelope_digest_ = sha256(canonical_form());
}

Sequence EventEnvelope::sequence() const noexcept {
    return sequence_;
}

LogicalTime EventEnvelope::logical_time() const noexcept {
    return logical_time_;
}

const std::string& EventEnvelope::event_id() const noexcept {
    return event_id_;
}

const std::string& EventEnvelope::source_id() const noexcept {
    return source_id_;
}

const std::string& EventEnvelope::topic() const noexcept {
    return topic_;
}

const std::optional<std::string>& EventEnvelope::causal_parent_id() const noexcept {
    return causal_parent_id_;
}

const std::string& EventEnvelope::payload_digest() const noexcept {
    return payload_digest_;
}

const std::string& EventEnvelope::envelope_digest() const noexcept {
    return envelope_digest_;
}

std::string EventEnvelope::canonical_form() const {
    return event_material(sequence_,
                          logical_time_,
                          event_id_,
                          source_id_,
                          topic_,
                          causal_parent_id_,
                          payload_digest_);
}

bool EventEnvelope::operator==(const EventEnvelope& other) const noexcept {
    return std::tie(sequence_,
                    logical_time_,
                    event_id_,
                    source_id_,
                    topic_,
                    causal_parent_id_,
                    payload_digest_,
                    envelope_digest_) ==
           std::tie(other.sequence_,
                    other.logical_time_,
                    other.event_id_,
                    other.source_id_,
                    other.topic_,
                    other.causal_parent_id_,
                    other.payload_digest_,
                    other.envelope_digest_);
}

bool DispatchOutcome::handled() const noexcept {
    return !handlers.empty();
}

bool DispatchOutcome::successful() const noexcept {
    return status == DispatchStatus::succeeded;
}

std::size_t DispatchOutcome::failure_count() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        handlers.begin(), handlers.end(), [](const HandlerOutcome& outcome) {
            return outcome.status == HandlerStatus::failed;
        }));
}

DeterministicDispatcher::DeterministicDispatcher(std::size_t history_capacity,
                                                 Sequence first_sequence,
                                                 LogicalTime initial_logical_time)
    : clock_(initial_logical_time),
      next_sequence_(first_sequence),
      history_capacity_(history_capacity) {
    if (first_sequence == 0 || first_sequence == std::numeric_limits<Sequence>::max()) {
        throw std::invalid_argument("first_sequence must be within the usable sequence range");
    }
}

SubscriptionId DeterministicDispatcher::subscribe(std::string topic,
                                                  Handler handler,
                                                  SubscriptionOptions options) {
    validate_text(topic, "subscription topic", kMaximumTopicLength);
    if (!handler) {
        throw std::invalid_argument("subscription handler cannot be empty");
    }

    const std::lock_guard lock(state_mutex_);
    if (next_subscription_id_ == std::numeric_limits<SubscriptionId>::max()) {
        throw std::overflow_error("subscription identifiers are exhausted");
    }
    const auto id = next_subscription_id_++;
    subscriptions_.emplace(
        id, Subscription{std::move(topic), std::move(handler), options.one_shot});
    return id;
}

bool DeterministicDispatcher::unsubscribe(SubscriptionId subscription_id) {
    const std::lock_guard lock(state_mutex_);
    return subscriptions_.erase(subscription_id) != 0;
}

DispatchOutcome DeterministicDispatcher::publish(EventDraft draft) {
    DispatchScope dispatch_scope(this);
    const std::lock_guard dispatch_lock(dispatch_mutex_);

    if (next_sequence_ == std::numeric_limits<Sequence>::max()) {
        throw std::overflow_error("event sequences are exhausted");
    }
    const auto current_time = clock_.value();
    if (current_time == std::numeric_limits<LogicalTime>::max()) {
        throw std::overflow_error("logical clock is exhausted");
    }

    EventEnvelope event(next_sequence_,
                        current_time + 1,
                        std::move(draft.event_id),
                        std::move(draft.source_id),
                        std::move(draft.topic),
                        std::move(draft.causal_parent_id),
                        std::move(draft.payload_digest));
    validate_causal_parent(event);
    if (!seen_event_ids_.insert(event.event_id()).second) {
        throw std::invalid_argument("event_id has already been dispatched");
    }

    static_cast<void>(clock_.advance_to(event.logical_time()));
    ++next_sequence_;
    return dispatch_validated(event);
}

std::vector<DispatchOutcome> DeterministicDispatcher::replay(
    std::span<const EventEnvelope> events) {
    DispatchScope dispatch_scope(this);
    const std::lock_guard dispatch_lock(dispatch_mutex_);

    auto expected_sequence = next_sequence_;
    auto previous_time = clock_.value();
    auto known_event_ids = seen_event_ids_;

    for (const auto& event : events) {
        if (expected_sequence == std::numeric_limits<Sequence>::max()) {
            throw std::overflow_error("replay would exhaust event sequences");
        }
        if (event.sequence() != expected_sequence) {
            throw std::invalid_argument("replay event sequence is not contiguous");
        }
        if (event.logical_time() <= previous_time) {
            throw std::invalid_argument("replay logical time is not strictly increasing");
        }
        if (known_event_ids.contains(event.event_id())) {
            throw std::invalid_argument("replay contains a duplicate event_id");
        }
        if (event.causal_parent_id().has_value() &&
            !known_event_ids.contains(*event.causal_parent_id())) {
            throw std::invalid_argument("replay causal parent has not been observed");
        }
        known_event_ids.insert(event.event_id());
        previous_time = event.logical_time();
        ++expected_sequence;
    }

    std::vector<DispatchOutcome> outcomes;
    outcomes.reserve(events.size());
    for (const auto& event : events) {
        seen_event_ids_.insert(event.event_id());
        static_cast<void>(clock_.advance_to(event.logical_time()));
        next_sequence_ = event.sequence() + 1;
        outcomes.push_back(dispatch_validated(event));
    }
    return outcomes;
}

std::vector<DispatchOutcome> DeterministicDispatcher::history_snapshot() const {
    const std::lock_guard lock(state_mutex_);
    return {history_.begin(), history_.end()};
}

std::size_t DeterministicDispatcher::history_size() const {
    const std::lock_guard lock(state_mutex_);
    return history_.size();
}

std::size_t DeterministicDispatcher::history_capacity() const {
    const std::lock_guard lock(state_mutex_);
    return history_capacity_;
}

std::uint64_t DeterministicDispatcher::history_eviction_count() const {
    const std::lock_guard lock(state_mutex_);
    return history_evictions_;
}

void DeterministicDispatcher::set_history_capacity(std::size_t capacity) {
    const std::lock_guard lock(state_mutex_);
    history_capacity_ = capacity;
    while (history_.size() > history_capacity_) {
        history_.pop_front();
        ++history_evictions_;
    }
}

Sequence DeterministicDispatcher::next_sequence() const {
    const std::lock_guard lock(dispatch_mutex_);
    return next_sequence_;
}

LogicalTime DeterministicDispatcher::logical_time() const {
    const std::lock_guard lock(dispatch_mutex_);
    return clock_.value();
}

bool DeterministicDispatcher::has_seen(std::string_view event_id) const {
    const std::lock_guard lock(dispatch_mutex_);
    return seen_event_ids_.contains(event_id);
}

DispatchOutcome DeterministicDispatcher::dispatch_validated(const EventEnvelope& event) {
    std::vector<std::pair<SubscriptionId, Handler>> handlers;
    std::vector<SubscriptionId> one_shot_ids;
    {
        const std::lock_guard lock(state_mutex_);
        for (const auto& [id, subscription] : subscriptions_) {
            if (subscription.topic == event.topic()) {
                handlers.emplace_back(id, subscription.handler);
                if (subscription.one_shot) {
                    one_shot_ids.push_back(id);
                }
            }
        }
        for (const auto id : one_shot_ids) {
            subscriptions_.erase(id);
        }
    }

    std::vector<HandlerOutcome> handler_outcomes;
    handler_outcomes.reserve(handlers.size());
    for (const auto& [id, handler] : handlers) {
        try {
            handler(event);
            handler_outcomes.push_back({id, HandlerStatus::succeeded, {}});
        } catch (const std::exception& error) {
            handler_outcomes.push_back({id, HandlerStatus::failed, error.what()});
        } catch (...) {
            handler_outcomes.push_back(
                {id, HandlerStatus::failed, "handler threw a non-standard exception"});
        }
    }

    const auto failures = static_cast<std::size_t>(std::count_if(
        handler_outcomes.begin(), handler_outcomes.end(), [](const HandlerOutcome& outcome) {
            return outcome.status == HandlerStatus::failed;
        }));
    DispatchStatus status = DispatchStatus::unhandled;
    if (!handler_outcomes.empty()) {
        if (failures == 0) {
            status = DispatchStatus::succeeded;
        } else if (failures == handler_outcomes.size()) {
            status = DispatchStatus::failed;
        } else {
            status = DispatchStatus::partially_failed;
        }
    }

    DispatchOutcome outcome{event, std::move(handler_outcomes), status};
    {
        const std::lock_guard lock(state_mutex_);
        if (history_capacity_ == 0) {
            ++history_evictions_;
        } else {
            if (history_.size() == history_capacity_) {
                history_.pop_front();
                ++history_evictions_;
            }
            history_.push_back(outcome);
        }
    }
    return outcome;
}

void DeterministicDispatcher::validate_causal_parent(const EventEnvelope& event) const {
    if (event.causal_parent_id().has_value() &&
        !seen_event_ids_.contains(*event.causal_parent_id())) {
        throw std::invalid_argument("causal parent has not been dispatched");
    }
}

ResourceQuantity ResourceSnapshot::available() const noexcept {
    if (current > capacity || reserved > capacity - current) {
        return 0;
    }
    return capacity - current - reserved;
}

namespace detail {

struct ResourcePoolState final {
    struct ReservationRecord final {
        ResourceRequest amounts;
        ReservationState state{ReservationState::reserved};
    };

    mutable std::mutex mutex;
    ResourceSnapshotMap resources;
    std::map<ReservationId, ReservationRecord> reservations;
    ReservationId next_reservation_id{1};
};

} // namespace detail

ResourceReservation::ResourceReservation(
    std::shared_ptr<detail::ResourcePoolState> state,
    ReservationId id) noexcept
    : state_(std::move(state)),
      id_(id),
      terminal_state_(ReservationState::reserved) {}

ResourceReservation::~ResourceReservation() noexcept {
    release_noexcept();
}

ResourceReservation::ResourceReservation(ResourceReservation&& other) noexcept
    : state_(std::move(other.state_)),
      id_(std::exchange(other.id_, 0)),
      terminal_state_(std::exchange(other.terminal_state_, ReservationState::invalid)) {}

ResourceReservation& ResourceReservation::operator=(ResourceReservation&& other) noexcept {
    if (this != &other) {
        release_noexcept();
        state_ = std::move(other.state_);
        id_ = std::exchange(other.id_, 0);
        terminal_state_ =
            std::exchange(other.terminal_state_, ReservationState::invalid);
    }
    return *this;
}

ReservationId ResourceReservation::id() const noexcept {
    return id_;
}

ReservationState ResourceReservation::state() const noexcept {
    if (id_ == 0 || state_ == nullptr) {
        return terminal_state_;
    }
    try {
        const std::lock_guard lock(state_->mutex);
        const auto iterator = state_->reservations.find(id_);
        return iterator == state_->reservations.end() ? ReservationState::released
                                                      : iterator->second.state;
    } catch (...) {
        return ReservationState::invalid;
    }
}

bool ResourceReservation::commit() {
    if (id_ == 0 || state_ == nullptr) {
        return false;
    }
    const std::lock_guard lock(state_->mutex);
    const auto reservation = state_->reservations.find(id_);
    if (reservation == state_->reservations.end() ||
        reservation->second.state != ReservationState::reserved) {
        return false;
    }
    for (const auto& [resource, quantity] : reservation->second.amounts) {
        auto& account = state_->resources.at(resource);
        account.reserved -= quantity;
        account.current += quantity;
    }
    reservation->second.state = ReservationState::committed;
    terminal_state_ = ReservationState::committed;
    return true;
}

bool ResourceReservation::rollback() {
    if (id_ == 0 || state_ == nullptr) {
        return false;
    }
    const std::lock_guard lock(state_->mutex);
    const auto reservation = state_->reservations.find(id_);
    if (reservation == state_->reservations.end() ||
        reservation->second.state != ReservationState::reserved) {
        return false;
    }
    for (const auto& [resource, quantity] : reservation->second.amounts) {
        state_->resources.at(resource).reserved -= quantity;
    }
    state_->reservations.erase(reservation);
    id_ = 0;
    terminal_state_ = ReservationState::released;
    return true;
}

bool ResourceReservation::release() {
    if (id_ == 0 || state_ == nullptr) {
        return false;
    }
    const std::lock_guard lock(state_->mutex);
    const auto reservation = state_->reservations.find(id_);
    if (reservation == state_->reservations.end()) {
        id_ = 0;
        terminal_state_ = ReservationState::released;
        return false;
    }
    for (const auto& [resource, quantity] : reservation->second.amounts) {
        auto& account = state_->resources.at(resource);
        if (reservation->second.state == ReservationState::reserved) {
            account.reserved -= quantity;
        } else {
            account.current -= quantity;
        }
    }
    state_->reservations.erase(reservation);
    id_ = 0;
    terminal_state_ = ReservationState::released;
    return true;
}

ResourceReservation::operator bool() const noexcept {
    return id_ != 0 && state_ != nullptr;
}

void ResourceReservation::release_noexcept() noexcept {
    try {
        static_cast<void>(release());
    } catch (...) {
        // Destructors must not terminate the process during stack unwinding.
    }
}

ResourceAccounts::ResourceAccounts(ResourceRequest capacities)
    : state_(std::make_shared<detail::ResourcePoolState>()) {
    if (capacities.empty()) {
        throw std::invalid_argument("at least one resource capacity is required");
    }
    for (const auto& [resource, capacity] : capacities) {
        validate_text(resource, "resource name", kMaximumResourceNameLength);
        state_->resources.emplace(resource, ResourceSnapshot{capacity, 0, 0, 0});
    }
}

std::optional<ResourceReservation> ResourceAccounts::try_reserve(
    const ResourceRequest& request,
    ReservationError* error) {
    clear_error(error);
    if (request.empty()) {
        set_error(error,
                  ReservationErrorCode::empty_request,
                  {},
                  0,
                  0,
                  "reservation request cannot be empty");
        return std::nullopt;
    }

    const std::lock_guard lock(state_->mutex);
    for (const auto& [resource, quantity] : request) {
        try {
            validate_text(resource, "resource name", kMaximumResourceNameLength);
        } catch (const std::invalid_argument& exception) {
            set_error(error,
                      ReservationErrorCode::unknown_resource,
                      resource,
                      quantity,
                      0,
                      exception.what());
            return std::nullopt;
        }
        if (quantity == 0) {
            set_error(error,
                      ReservationErrorCode::invalid_quantity,
                      resource,
                      quantity,
                      0,
                      "reservation quantities must be positive");
            return std::nullopt;
        }
        const auto account = state_->resources.find(resource);
        if (account == state_->resources.end()) {
            set_error(error,
                      ReservationErrorCode::unknown_resource,
                      resource,
                      quantity,
                      0,
                      "resource is not configured");
            return std::nullopt;
        }
        const auto available = account->second.available();
        if (quantity > available) {
            set_error(error,
                      ReservationErrorCode::insufficient_capacity,
                      resource,
                      quantity,
                      available,
                      "resource capacity is insufficient");
            return std::nullopt;
        }
    }

    if (state_->next_reservation_id == std::numeric_limits<ReservationId>::max()) {
        set_error(error,
                  ReservationErrorCode::identifier_exhausted,
                  {},
                  0,
                  0,
                  "reservation identifiers are exhausted");
        return std::nullopt;
    }

    const auto id = state_->next_reservation_id;
    state_->reservations.emplace(
        id,
        detail::ResourcePoolState::ReservationRecord{request, ReservationState::reserved});
    for (const auto& [resource, quantity] : request) {
        auto& account = state_->resources.at(resource);
        account.reserved += quantity;
        account.peak = std::max(account.peak, account.current + account.reserved);
    }
    ++state_->next_reservation_id;
    return ResourceReservation(state_, id);
}

bool ResourceAccounts::set_capacity(std::string_view resource,
                                    ResourceQuantity capacity,
                                    ReservationError* error) {
    clear_error(error);
    const std::lock_guard lock(state_->mutex);
    const auto account = state_->resources.find(resource);
    if (account == state_->resources.end()) {
        set_error(error,
                  ReservationErrorCode::unknown_resource,
                  std::string(resource),
                  capacity,
                  0,
                  "resource is not configured");
        return false;
    }
    if (account->second.current > std::numeric_limits<ResourceQuantity>::max()
        - account->second.reserved) {
        set_error(error,
                  ReservationErrorCode::capacity_below_usage,
                  std::string(resource),
                  capacity,
                  std::numeric_limits<ResourceQuantity>::max(),
                  "resource usage accounting overflowed");
        return false;
    }
    const auto occupied = account->second.current + account->second.reserved;
    if (capacity < occupied) {
        set_error(error,
                  ReservationErrorCode::capacity_below_usage,
                  std::string(resource),
                  capacity,
                  occupied,
                  "capacity cannot be reduced below current and reserved usage");
        return false;
    }
    account->second.capacity = capacity;
    return true;
}

std::optional<ResourceSnapshot> ResourceAccounts::snapshot(
    std::string_view resource) const {
    const std::lock_guard lock(state_->mutex);
    const auto account = state_->resources.find(resource);
    if (account == state_->resources.end()) {
        return std::nullopt;
    }
    return account->second;
}

ResourceSnapshotMap ResourceAccounts::snapshot_all() const {
    const std::lock_guard lock(state_->mutex);
    return state_->resources;
}

std::size_t ResourceAccounts::active_reservations() const {
    const std::lock_guard lock(state_->mutex);
    return state_->reservations.size();
}

bool StateTransitionOutcome::applied() const noexcept {
    return status == StateTransitionStatus::applied;
}

CausalStateMachine::CausalStateMachine(std::string initial_state,
                                       std::vector<StateTransitionRule> rules,
                                       Sequence first_sequence)
    : initial_state_(std::move(initial_state)),
      current_state_(initial_state_),
      first_sequence_(first_sequence),
      next_sequence_(first_sequence),
      replay_digest_(state_seed(initial_state_, first_sequence_)) {
    validate_text(initial_state_, "initial state", kMaximumIdentifierLength);
    if (first_sequence_ == 0 || first_sequence_ == std::numeric_limits<Sequence>::max()) {
        throw std::invalid_argument("first_sequence must be within the usable sequence range");
    }
    for (auto& rule : rules) {
        validate_text(rule.from_state, "transition from_state", kMaximumIdentifierLength);
        validate_text(rule.event_topic, "transition event_topic", kMaximumTopicLength);
        validate_text(rule.to_state, "transition to_state", kMaximumIdentifierLength);
        const auto key = std::make_pair(std::move(rule.from_state),
                                        std::move(rule.event_topic));
        if (!rules_.emplace(key, std::move(rule.to_state)).second) {
            throw std::invalid_argument(
                "a state and event topic can have only one transition rule");
        }
    }
}

StateTransitionOutcome CausalStateMachine::apply(const EventEnvelope& event) {
    const std::lock_guard lock(mutex_);
    return apply_locked(event);
}

std::vector<StateTransitionOutcome> CausalStateMachine::replay(
    std::span<const EventEnvelope> events) {
    const std::lock_guard lock(mutex_);
    const auto saved_current_state = current_state_;
    const auto saved_next_sequence = next_sequence_;
    const auto saved_last_event_id = last_event_id_;
    const auto saved_replay_digest = replay_digest_;
    const auto saved_seen_event_ids = seen_event_ids_;
    const auto saved_history = history_;
    std::vector<StateTransitionOutcome> outcomes;
    outcomes.reserve(events.size());
    for (const auto& event : events) {
        outcomes.push_back(apply_locked(event));
        if (!outcomes.back().applied()) {
            current_state_ = saved_current_state;
            next_sequence_ = saved_next_sequence;
            last_event_id_ = saved_last_event_id;
            replay_digest_ = saved_replay_digest;
            seen_event_ids_ = saved_seen_event_ids;
            history_ = saved_history;
            break;
        }
    }
    return outcomes;
}

std::string CausalStateMachine::current_state() const {
    const std::lock_guard lock(mutex_);
    return current_state_;
}

Sequence CausalStateMachine::next_sequence() const {
    const std::lock_guard lock(mutex_);
    return next_sequence_;
}

std::string CausalStateMachine::replay_digest() const {
    const std::lock_guard lock(mutex_);
    return replay_digest_;
}

std::vector<StateTransitionRecord> CausalStateMachine::history_snapshot() const {
    const std::lock_guard lock(mutex_);
    return history_;
}

bool CausalStateMachine::verify() const {
    const std::lock_guard lock(mutex_);
    auto state = initial_state_;
    auto expected_sequence = first_sequence_;
    std::optional<std::string> last_event_id;
    std::set<std::string, std::less<>> event_ids;
    auto digest = state_seed(initial_state_, first_sequence_);

    for (const auto& record : history_) {
        if (expected_sequence == std::numeric_limits<Sequence>::max() ||
            record.event_sequence != expected_sequence ||
            !event_ids.insert(record.event_id).second ||
            record.causal_parent_id != last_event_id || record.from_state != state ||
            record.previous_digest != digest) {
            return false;
        }
        const auto rule = rules_.find({record.from_state, record.event_topic});
        if (rule == rules_.end() || rule->second != record.to_state ||
            transition_digest(record) != record.digest) {
            return false;
        }
        state = record.to_state;
        last_event_id = record.event_id;
        digest = record.digest;
        ++expected_sequence;
    }

    return state == current_state_ && expected_sequence == next_sequence_ &&
           last_event_id == last_event_id_ && digest == replay_digest_ &&
           event_ids == seen_event_ids_;
}

StateTransitionOutcome CausalStateMachine::apply_locked(const EventEnvelope& event) {
    const auto rejected = [&](StateTransitionStatus status, std::string message) {
        return StateTransitionOutcome{status,
                                      event.sequence(),
                                      event.event_id(),
                                      current_state_,
                                      current_state_,
                                      std::move(message)};
    };

    if (next_sequence_ == std::numeric_limits<Sequence>::max()) {
        return rejected(StateTransitionStatus::sequence_exhausted,
                        "state transition sequence is exhausted");
    }
    if (event.sequence() != next_sequence_) {
        return rejected(StateTransitionStatus::sequence_mismatch,
                        "event sequence does not match the next transition sequence");
    }
    if (seen_event_ids_.contains(event.event_id())) {
        return rejected(StateTransitionStatus::duplicate_event,
                        "event_id was already applied");
    }
    if (event.causal_parent_id() != last_event_id_) {
        return rejected(StateTransitionStatus::causal_mismatch,
                        "event does not directly descend from the previous transition");
    }
    const auto rule = rules_.find({current_state_, event.topic()});
    if (rule == rules_.end()) {
        return rejected(StateTransitionStatus::transition_not_allowed,
                        "no transition rule matches the current state and event topic");
    }

    StateTransitionRecord record;
    record.event_sequence = event.sequence();
    record.event_id = event.event_id();
    record.event_topic = event.topic();
    record.causal_parent_id = event.causal_parent_id();
    record.from_state = current_state_;
    record.to_state = rule->second;
    record.event_envelope_digest = event.envelope_digest();
    record.previous_digest = replay_digest_;
    record.digest = transition_digest(record);

    history_.push_back(record);
    seen_event_ids_.insert(event.event_id());
    current_state_ = rule->second;
    last_event_id_ = event.event_id();
    replay_digest_ = record.digest;
    ++next_sequence_;

    return StateTransitionOutcome{StateTransitionStatus::applied,
                                  event.sequence(),
                                  event.event_id(),
                                  record.from_state,
                                  record.to_state,
                                  {}};
}

} // namespace genesis::runtime
