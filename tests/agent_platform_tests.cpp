#include "genesis/agents/platform.hpp"
#include "genesis/agents/persistence.hpp"
#include "genesis/runtime/runtime.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <chrono>

namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::string evidence(std::string_view label) { return genesis::runtime::sha256(label); }

void qualify_adapter(genesis::agents::AdapterRegistry& registry,
                     genesis::agents::AdapterDescriptor descriptor,
                     std::uint64_t observed_at = 10,
                     std::uint64_t qualified_at = 20,
                     std::uint64_t valid_until = 100) {
    const auto id = descriptor.adapter_id;
    descriptor.state = genesis::agents::AdapterState::declared;
    check(registry.declare(std::move(descriptor)), "adapter declaration failed");
    check(registry.record_probe(id, true, "observer." + id,
                                evidence("probe:" + id), observed_at),
          "adapter observation failed");
    check(registry.qualify(id, "qualifier." + id,
                           evidence("qualification:" + id), qualified_at,
                           valid_until),
          "adapter qualification failed");
}

genesis::agents::AgentDefinition agent() {
    return {"1.0", "daily-research", "Daily research", "Collect approved updates", "local-default",
            {{"browser", {"read"}, true, false}}, genesis::agents::ApprovalMode::risky_actions, {}, true};
}

void test_preflight_boundaries() {
    using namespace genesis::agents;
    const WorkflowDefinition original{"morning", {{"fetch", "browser", "read", false}}};
    const RunRequest request{"preflight-1", "daily-research", "morning", evidence("input"), 50};
    enum class ToolSetup { missing, declared, observed, qualified, expired };
    const auto prepare = [&](const AgentDefinition& definition, const WorkflowDefinition& plan,
                             AdapterDescriptor descriptor,
                             ToolSetup setup = ToolSetup::qualified) {
        AgentRegistry agents;
        ModelRouter models;
        ToolRegistry tools;
        check(agents.register_agent(definition), "preflight agent fixture invalid");
        qualify_adapter(models, {"local-default", AdapterKind::model,
                                  AdapterState::declared, {"inference"}});
        if (setup != ToolSetup::missing) {
            descriptor.state = AdapterState::declared;
            check(tools.declare(descriptor), "preflight tool fixture invalid");
            if (setup != ToolSetup::declared) {
                check(tools.record_probe(descriptor.adapter_id, true, "observer.browser",
                                         evidence("browser-probe"), 10),
                      "preflight tool observation failed");
            }
            if (setup == ToolSetup::qualified || setup == ToolSetup::expired) {
                check(tools.qualify(descriptor.adapter_id, "qualifier.browser",
                                    evidence("browser-qualification"), 20,
                                    setup == ToolSetup::expired ? 40 : 100),
                      "preflight tool qualification failed");
            }
        }
        return AgentRuntime{}.prepare(request, agents, plan, models, tools);
    };
    const AdapterDescriptor tool{"browser", AdapterKind::tool, AdapterState::declared, {"read"}};
    auto definition = agent();
    definition.approval_mode = ApprovalMode::never;
    definition.tool_grants[0].network_allowed = false;
    check(prepare(definition, original, tool).state == RunState::ready,
          "valid local static preflight was rejected");
    for (const auto mode : {ApprovalMode::risky_actions, ApprovalMode::always}) {
        auto candidate = definition;
        candidate.approval_mode = mode;
        check(prepare(candidate, original, tool).state == RunState::pending_approval,
              "unclassified action escaped approval");
    }
    auto plan = original;
    plan.steps[0].requires_approval = true;
    check(prepare(definition, plan, tool).state == RunState::pending_approval,
          "never mode bypassed mandatory step review");
    for (bool network : {false, true}) {
        auto candidate = definition;
        candidate.tool_grants[0].network_allowed = network;
        candidate.tool_grants[0].filesystem_write_allowed = !network;
        check(prepare(candidate, original, tool).state == RunState::pending_approval,
              "effect-capable grant escaped review");
    }
    for (const auto setup : {ToolSetup::missing, ToolSetup::declared,
                             ToolSetup::observed, ToolSetup::expired}) {
        check(prepare(definition, original, tool, setup).state == RunState::denied,
              "missing, unqualified or expired adapter passed preflight");
    }
    auto mismatch = tool;
    mismatch.capabilities = {"write"};
    check(prepare(definition, original, mismatch).state == RunState::denied,
          "grant bypassed adapter capability mismatch");
    mismatch = tool;
    mismatch.kind = AdapterKind::model;
    check(prepare(definition, original, mismatch).state == RunState::denied,
          "model descriptor was accepted as a tool");
    plan = original;
    plan.steps.push_back(plan.steps.front());
    check(prepare(definition, plan, tool).state == RunState::denied,
          "direct preflight bypassed duplicate-step validation");
    WorkflowRegistry workflows;
    check(!workflows.register_workflow(plan) && workflows.snapshot().empty(),
          "invalid workflow mutated registry");
    for (int field = 0; field < 4; ++field) {
        plan = original;
        if (field == 0) plan.workflow_id.clear();
        if (field == 1) plan.steps[0].step_id.clear();
        if (field == 2) plan.steps[0].tool_id.clear();
        if (field == 3) plan.steps[0].action.clear();
        check(prepare(definition, plan, tool).state == RunState::denied,
              "empty workflow field passed preflight");
    }
    plan = original;
    plan.steps.push_back({"fetch-2", "browser", "read", false});
    definition.limits.max_tool_calls = 1;
    const auto too_many_calls = prepare(definition, plan, tool);
    check(too_many_calls.state == RunState::denied
          && too_many_calls.diagnostic == "workflow exceeds the agent tool-call limit",
          "tool-call limit was not enforced independently");
    definition.limits.max_tool_calls = 2;
    check(prepare(definition, plan, tool).state == RunState::ready,
          "exact tool-call limit was rejected");
    definition.limits.max_steps = 1;
    check(prepare(definition, plan, tool).state == RunState::denied,
          "step limit was not enforced independently");
    auto invalid = agent();
    invalid.approval_mode = static_cast<ApprovalMode>(99);
    check(!AgentFactory{}.create(invalid), "invalid approval enum was accepted");
    invalid = agent();
    invalid.tool_grants[0].allowed_actions.insert("");
    check(!AgentFactory{}.create(invalid), "empty granted action was accepted");
    ToolRegistry invalid_tools;
    mismatch = tool;
    mismatch.state = static_cast<AdapterState>(99);
    check(!invalid_tools.declare(mismatch) && !invalid_tools.find("browser"),
          "invalid adapter enum mutated registry");
    TriggerRegistry triggers;
    check(!triggers.register_trigger({"invalid", static_cast<TriggerKind>(99), "event", false}),
          "invalid trigger enum was accepted");
    PlatformSnapshot snapshot;
    snapshot.owner_id = "preflight-owner";
    check(snapshot.agents.register_agent(agent()), "approval recovery fixture invalid");
    const auto restored = AgentPlatformStore::deserialize(AgentPlatformStore::serialize(snapshot));
    check(restored && restored->agents.find("daily-research"), "approval fixture recovery failed");
    check(prepare(*restored->agents.find("daily-research"), original, tool).state == RunState::pending_approval,
          "recovered policy bypassed review");

    auto unsandboxed = definition;
    unsandboxed.sandbox_required = false;
    check(prepare(unsandboxed, original, tool).state == RunState::denied,
          "unsandboxed agent passed preflight");

    AgentRegistry model_agents;
    check(model_agents.register_agent(definition), "model gate agent fixture invalid");
    ModelRouter unqualified_models;
    check(unqualified_models.declare({"local-default", AdapterKind::model,
                                      AdapterState::declared, {"inference"}}),
          "model gate declaration failed");
    ToolRegistry qualified_tools;
    qualify_adapter(qualified_tools, tool);
    check(AgentRuntime{}.prepare(request, model_agents, original,
                                 unqualified_models, qualified_tools).state == RunState::denied,
          "unqualified model adapter passed preflight");
}

void test_adapter_evidence_lifecycle() {
    using namespace genesis::agents;
    ToolRegistry tools;
    check(!tools.declare({"direct", AdapterKind::tool, AdapterState::qualified, {"read"}}),
          "caller-supplied qualification was accepted");
    check(!tools.declare({"default-state", AdapterKind::tool,
                          AdapterState::unavailable, {"read"}}),
          "unavailable adapter was accepted as a declaration");
    check(tools.declare({"browser", AdapterKind::tool,
                         AdapterState::declared, {"read"}}),
          "evidence lifecycle declaration failed");
    check(!tools.qualify("browser", "qualifier.browser", evidence("too-early"), 20, 100),
          "unobserved adapter was qualified");
    check(!tools.record_probe("browser", true, "observer.browser", "not-a-digest", 10),
          "malformed observation evidence was accepted");
    check(tools.record_probe("browser", true, "observer.browser",
                             evidence("observed"), 10),
          "valid observation evidence was rejected");
    check(!tools.qualify("browser", "observer.browser", evidence("same-party"), 20, 100),
          "observer self-qualified an adapter");
    check(tools.qualify("browser", "qualifier.browser", evidence("qualified"), 20, 100)
              && tools.is_qualified("browser", 20)
              && tools.is_qualified("browser", 100)
              && !tools.is_qualified("browser", 101),
          "qualification validity interval was not enforced");
    check(!tools.record_probe("browser", true, "observer.browser", evidence("stale"), 10),
          "non-advancing adapter evidence was accepted");
    check(tools.record_probe("browser", false, "observer.browser", evidence("failed"), 101)
              && tools.find("browser")->state == AdapterState::observed
              && !tools.is_qualified("browser", 101),
          "failed probe did not demote a qualified adapter");
    check(!tools.qualify("browser", "qualifier.browser", evidence("without-recovery"), 102, 200),
          "adapter requalified without a fresh successful probe");
    check(tools.record_probe("browser", true, "observer.browser", evidence("recovered"), 103)
              && tools.qualify("browser", "qualifier.browser", evidence("requalified"), 104, 200)
              && tools.is_qualified("browser", 150),
          "adapter did not recover through a fresh evidence cycle");
}
}

int main() {
    try {
        test_preflight_boundaries();
        test_adapter_evidence_lifecycle();
        genesis::agents::AgentFactory factory;
        std::vector<genesis::agents::ValidationIssue> issues;
        auto created = factory.create(agent(), &issues);
        check(created && issues.empty(), "valid agent was rejected");

        genesis::agents::AgentRegistry agents;
        check(agents.register_agent(*created) && !agents.register_agent(*created), "registry uniqueness failed");
        genesis::agents::ModelRouter models;
        qualify_adapter(models, {"local-default", genesis::agents::AdapterKind::model,
                                  genesis::agents::AdapterState::declared, {"inference"}});
        genesis::agents::ToolRegistry tools;
        qualify_adapter(tools, {"browser", genesis::agents::AdapterKind::tool,
                                 genesis::agents::AdapterState::declared, {"read"}});
        genesis::agents::WorkflowDefinition workflow{"morning", {{"fetch", "browser", "read", true}}};
        genesis::agents::AgentRuntime runtime;
        auto prepared = runtime.prepare({"run-1", "daily-research", "morning", evidence("input-1"), 50},
                                        agents, workflow, models, tools);
        check(prepared.state == genesis::agents::RunState::pending_approval, "approval gate was bypassed");
        workflow.steps[0].action = "write";
        auto denied = runtime.prepare({"run-2", "daily-research", "morning", evidence("input-2"), 50},
                                      agents, workflow, models, tools);
        check(denied.state == genesis::agents::RunState::denied, "ungranted action was accepted");

        genesis::agents::PlatformSnapshot snapshot;
        snapshot.owner_id = "operator-1";
        check(snapshot.agents.register_agent(agent()), "snapshot agent registration failed");
        check(snapshot.workflows.register_workflow({"morning", {{"fetch", "browser", "read", true}}}), "workflow registration failed");
        check(snapshot.triggers.register_trigger({"weekdays", genesis::agents::TriggerKind::schedule, "weekdays 09:00 Europe/London", false}), "trigger registration failed");
        const auto bytes = genesis::agents::AgentPlatformStore::serialize(snapshot);
        auto restored = genesis::agents::AgentPlatformStore::deserialize(bytes);
        check(restored && restored->agents.find("daily-research") && restored->workflows.find("morning")
              && restored->triggers.find("weekdays") && !restored->triggers.find("weekdays")->enabled,
              "platform snapshot round-trip failed");
        auto corrupt = bytes; corrupt[corrupt.size() / 2] ^= 1;
        check(!genesis::agents::AgentPlatformStore::deserialize(corrupt), "corrupt platform snapshot was accepted");
        auto noncanonical_boolean = bytes;
        noncanonical_boolean[noncanonical_boolean.size() - 64U - 8U] = 2;
        noncanonical_boolean.replace(noncanonical_boolean.size() - 64U, 64U,
            genesis::runtime::sha256(std::string_view{noncanonical_boolean}.substr(
                0, noncanonical_boolean.size() - 64U)));
        check(!genesis::agents::AgentPlatformStore::deserialize(noncanonical_boolean),
              "noncanonical persisted boolean was accepted");
        const auto store_root = std::filesystem::temp_directory_path() /
            ("genesis-agent-platform-test-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        check(std::filesystem::create_directory(store_root), "cannot reserve unique test directory");
        genesis::agents::AgentPlatformStore store(store_root);
        check(store.write(snapshot, "v1"), "platform snapshot write failed");
        check(store.read("operator-1", "v1").has_value(), "platform snapshot recovery failed");
        check(store.write(snapshot, "v1"), "identical snapshot write was not idempotent");
        genesis::agents::PlatformStoreError store_error;
        check(!store.read("../operator-1", "v1", &store_error)
              && store_error.code == genesis::agents::PlatformStoreErrorCode::invalid_identifier,
              "unsafe owner path was accepted");
        check(!store.write(snapshot, "../v2", &store_error)
              && store_error.code == genesis::agents::PlatformStoreErrorCode::invalid_identifier,
              "unsafe version path was accepted");
        check(!store.read("other-owner", "v1", &store_error)
              && store_error.code == genesis::agents::PlatformStoreErrorCode::not_found,
              "another owner recovered a snapshot");
        auto large_snapshot = snapshot;
        auto large_agent = agent();
        large_agent.agent_id = "large-agent";
        large_agent.purpose.assign(2048, 'x');
        check(large_snapshot.agents.register_agent(large_agent), "large agent fixture invalid");
        genesis::agents::AgentPlatformStore small_store(store_root, 1024);
        check(!small_store.write(large_snapshot, "oversized", &store_error)
              && store_error.code == genesis::agents::PlatformStoreErrorCode::invalid_snapshot,
              "oversized snapshot was accepted");
        check(snapshot.triggers.register_trigger({"manual", genesis::agents::TriggerKind::manual, "", false}),
              "second trigger registration failed");
        check(!store.write(snapshot, "v1"), "immutable platform version was overwritten");
        std::filesystem::remove_all(store_root);
        std::cout << "All agent platform tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
