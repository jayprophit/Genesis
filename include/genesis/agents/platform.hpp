#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace genesis::agents {

enum class AdapterKind { model, tool, identity, browser, user_interface };
enum class AdapterState { unavailable, declared, observed, qualified };
enum class TriggerKind { manual, schedule, event };
enum class RunState { pending_approval, ready, running, succeeded, failed, denied };
enum class ApprovalMode { never, risky_actions, always };

struct ResourceLimits final {
    std::uint64_t max_runtime_ms{60'000};
    std::size_t max_steps{32};
    std::size_t max_tool_calls{16};
    std::size_t max_memory_bytes{1'048'576};
};

struct ToolGrant final {
    std::string tool_id;
    std::set<std::string, std::less<>> allowed_actions;
    bool network_allowed{false};
    bool filesystem_write_allowed{false};
};

struct AgentDefinition final {
    std::string schema_version{"1.0"};
    std::string agent_id;
    std::string display_name;
    std::string purpose;
    std::string model_route;
    std::vector<ToolGrant> tool_grants;
    ApprovalMode approval_mode{ApprovalMode::risky_actions};
    ResourceLimits limits;
    bool sandbox_required{true};
};

struct ValidationIssue final {
    std::string field;
    std::string message;
};

[[nodiscard]] std::vector<ValidationIssue> validate(const AgentDefinition& definition);

class AgentFactory final {
public:
    [[nodiscard]] std::optional<AgentDefinition> create(AgentDefinition candidate,
                                                        std::vector<ValidationIssue>* issues = nullptr) const;
};

class AgentRegistry final {
public:
    [[nodiscard]] bool register_agent(AgentDefinition definition);
    [[nodiscard]] const AgentDefinition* find(std::string_view agent_id) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::vector<AgentDefinition> snapshot() const;
private:
    std::map<std::string, AgentDefinition, std::less<>> agents_;
};

struct AdapterDescriptor final {
    AdapterDescriptor() = default;
    AdapterDescriptor(std::string id,
                      AdapterKind adapter_kind,
                      AdapterState adapter_state,
                      std::set<std::string, std::less<>> adapter_capabilities)
        : adapter_id(std::move(id)),
          kind(adapter_kind),
          state(adapter_state),
          capabilities(std::move(adapter_capabilities)) {}

    std::string adapter_id;
    AdapterKind kind{AdapterKind::tool};
    AdapterState state{AdapterState::unavailable};
    std::set<std::string, std::less<>> capabilities;
    std::size_t successful_probes{};
    std::size_t failed_probes{};
    std::string last_observer_id;
    std::string last_probe_digest;
    std::uint64_t last_probe_at{};
    bool last_probe_succeeded{false};
    std::string qualifier_id;
    std::string qualification_digest;
    std::uint64_t qualified_at{};
    std::uint64_t valid_until{};
};

class AdapterRegistry final {
public:
    // Registration accepts declared adapters only. Observation and
    // qualification are evidence-bearing transitions, not caller labels.
    [[nodiscard]] bool declare(AdapterDescriptor adapter);
    [[nodiscard]] bool record_probe(std::string_view adapter_id,
                                    bool succeeded,
                                    std::string observer_id,
                                    std::string evidence_digest,
                                    std::uint64_t observed_at);
    [[nodiscard]] bool qualify(std::string_view adapter_id,
                               std::string qualifier_id,
                               std::string evidence_digest,
                               std::uint64_t qualified_at,
                               std::uint64_t valid_until);
    [[nodiscard]] const AdapterDescriptor* find(std::string_view adapter_id) const noexcept;
    [[nodiscard]] bool is_qualified(std::string_view adapter_id,
                                    std::uint64_t at) const noexcept;
private:
    std::map<std::string, AdapterDescriptor, std::less<>> adapters_;
};

using ModelRouter = AdapterRegistry;
using ToolRegistry = AdapterRegistry;

struct WorkflowStep final {
    std::string step_id;
    std::string tool_id;
    std::string action;
    bool requires_approval{false};
};

struct WorkflowDefinition final {
    std::string workflow_id;
    std::vector<WorkflowStep> steps;
};

[[nodiscard]] std::vector<ValidationIssue> validate(const WorkflowDefinition& definition);

class WorkflowRegistry final {
public:
    [[nodiscard]] bool register_workflow(WorkflowDefinition definition);
    [[nodiscard]] const WorkflowDefinition* find(std::string_view workflow_id) const noexcept;
    [[nodiscard]] std::vector<WorkflowDefinition> snapshot() const;
private:
    std::map<std::string, WorkflowDefinition, std::less<>> workflows_;
};

struct TriggerDefinition final {
    std::string trigger_id;
    TriggerKind kind{TriggerKind::manual};
    std::string expression;
    bool enabled{false};
};

class TriggerRegistry final {
public:
    [[nodiscard]] bool register_trigger(TriggerDefinition definition);
    [[nodiscard]] const TriggerDefinition* find(std::string_view trigger_id) const noexcept;
    [[nodiscard]] std::vector<TriggerDefinition> snapshot() const;
private:
    std::map<std::string, TriggerDefinition, std::less<>> triggers_;
};

struct RunRequest final {
    std::string run_id;
    std::string agent_id;
    std::string workflow_id;
    std::string input_digest;
    std::uint64_t prepared_at{};
};

struct RunRecord final {
    RunRequest request;
    RunState state{RunState::pending_approval};
    std::size_t completed_steps{};
    std::string diagnostic;
};

class AgentRuntime final {
public:
    // Static preflight only. Ready does not authorize execution. In this
    // scaffold each step is one tool call and action risk is unclassified;
    // risky_actions therefore requires approval for every tool action.
    [[nodiscard]] RunRecord prepare(const RunRequest& request,
                                    const AgentRegistry& agents,
                                    const WorkflowDefinition& workflow,
                                    const ModelRouter& models,
                                    const ToolRegistry& tools) const;
};

} // namespace genesis::agents
