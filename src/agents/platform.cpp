#include "genesis/agents/platform.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace genesis::agents {
namespace {

bool evidence_digest(std::string_view value) {
    return value.size() == 64U
        && std::all_of(value.begin(), value.end(), [](unsigned char character) {
               return std::isxdigit(character) != 0;
           });
}

bool identifier(std::string_view value) {
    return !value.empty() && value.size() <= 128U
        && std::isalnum(static_cast<unsigned char>(value.front())) != 0
        && std::all_of(value.begin(), value.end(), [](unsigned char character) {
               return std::isalnum(character) != 0 || character == '_'
                   || character == '-' || character == '.';
           });
}

bool bounded_text(std::string_view value, std::size_t maximum_size) {
    return !value.empty() && value.size() <= maximum_size
        && std::all_of(value.begin(), value.end(), [](unsigned char character) {
               return character >= 0x20U && character != 0x7fU;
           });
}

void clear_qualification(AdapterDescriptor& adapter) {
    adapter.qualifier_id.clear();
    adapter.qualification_digest.clear();
    adapter.qualified_at = 0;
    adapter.valid_until = 0;
}

} // namespace

std::vector<ValidationIssue> validate(const AgentDefinition& value) {
    std::vector<ValidationIssue> issues;
    if (value.schema_version != "1.0") issues.push_back({"schema_version", "unsupported schema version"});
    if (!identifier(value.agent_id)) issues.push_back({"agent_id", "must be a bounded canonical identifier"});
    if (!bounded_text(value.display_name, 256U)) issues.push_back({"display_name", "must be bounded printable text"});
    if (!bounded_text(value.purpose, 4096U)) issues.push_back({"purpose", "must be bounded printable text"});
    if (!identifier(value.model_route)) issues.push_back({"model_route", "must be a bounded canonical identifier"});
    if (value.approval_mode != ApprovalMode::never
        && value.approval_mode != ApprovalMode::risky_actions
        && value.approval_mode != ApprovalMode::always)
        issues.push_back({"approval_mode", "unsupported approval mode"});
    if (value.limits.max_runtime_ms == 0) issues.push_back({"limits.max_runtime_ms", "must be positive"});
    if (value.limits.max_steps == 0) issues.push_back({"limits.max_steps", "must be positive"});
    if (value.limits.max_tool_calls == 0) issues.push_back({"limits.max_tool_calls", "must be positive"});
    if (value.limits.max_memory_bytes == 0) issues.push_back({"limits.max_memory_bytes", "must be positive"});
    std::set<std::string, std::less<>> ids;
    for (const auto& grant : value.tool_grants) {
        if (!identifier(grant.tool_id)) issues.push_back({"tool_grants.tool_id", "must be a bounded canonical identifier"});
        else if (!ids.insert(grant.tool_id).second) issues.push_back({"tool_grants", "duplicate tool grant"});
        if (grant.allowed_actions.empty()) issues.push_back({"tool_grants.allowed_actions", "must not be empty"});
        if (std::any_of(grant.allowed_actions.begin(), grant.allowed_actions.end(),
                        [](const auto& action) { return !identifier(action); }))
            issues.push_back({"tool_grants.allowed_actions", "actions must be bounded canonical identifiers"});
    }
    return issues;
}

std::optional<AgentDefinition> AgentFactory::create(AgentDefinition candidate,
                                                    std::vector<ValidationIssue>* issues) const {
    auto found = validate(candidate);
    if (issues) *issues = found;
    if (!found.empty()) return std::nullopt;
    return candidate;
}

bool AgentRegistry::register_agent(AgentDefinition definition) {
    if (!validate(definition).empty()) return false;
    return agents_.emplace(definition.agent_id, std::move(definition)).second;
}

const AgentDefinition* AgentRegistry::find(std::string_view id) const noexcept {
    const auto found = agents_.find(id);
    return found == agents_.end() ? nullptr : &found->second;
}

std::size_t AgentRegistry::size() const noexcept { return agents_.size(); }

std::vector<AgentDefinition> AgentRegistry::snapshot() const {
    std::vector<AgentDefinition> result;
    result.reserve(agents_.size());
    for (const auto& [_, definition] : agents_) result.push_back(definition);
    return result;
}

bool AdapterRegistry::declare(AdapterDescriptor adapter) {
    if (!identifier(adapter.adapter_id) || adapter.state != AdapterState::declared
        || adapter.capabilities.empty()
        || std::any_of(adapter.capabilities.begin(), adapter.capabilities.end(),
                       [](const auto& capability) { return !identifier(capability); })
        || adapter.successful_probes != 0 || adapter.failed_probes != 0
        || !adapter.last_observer_id.empty() || !adapter.last_probe_digest.empty()
        || adapter.last_probe_at != 0 || adapter.last_probe_succeeded
        || !adapter.qualifier_id.empty()
        || !adapter.qualification_digest.empty() || adapter.qualified_at != 0
        || adapter.valid_until != 0) return false;
    switch (adapter.kind) {
    case AdapterKind::model: case AdapterKind::tool: case AdapterKind::identity:
    case AdapterKind::browser: case AdapterKind::user_interface: break;
    default: return false;
    }
    return adapters_.emplace(adapter.adapter_id, std::move(adapter)).second;
}

bool AdapterRegistry::record_probe(std::string_view id,
                                   bool succeeded,
                                   std::string observer_id,
                                   std::string digest,
                                   std::uint64_t observed_at) {
    const auto found = adapters_.find(id);
    if (found == adapters_.end() || !identifier(observer_id)
        || !evidence_digest(digest) || observed_at == 0
        || observed_at <= found->second.last_probe_at) return false;
    auto& adapter = found->second;
    if (adapter.state != AdapterState::declared
        && adapter.state != AdapterState::observed
        && adapter.state != AdapterState::qualified) return false;
    adapter.last_observer_id = std::move(observer_id);
    adapter.last_probe_digest = std::move(digest);
    adapter.last_probe_at = observed_at;
    adapter.last_probe_succeeded = succeeded;
    if (succeeded) {
        ++adapter.successful_probes;
        if (adapter.state == AdapterState::declared) adapter.state = AdapterState::observed;
    } else {
        ++adapter.failed_probes;
        if (adapter.state == AdapterState::qualified) {
            adapter.state = AdapterState::observed;
            clear_qualification(adapter);
        }
    }
    return true;
}

bool AdapterRegistry::qualify(std::string_view id,
                              std::string qualifier_id,
                              std::string digest,
                              std::uint64_t qualified_at,
                              std::uint64_t valid_until) {
    const auto found = adapters_.find(id);
    if (found == adapters_.end()) return false;
    auto& adapter = found->second;
    if (adapter.state != AdapterState::observed || adapter.successful_probes == 0
        || !adapter.last_probe_succeeded
        || !identifier(qualifier_id) || qualifier_id == adapter.last_observer_id
        || !evidence_digest(digest) || qualified_at < adapter.last_probe_at
        || valid_until <= qualified_at) return false;
    adapter.qualifier_id = std::move(qualifier_id);
    adapter.qualification_digest = std::move(digest);
    adapter.qualified_at = qualified_at;
    adapter.valid_until = valid_until;
    adapter.state = AdapterState::qualified;
    return true;
}

const AdapterDescriptor* AdapterRegistry::find(std::string_view id) const noexcept {
    const auto found = adapters_.find(id);
    return found == adapters_.end() ? nullptr : &found->second;
}

bool AdapterRegistry::is_qualified(std::string_view id, std::uint64_t at) const noexcept {
    const auto* adapter = find(id);
    return adapter && at != 0 && adapter->state == AdapterState::qualified
        && adapter->successful_probes != 0
        && identifier(adapter->last_observer_id)
        && evidence_digest(adapter->last_probe_digest)
        && adapter->last_probe_succeeded
        && identifier(adapter->qualifier_id)
        && adapter->qualifier_id != adapter->last_observer_id
        && evidence_digest(adapter->qualification_digest)
        && adapter->qualified_at >= adapter->last_probe_at
        && adapter->valid_until > adapter->qualified_at
        && at >= adapter->qualified_at && at <= adapter->valid_until;
}

std::vector<ValidationIssue> validate(const WorkflowDefinition& definition) {
    std::vector<ValidationIssue> issues;
    if (!identifier(definition.workflow_id)) issues.push_back({"workflow_id", "must be a bounded canonical identifier"});
    if (definition.steps.empty()) issues.push_back({"steps", "must not be empty"});
    std::set<std::string, std::less<>> step_ids;
    for (const auto& step : definition.steps) {
        if (!identifier(step.step_id)) issues.push_back({"steps.step_id", "must be a bounded canonical identifier"});
        else if (!step_ids.insert(step.step_id).second) issues.push_back({"steps.step_id", "duplicate step ID"});
        if (!identifier(step.tool_id)) issues.push_back({"steps.tool_id", "must be a bounded canonical identifier"});
        if (!identifier(step.action)) issues.push_back({"steps.action", "must be a bounded canonical identifier"});
    }
    return issues;
}

bool WorkflowRegistry::register_workflow(WorkflowDefinition definition) {
    if (!validate(definition).empty()) return false;
    return workflows_.emplace(definition.workflow_id, std::move(definition)).second;
}

const WorkflowDefinition* WorkflowRegistry::find(std::string_view id) const noexcept {
    const auto found = workflows_.find(id);
    return found == workflows_.end() ? nullptr : &found->second;
}

std::vector<WorkflowDefinition> WorkflowRegistry::snapshot() const {
    std::vector<WorkflowDefinition> result;
    result.reserve(workflows_.size());
    for (const auto& [_, definition] : workflows_) result.push_back(definition);
    return result;
}

bool TriggerRegistry::register_trigger(TriggerDefinition definition) {
    if (!identifier(definition.trigger_id)) return false;
    if (definition.kind != TriggerKind::manual && definition.kind != TriggerKind::schedule
        && definition.kind != TriggerKind::event) return false;
    if (definition.kind != TriggerKind::manual
        && !bounded_text(definition.expression, 4096U)) return false;
    if (definition.kind == TriggerKind::manual && !definition.expression.empty()
        && !bounded_text(definition.expression, 4096U)) return false;
    return triggers_.emplace(definition.trigger_id, std::move(definition)).second;
}

const TriggerDefinition* TriggerRegistry::find(std::string_view id) const noexcept {
    const auto found = triggers_.find(id);
    return found == triggers_.end() ? nullptr : &found->second;
}

std::vector<TriggerDefinition> TriggerRegistry::snapshot() const {
    std::vector<TriggerDefinition> result;
    result.reserve(triggers_.size());
    for (const auto& [_, definition] : triggers_) result.push_back(definition);
    return result;
}

RunRecord AgentRuntime::prepare(const RunRequest& request,
                                const AgentRegistry& agents,
                                const WorkflowDefinition& workflow,
                                const ModelRouter& models,
                                const ToolRegistry& tools) const {
    RunRecord result{request, RunState::denied, 0, {}};
    const auto* agent = agents.find(request.agent_id);
    if (!agent) { result.diagnostic = "agent is not registered"; return result; }
    if (!identifier(request.run_id) || !evidence_digest(request.input_digest)
        || request.prepared_at == 0) {
        result.diagnostic = "canonical run ID, input digest and preparation time are required"; return result;
    }
    if (workflow.workflow_id != request.workflow_id) {
        result.diagnostic = "workflow is missing or mismatched"; return result;
    }
    if (workflow.steps.size() > agent->limits.max_steps) {
        result.diagnostic = "workflow exceeds the agent step limit"; return result;
    }
    if (workflow.steps.size() > agent->limits.max_tool_calls) {
        result.diagnostic = "workflow exceeds the agent tool-call limit"; return result;
    }
    if (!validate(workflow).empty()) {
        result.diagnostic = "workflow structure is invalid"; return result;
    }
    const auto* model = models.find(agent->model_route);
    if (!model || model->kind != AdapterKind::model
        || !models.is_qualified(agent->model_route, request.prepared_at)) {
        result.diagnostic = "agent requires an evidence-qualified model adapter"; return result;
    }
    if (!agent->sandbox_required) {
        result.diagnostic = "agent does not require sandboxing"; return result;
    }
    // No trusted per-action risk classifier exists yet. Unclassified actions
    // conservatively require review under risky_actions, including on recovery.
    bool approval = agent->approval_mode != ApprovalMode::never;
    for (const auto& step : workflow.steps) {
        const auto* tool = tools.find(step.tool_id);
        if (!tool || tool->kind != AdapterKind::tool
            || !tools.is_qualified(step.tool_id, request.prepared_at)) {
            result.diagnostic = "workflow requires an unqualified tool adapter"; return result;
        }
        if (!tool->capabilities.contains(step.action)) {
            result.diagnostic = "tool does not declare the requested action"; return result;
        }
        const auto grant = std::find_if(agent->tool_grants.begin(), agent->tool_grants.end(),
            [&](const ToolGrant& value) { return value.tool_id == step.tool_id && value.allowed_actions.contains(step.action); });
        if (grant == agent->tool_grants.end()) {
            result.diagnostic = "workflow action is outside the agent grant"; return result;
        }
        approval = approval || step.requires_approval
            || grant->network_allowed || grant->filesystem_write_allowed;
    }
    result.state = approval ? RunState::pending_approval : RunState::ready;
    result.diagnostic = approval ? "human approval required; execution remains unavailable"
                                 : "static preflight passed; execution remains unavailable";
    return result;
}

} // namespace genesis::agents
