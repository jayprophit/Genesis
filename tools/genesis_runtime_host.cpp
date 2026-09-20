// genesis_runtime_host: minimal persistent organism runtime host (P3).
//
// Verbs: --boot | --event --topic T --payload P | --checkpoint | --status
//        --self-test   (full boot->event->checkpoint->restart cycle, for CTest)
// Identity (namespace/local_key) never derives from --model-route; the route
// is recorded as an annotation only, so model replacement keeps identity.
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "genesis/identity/entity_persistence.hpp"
#include "genesis/identity/entity_registry.hpp"
#include "genesis/memory/graph.hpp"
#include "genesis/memory/persistence.hpp"
#include "genesis/runtime/runtime.hpp"

namespace fs = std::filesystem;
namespace runtime = genesis::runtime;
namespace identity = genesis::identity;
namespace memory = genesis::memory;

namespace {

constexpr std::string_view kNamespace = "local.genesis";
constexpr std::string_view kOrganismKey = "organism-main";
constexpr std::string_view kManifestFile = "genesis.boot";
constexpr std::uint64_t kEventBudget = 4096;

struct Manifest {
    std::string version = "v0";
    std::uint64_t version_number = 0;
    std::string organism_id;
    std::string state = "boot";
    runtime::Sequence next_sequence = 1;
    std::string registry_digest;
    std::string memory_digest;
    std::string model_route;
};

std::string NextVersion(std::uint64_t n) { return "v" + std::to_string(n); }

std::map<std::string, std::string> ReadManifest(const fs::path& root, bool* present) {
    std::map<std::string, std::string> out;
    std::ifstream in(root / kManifestFile);
    *present = static_cast<bool>(in);
    if (!*present) return out;
    std::string line;
    while (std::getline(in, line)) {
        const auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        out[line.substr(0, pos)] = line.substr(pos + 1);
    }
    return out;
}

bool WriteManifest(const fs::path& root, const Manifest& m) {
    const fs::path tmp = root / "genesis.boot.tmp";
    std::ofstream out(tmp, std::ios::trunc);
    if (!out) return false;
    out << "version=" << m.version << "\n"
        << "organism=" << m.organism_id << "\n"
        << "state=" << m.state << "\n"
        << "next_sequence=" << m.next_sequence << "\n"
        << "registry_digest=" << m.registry_digest << "\n"
        << "memory_digest=" << m.memory_digest << "\n"
        << "model_route=" << m.model_route << "\n";
    out.close();
    if (!out) return false;
    std::error_code ec;
    fs::rename(tmp, root / kManifestFile, ec);
    return !ec;
}

Manifest LoadManifest(const fs::path& root, bool* present) {
    Manifest m;
    const auto kv = ReadManifest(root, present);
    if (!*present) return m;
    m.version = kv.count("version") ? kv.at("version") : "v0";
    m.organism_id = kv.count("organism") ? kv.at("organism") : "";
    m.state = kv.count("state") ? kv.at("state") : "boot";
    m.next_sequence = kv.count("next_sequence") ? std::stoull(kv.at("next_sequence")) : 1;
    m.registry_digest = kv.count("registry_digest") ? kv.at("registry_digest") : "";
    m.memory_digest = kv.count("memory_digest") ? kv.at("memory_digest") : "";
    m.model_route = kv.count("model_route") ? kv.at("model_route") : "";
    if (m.version.size() > 1 && m.version[0] == 'v')
        m.version_number = std::stoull(m.version.substr(1));
    return m;
}

std::vector<runtime::StateTransitionRule> OrganismRules() {
    return {
        {"boot", "genesis.boot", "ready"},
        {"ready", "organism.event", "ready"},
        {"ready", "organism.checkpoint", "ready"},
    };
}

struct LoadedState {
    Manifest manifest;
    identity::EntityRegistry registry = identity::EntityRegistry(
        std::string(kNamespace), std::string(kOrganismKey), 256, 1024);
    // MemoryGraph rejects an empty organism id: emplace only once the
    // identity is known (fresh boot) or reloaded (existing store).
    std::optional<memory::MemoryGraph> graph;
    // Dispatcher/machine are neither copyable nor movable: reconstruct in place.
    std::optional<runtime::DeterministicDispatcher> dispatcher;
    std::optional<runtime::CausalStateMachine> machine;
    runtime::ResourceAccounts accounts{runtime::ResourceRequest{{"events", kEventBudget}}};
};

// Rebuilds the in-memory runtime from durable stores. Returns false on error.
bool LoadState(const fs::path& root, LoadedState& st, std::string& error) {
    bool present = false;
    st.manifest = LoadManifest(root, &present);
    if (!present) {
        error = "no boot manifest; run --boot first";
        return false;
    }
    identity::EntityRegistryStore registry_store(root / "identity");
    identity::EntityStoreError registry_error;
    auto registry = registry_store.read(std::string(kNamespace), std::string(kOrganismKey),
                                        st.manifest.version, &registry_error);
    if (!registry) {
        error = "registry read failed: " + registry_error.message;
        return false;
    }
    memory::MemoryStore memory_store(root / "memory");
    memory::MemoryStoreError memory_error;
    auto graph = memory_store.read(st.manifest.organism_id, st.manifest.version, &memory_error);
    if (!graph) {
        error = "memory read failed: " + memory_error.message;
        return false;
    }
    st.registry = std::move(*registry);
    st.graph.emplace(std::move(*graph));
    st.dispatcher.emplace(1024, st.manifest.next_sequence, 0);
    st.machine.emplace(st.manifest.state, OrganismRules(), st.manifest.next_sequence);
    st.accounts = runtime::ResourceAccounts(runtime::ResourceRequest{{"events", kEventBudget}});
    return true;
}

// Persists registry+memory as the next immutable version and updates manifest.
bool PersistState(const fs::path& root, LoadedState& st, std::string& error) {
    const std::string version = NextVersion(st.manifest.version_number + 1);
    identity::EntityRegistryStore registry_store(root / "identity");
    identity::EntityStoreError registry_error;
    if (!registry_store.write(st.registry, version, &registry_error)) {
        error = "registry write failed: " + registry_error.message;
        return false;
    }
    memory::MemoryStore memory_store(root / "memory");
    memory::MemoryStoreError memory_error;
    if (!memory_store.write(*st.graph, version, &memory_error)) {
        error = "memory write failed: " + memory_error.message;
        return false;
    }
    st.manifest.version = version;
    st.manifest.version_number += 1;
    st.manifest.state = st.machine->current_state();
    st.manifest.next_sequence = st.machine->next_sequence();
    st.manifest.registry_digest = runtime::sha256(
        identity::EntityRegistryStore::serialize(st.registry));
    st.manifest.memory_digest = runtime::sha256(memory::MemoryStore::serialize(*st.graph));
    if (!WriteManifest(root, st.manifest)) {
        error = "manifest write failed";
        return false;
    }
    return true;
}

int Fail(const std::string& message) {
    std::cerr << "genesis_runtime_host: " << message << "\n";
    return 1;
}

std::optional<runtime::DispatchOutcome> ApplyEvent(LoadedState& st, const std::string& topic,
                const std::string& payload, bool fault_demo,
                std::size_t& observed, std::string& error);

int CmdBoot(const fs::path& root, const std::string& model_route) {
    bool present = false;
    ReadManifest(root, &present);
    if (present) {
        LoadedState st;
        std::string error;
        if (!LoadState(root, st, error)) return Fail(error);
        if (!st.manifest.organism_id.empty() &&
            st.registry.organism_identity(st.manifest.organism_id).has_value()) {
            std::cout << "already booted identity=" << st.manifest.organism_id
                      << " state=" << st.manifest.state << "\n";
            return 0;
        }
        return Fail("manifest present but organism identity missing");
    }
    std::error_code ec;
    fs::create_directories(root / "identity", ec);
    fs::create_directories(root / "memory", ec);

    LoadedState st;
    const std::string seed = runtime::sha256("genesis-boot");
    auto address = identity::make_entity_address(
        std::string(kNamespace), identity::EntityKind::organism,
        std::string(kOrganismKey), seed, 1);
    identity::EntityRegistryError registry_error;
    if (!st.registry.register_entity(address, &registry_error))
        return Fail("register organism: " + registry_error.message);
    // Fresh graph owned by the organism identity (model route excluded).
    st.graph.emplace(address.entity_id, 4096, 8192);
    st.machine.emplace("boot", OrganismRules(), 1);
    st.dispatcher.emplace(1024, 1, 0);

    st.manifest.organism_id = address.entity_id;
    st.manifest.model_route = model_route;
    std::string error;
    std::size_t observed = 0;
    if (!ApplyEvent(st, "genesis.boot", "genesis boot model_route=" + model_route,
                    false, observed, error))
        return Fail(error);
    if (!PersistState(root, st, error)) return Fail(error);
    std::cout << "booted identity=" << st.manifest.organism_id
              << " version=" << st.manifest.version << "\n";
    return 0;
}

// Publishes one event through dispatcher + state machine + memory. The
// caller owns budget reservation and persistence.
std::optional<runtime::DispatchOutcome> ApplyEvent(LoadedState& st, const std::string& topic,
                const std::string& payload, bool fault_demo,
                std::size_t& observed, std::string& error) {
    if (fault_demo) {
        (void)st.dispatcher->subscribe(
            topic, [](const runtime::EventEnvelope&) { throw std::runtime_error("demo fault"); });
    }
    observed = 0;
    (void)st.dispatcher->subscribe(topic, [&](const runtime::EventEnvelope&) { ++observed; });

    runtime::EventDraft draft;
    draft.event_id = "evt-" + std::to_string(st.manifest.next_sequence);
    draft.source_id = st.manifest.organism_id;
    draft.topic = topic;
    draft.causal_parent_id = std::nullopt;
    draft.payload_digest = runtime::sha256(payload);
    auto outcome = st.dispatcher->publish(std::move(draft));
    if (fault_demo && outcome.failure_count() != 1) {
        error = "fault containment failed: expected 1 contained failure";
        return std::nullopt;
    }

    const auto transition = st.machine->apply(outcome.event);
    if (!transition.applied()) {
        error = "transition rejected: " + transition.message;
        return std::nullopt;
    }

    memory::MemoryNode node;
    node.id = outcome.event.event_id();
    node.owner_id = st.manifest.organism_id;
    node.content_digest = outcome.event.payload_digest();
    node.provenance_digest = outcome.event.envelope_digest();
    node.context = topic;
    node.features = {topic};
    std::string memory_error;
    if (!st.graph->add(std::move(node), &memory_error)) {
        error = "memory add failed: " + memory_error;
        return std::nullopt;
    }
    return outcome;
}

int CmdEvent(const fs::path& root, const std::string& topic, const std::string& payload,
             bool fault_demo) {
    LoadedState st;
    std::string error;
    if (!LoadState(root, st, error)) return Fail(error);

    runtime::ReservationError budget_error;
    auto reservation = st.accounts.try_reserve({{"events", 1}}, &budget_error);
    if (!reservation) {
        std::cerr << "genesis_runtime_host: event budget exhausted: "
                  << budget_error.message << "\n";
        return 3;
    }

    std::size_t observed = 0;
    const auto outcome = ApplyEvent(st, topic, payload, fault_demo, observed, error);
    if (!outcome) return Fail(error);

    if (!reservation->commit()) return Fail("resource commit failed");
    if (!PersistState(root, st, error)) return Fail(error);
    std::cout << "event=" << outcome->event.event_id()
              << " seq=" << outcome->event.sequence()
              << " state=" << st.machine->current_state()
              << " observed=" << observed
              << " contained_failures=" << outcome->failure_count()
              << " version=" << st.manifest.version << "\n";
    return 0;
}

int CmdCheckpoint(const fs::path& root) {
    LoadedState st;
    std::string error;
    if (!LoadState(root, st, error)) return Fail(error);
    if (!st.registry.verify()) return Fail("registry verify failed");
    if (!st.graph->verify()) return Fail("memory verify failed");
    if (!st.machine->verify()) {
        std::cerr << "genesis_runtime_host: state machine verify failed\n";
        return 2;
    }
    std::cout << "checkpoint ok identity=" << st.manifest.organism_id
              << " version=" << st.manifest.version
              << " state=" << st.manifest.state
              << " next_sequence=" << st.manifest.next_sequence
              << " registry=" << st.manifest.registry_digest.substr(0, 16)
              << " memory=" << st.manifest.memory_digest.substr(0, 16) << "\n";
    return 0;
}

int CmdStatus(const fs::path& root) { return CmdCheckpoint(root); }

int RunArgv(const std::vector<std::string>& args);

int CmdSelfTest(const fs::path& scratch) {
    const fs::path root = scratch / "genesis-host-selftest";
    std::error_code ec;
    fs::remove_all(root, ec);
    auto step = [&](const std::vector<std::string>& args) {
        return RunArgv(args);
    };
    const std::string r = root.string();
    // Boot under one model route; identity must not depend on it.
    if (step({"--boot", "--data-root", r, "--model-route", "test-route-a"}) != 0)
        return Fail("self-test: boot failed");
    bool present = false;
    const std::string id_a = LoadManifest(root, &present).organism_id;
    if (!present || id_a.empty()) return Fail("self-test: no identity after boot");
    // Re-boot under a different route: same identity, no duplicate.
    if (step({"--boot", "--data-root", r, "--model-route", "test-route-b"}) != 0)
        return Fail("self-test: re-boot failed");
    const std::string id_b = LoadManifest(root, &present).organism_id;
    if (id_a != id_b) return Fail("self-test: model route changed identity");
    // Event persists memory + advances deterministic sequence.
    if (step({"--event", "--data-root", r, "--topic", "organism.event",
              "--payload", "self-test observation"}) != 0)
        return Fail("self-test: event failed");
    // Fault containment: throwing handler must not break the machine.
    if (step({"--event", "--data-root", r, "--topic", "organism.event",
              "--payload", "fault probe", "--fault-demo"}) != 0)
        return Fail("self-test: fault containment failed");
    if (step({"--checkpoint", "--data-root", r}) != 0)
        return Fail("self-test: checkpoint failed");
    // Restart continuity: reload and confirm identity + memory + sequence.
    LoadedState st;
    std::string error;
    if (!LoadState(root, st, error)) return Fail("self-test: reload: " + error);
    if (st.manifest.organism_id != id_a)
        return Fail("self-test: identity changed across restart");
    if (st.graph->size() < 2) return Fail("self-test: memory lost across restart");
    if (st.manifest.next_sequence < 3)
        return Fail("self-test: sequence did not advance");
    std::cout << "self-test ok identity=" << id_a << "\n";
    fs::remove_all(root, ec);
    return 0;
}

int RunArgv(const std::vector<std::string>& args) {
    std::string verb, root = ".genesis-host", topic = "organism.event",
                payload, model_route;
    bool fault_demo = false;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const auto& a = args[i];
        if (a == "--boot" || a == "--event" || a == "--checkpoint" ||
            a == "--status" || a == "--self-test") {
            verb = a;
        } else if (a == "--data-root" && i + 1 < args.size()) {
            root = args[++i];
        } else if (a == "--topic" && i + 1 < args.size()) {
            topic = args[++i];
        } else if (a == "--payload" && i + 1 < args.size()) {
            payload = args[++i];
        } else if (a == "--model-route" && i + 1 < args.size()) {
            model_route = args[++i];
        } else if (a == "--fault-demo") {
            fault_demo = true;
        } else {
            return Fail("unknown argument: " + a);
        }
    }
    if (verb == "--boot") return CmdBoot(root, model_route);
    if (verb == "--event") return CmdEvent(root, topic, payload, fault_demo);
    if (verb == "--checkpoint") return CmdCheckpoint(root);
    if (verb == "--status") return CmdStatus(root);
    if (verb == "--self-test") {
        std::error_code ec;
        const fs::path scratch = fs::temp_directory_path(ec);
        if (ec) return Fail("no temp dir");
        return CmdSelfTest(scratch);
    }
    return Fail("usage: --boot|--event|--checkpoint|--status|--self-test "
                "[--data-root DIR] [--topic T] [--payload P] "
                "[--model-route R] [--fault-demo]");
}

} // namespace

int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
    return RunArgv(args);
}
