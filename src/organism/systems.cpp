#include "genesis/organism/systems.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace genesis::organism {
namespace {
bool text_ok(std::string_view value) { return !value.empty() && value.size() <= 256 && std::all_of(value.begin(), value.end(), [](unsigned char c) { return c >= 0x20 && c != 0x7f; }); }
bool digest_ok(std::string_view value) { return value.size() == 64 && std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isxdigit(c) != 0; }); }
std::string resource_name(MetabolicResource value) {
    static constexpr std::string_view names[]{"cpu_units", "gpu_units", "vram_bytes", "ram_bytes", "disk_bytes", "io_units", "bandwidth_units", "energy_units"};
    return std::string(names[static_cast<std::size_t>(value)]);
}
CompensatoryAction action_for(Metric metric) {
    switch (metric) {
    case Metric::memory_pressure: return CompensatoryAction::release_memory;
    case Metric::temperature: return CompensatoryAction::request_cooling;
    case Metric::error_rate: return CompensatoryAction::isolate_fault;
    case Metric::agent_count: return CompensatoryAction::reduce_agents;
    case Metric::maintenance_backlog: return CompensatoryAction::schedule_maintenance;
    case Metric::fragmentation: case Metric::storage_pressure: return CompensatoryAction::consolidate_storage;
    case Metric::learning_saturation: return CompensatoryAction::pause_learning;
    case Metric::latency: case Metric::system_load: case Metric::resource_pressure: return CompensatoryAction::reduce_load;
    case Metric::uncertainty: return CompensatoryAction::request_operator;
    }
    return CompensatoryAction::request_operator;
}
} // namespace

SignalRouter::SignalRouter(std::size_t capacity) : capacity_(capacity) { if (capacity == 0) throw std::invalid_argument("signal capacity must be positive"); }
bool SignalRouter::register_endpoint(std::string endpoint) { return text_ok(endpoint) && endpoints_.insert(std::move(endpoint)).second; }
bool SignalRouter::enqueue(Signal signal, std::string* error) {
    if (error) error->clear();
    const auto reject = [&](std::string message) { if (error) *error = std::move(message); return false; };
    if (!text_ok(signal.id) || !text_ok(signal.source) || !text_ok(signal.destination) || !text_ok(signal.topic) || !digest_ok(signal.payload_digest)) return reject("invalid signal fields");
    if (!endpoints_.contains(signal.source) || !endpoints_.contains(signal.destination)) return reject("unknown signal endpoint");
    if (signal.expires_at <= signal.created_at) return reject("invalid signal lifetime");
    if (queue_.size() >= capacity_) return reject("signal queue capacity exceeded");
    if (queued_ids_.contains(signal.id)) return reject("duplicate signal id");
    signal.sequence = next_sequence_++;
    queued_ids_.insert(signal.id);
    queue_.push_back(std::move(signal));
    return true;
}
std::vector<Signal> SignalRouter::drain(std::string_view destination, std::uint64_t now, std::size_t maximum) {
    std::vector<Signal> result;
    queue_.erase(std::remove_if(queue_.begin(), queue_.end(), [&](const Signal& signal) { if (signal.expires_at <= now) { queued_ids_.erase(signal.id); ++expired_; return true; } return false; }), queue_.end());
    std::stable_sort(queue_.begin(), queue_.end(), [](const Signal& a, const Signal& b) { if (a.priority != b.priority) return a.priority > b.priority; if (a.created_at != b.created_at) return a.created_at < b.created_at; return a.sequence < b.sequence; });
    std::vector<Signal> retained;
    retained.reserve(queue_.size());
    result.reserve(std::min(maximum, queue_.size()));
    for (auto& signal : queue_) {
        if (signal.destination == destination && result.size() < maximum) {
            queued_ids_.erase(signal.id);
            result.push_back(std::move(signal));
        } else {
            retained.push_back(std::move(signal));
        }
    }
    queue_ = std::move(retained);
    return result;
}
std::size_t SignalRouter::queued() const noexcept { return queue_.size(); }
std::uint64_t SignalRouter::expired_count() const noexcept { return expired_; }

Metabolism::Metabolism(std::map<MetabolicResource, std::uint64_t> capacities)
    : accounts_([&] { runtime::ResourceRequest translated; for (const auto& [resource, amount] : capacities) translated.emplace(resource_name(resource), amount); return translated; }()) {}
std::optional<runtime::ResourceReservation> Metabolism::reserve(const std::map<MetabolicResource, std::uint64_t>& request) { runtime::ResourceRequest translated; for (const auto& [resource, amount] : request) translated.emplace(resource_name(resource), amount); return accounts_.try_reserve(translated); }
std::uint64_t Metabolism::available(MetabolicResource resource) const { const auto snapshot = accounts_.snapshot(resource_name(resource)); return snapshot ? snapshot->available() : 0; }

bool HomeostasisController::configure(Metric metric, OperatingBand band) { if (!std::isfinite(band.hard_min) || !std::isfinite(band.target_min) || !std::isfinite(band.target_max) || !std::isfinite(band.hard_max) || band.hard_min > band.target_min || band.target_min > band.target_max || band.target_max > band.hard_max) return false; bands_[metric] = band; return true; }
HomeostasisDecision HomeostasisController::evaluate(Metric metric, double value) const { const auto found = bands_.find(metric); if (found == bands_.end() || !std::isfinite(value)) return {}; const auto& b = found->second; if (value < b.hard_min) return {PressureLevel::critical_low, CompensatoryAction::request_operator}; if (value < b.target_min) return {PressureLevel::low_warning, action_for(metric)}; if (value <= b.target_max) return {PressureLevel::nominal, CompensatoryAction::none}; if (value <= b.hard_max) return {PressureLevel::high_warning, action_for(metric)}; return {PressureLevel::critical_high, metric == Metric::system_load ? CompensatoryAction::throttle_compute : action_for(metric)}; }
bool ModulationState::apply(const ModulationState& d) { double* values[]{&urgency,&exploration,&caution,&learning_plasticity,&consolidation_priority,&social_orientation,&resource_conservation,&focus,&stress}; const double deltas[]{d.urgency,d.exploration,d.caution,d.learning_plasticity,d.consolidation_priority,d.social_orientation,d.resource_conservation,d.focus,d.stress}; for (double delta : deltas) if (!std::isfinite(delta)) return false; for (std::size_t i=0;i<9;++i) *values[i]=std::clamp(*values[i]+deltas[i],0.0,1.0); return true; }

IdentityClass ImmuneClassifier::classify(const IntegrityObservation& o) const { if (!text_ok(o.subject_id) || !text_ok(o.organism_id)) return IdentityClass::unknown; const bool qualified_auth = o.provider_qualified && o.authentication == AuthenticationState::authenticated; const bool claimed_self = o.subject_id == o.organism_id || (!o.expected_owner_id.empty() && o.observed_owner_id == o.expected_owner_id); if (claimed_self && (!o.digest_match || o.behavioral_anomaly || o.authentication == AuthenticationState::failed)) return IdentityClass::corrupted_self; if (claimed_self && (qualified_auth || o.digest_match)) return IdentityClass::self; if (o.authorized_extension) return IdentityClass::authorized_extension; if (o.descendant) return IdentityClass::descendant; if (o.trusted_external && qualified_auth) return IdentityClass::trusted_external; if (o.authentication == AuthenticationState::failed || o.behavioral_anomaly) return IdentityClass::malicious_external; return IdentityClass::untrusted_external; }
ImmuneMemory::ImmuneMemory(std::size_t capacity) : capacity_(capacity) { if (capacity == 0) throw std::invalid_argument("immune memory capacity must be positive"); }
bool ImmuneMemory::remember(ImmuneIncident incident) { if (!text_ok(incident.signature) || !text_ok(incident.root_cause) || !text_ok(incident.mitigation) || !text_ok(incident.regression_test) || find(incident.signature)) return false; if (incidents_.size() == capacity_) incidents_.erase(incidents_.begin()); incidents_.push_back(std::move(incident)); return true; }
const ImmuneIncident* ImmuneMemory::find(std::string_view signature) const noexcept { const auto it=std::find_if(incidents_.begin(),incidents_.end(),[&](const auto& i){return i.signature==signature;}); return it==incidents_.end()?nullptr:&*it; }
bool ImmuneMemory::quarantine(std::string subject_id) { return text_ok(subject_id) && quarantine_.insert(std::move(subject_id)).second; }
bool ImmuneMemory::release(std::string_view subject_id) { return quarantine_.erase(std::string(subject_id)) != 0; }
bool ImmuneMemory::quarantined(std::string_view subject_id) const { return quarantine_.contains(std::string(subject_id)); }
const std::vector<ImmuneIncident>& ImmuneMemory::incidents() const noexcept { return incidents_; }
} // namespace genesis::organism
