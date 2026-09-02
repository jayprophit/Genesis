#pragma once

#include "genesis/runtime/runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::organism {

enum class SignalKind { excitatory, inhibitory, priority, reflex, modulatory, resource, stress, repair, reward, error };
struct Signal {
    std::string id, source, destination, topic, payload_digest;
    SignalKind kind{SignalKind::excitatory};
    std::uint8_t priority{};
    std::uint64_t created_at{}, expires_at{}, sequence{};
};
class SignalRouter final {
public:
    explicit SignalRouter(std::size_t capacity);
    bool register_endpoint(std::string endpoint);
    bool enqueue(Signal signal, std::string* error = nullptr);
    std::vector<Signal> drain(std::string_view destination, std::uint64_t now, std::size_t maximum);
    [[nodiscard]] std::size_t queued() const noexcept;
    [[nodiscard]] std::uint64_t expired_count() const noexcept;
private:
    std::size_t capacity_{};
    std::uint64_t next_sequence_{1}, expired_{};
    std::set<std::string> endpoints_;
    std::set<std::string> queued_ids_;
    std::vector<Signal> queue_;
};

enum class MetabolicResource { cpu, gpu, vram, ram, disk, io, bandwidth, energy };
class Metabolism final {
public:
    explicit Metabolism(std::map<MetabolicResource, std::uint64_t> capacities);
    std::optional<runtime::ResourceReservation> reserve(const std::map<MetabolicResource, std::uint64_t>& request);
    [[nodiscard]] std::uint64_t available(MetabolicResource resource) const;
private:
    runtime::ResourceAccounts accounts_;
};

enum class Metric { resource_pressure, memory_pressure, temperature, error_rate, latency, system_load, uncertainty, learning_saturation, agent_count, storage_pressure, fragmentation, maintenance_backlog };
enum class PressureLevel { nominal, low_warning, high_warning, critical_low, critical_high, invalid };
enum class CompensatoryAction { none, reduce_load, release_memory, request_cooling, isolate_fault, throttle_compute, reduce_agents, schedule_maintenance, consolidate_storage, pause_learning, request_operator };
struct OperatingBand { double hard_min{}, target_min{}, target_max{}, hard_max{}; };
struct HomeostasisDecision { PressureLevel level{PressureLevel::invalid}; CompensatoryAction action{CompensatoryAction::request_operator}; };
class HomeostasisController final {
public:
    bool configure(Metric metric, OperatingBand band);
    [[nodiscard]] HomeostasisDecision evaluate(Metric metric, double value) const;
private:
    std::map<Metric, OperatingBand> bands_;
};
struct ModulationState {
    double urgency{}, exploration{}, caution{}, learning_plasticity{}, consolidation_priority{}, social_orientation{}, resource_conservation{}, focus{}, stress{};
    bool apply(const ModulationState& delta);
};

enum class IdentityClass { self, authorized_extension, descendant, trusted_external, untrusted_external, corrupted_self, malicious_external, unknown };
enum class AuthenticationState { unavailable, diagnostic_only, authenticated, failed };
struct IntegrityObservation {
    std::string subject_id, organism_id, expected_owner_id, observed_owner_id, provider_id;
    bool authorized_extension{}, descendant{}, trusted_external{}, digest_match{true}, behavioral_anomaly{}, provider_qualified{};
    AuthenticationState authentication{AuthenticationState::unavailable};
};
class ImmuneClassifier final {
public:
    [[nodiscard]] IdentityClass classify(const IntegrityObservation& observation) const;
};
struct ImmuneIncident { std::string signature, root_cause, mitigation, regression_test; IdentityClass identity{IdentityClass::unknown}; std::uint64_t timestamp{}; };
class ImmuneMemory final {
public:
    explicit ImmuneMemory(std::size_t capacity);
    bool remember(ImmuneIncident incident);
    [[nodiscard]] const ImmuneIncident* find(std::string_view signature) const noexcept;
    bool quarantine(std::string subject_id);
    bool release(std::string_view subject_id);
    [[nodiscard]] bool quarantined(std::string_view subject_id) const;
    [[nodiscard]] const std::vector<ImmuneIncident>& incidents() const noexcept;
private:
    std::size_t capacity_{};
    std::vector<ImmuneIncident> incidents_;
    std::set<std::string> quarantine_;
};

} // namespace genesis::organism
