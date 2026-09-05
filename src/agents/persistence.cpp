#include "genesis/agents/persistence.hpp"

#include "genesis/common/immutable_snapshot.hpp"
#include "genesis/runtime/runtime.hpp"

#include <cctype>
#include <stdexcept>
#include <utility>

namespace genesis::agents { namespace {
constexpr std::string_view magic = "GENESIS-AGENT-PLATFORM-V1";
constexpr std::uint64_t maximum_items = 100'000;
constexpr std::uint64_t maximum_field = 4U * 1024U * 1024U;
void error(PlatformStoreError* target, PlatformStoreErrorCode code, std::string message) {
    if (target) { target->code = code; target->message = std::move(message); }
}
void put_u64(std::string& out, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64; shift += 8) out.push_back(static_cast<char>((value >> shift) & 255));
}
std::uint64_t get_u64(std::string_view bytes, std::size_t& offset) {
    if (offset > bytes.size() || bytes.size() - offset < 8) throw std::runtime_error("truncated integer");
    std::uint64_t value{};
    for (unsigned shift = 0; shift < 64; shift += 8)
        value |= static_cast<std::uint64_t>(static_cast<unsigned char>(bytes[offset++])) << shift;
    return value;
}
bool get_bool(std::string_view bytes, std::size_t& offset) {
    const auto value = get_u64(bytes, offset);
    if (value > 1U) throw std::runtime_error("invalid boolean value");
    return value == 1U;
}
void put_string(std::string& out, std::string_view value) { put_u64(out, value.size()); out.append(value); }
std::string get_string(std::string_view bytes, std::size_t& offset) {
    const auto size = get_u64(bytes, offset);
    if (size > maximum_field || size > bytes.size() - offset) throw std::runtime_error("invalid field length");
    std::string value{bytes.substr(offset, size)}; offset += size; return value;
}
bool safe_id(std::string_view value) {
    if (value.empty() || value.size() > 128 || value == "." || value == "..") return false;
    for (unsigned char c : value) if (!std::isalnum(c) && c != '_' && c != '-' && c != '.') return false;
    return true;
}
}

AgentPlatformStore::AgentPlatformStore(std::filesystem::path root, std::size_t maximum_record_bytes)
    : root_(std::move(root)), maximum_record_bytes_(maximum_record_bytes) {
    if (root_.empty() || maximum_record_bytes_ < 1024) throw std::invalid_argument("invalid agent platform store configuration");
}

std::string AgentPlatformStore::serialize(const PlatformSnapshot& value) {
    if (!safe_id(value.owner_id)) throw std::invalid_argument("invalid platform owner ID");
    std::string out{magic}; put_string(out, value.owner_id);
    const auto agents = value.agents.snapshot(); put_u64(out, agents.size());
    for (const auto& agent : agents) {
        if (!validate(agent).empty()) throw std::invalid_argument("invalid agent definition");
        put_string(out, agent.schema_version); put_string(out, agent.agent_id); put_string(out, agent.display_name);
        put_string(out, agent.purpose); put_string(out, agent.model_route); put_u64(out, static_cast<std::uint64_t>(agent.approval_mode));
        put_u64(out, agent.limits.max_runtime_ms); put_u64(out, agent.limits.max_steps); put_u64(out, agent.limits.max_tool_calls);
        put_u64(out, agent.limits.max_memory_bytes); put_u64(out, agent.sandbox_required); put_u64(out, agent.tool_grants.size());
        for (const auto& grant : agent.tool_grants) {
            put_string(out, grant.tool_id); put_u64(out, grant.network_allowed); put_u64(out, grant.filesystem_write_allowed);
            put_u64(out, grant.allowed_actions.size()); for (const auto& action : grant.allowed_actions) put_string(out, action);
        }
    }
    const auto workflows = value.workflows.snapshot(); put_u64(out, workflows.size());
    for (const auto& workflow : workflows) {
        put_string(out, workflow.workflow_id); put_u64(out, workflow.steps.size());
        for (const auto& step : workflow.steps) { put_string(out, step.step_id); put_string(out, step.tool_id); put_string(out, step.action); put_u64(out, step.requires_approval); }
    }
    const auto triggers = value.triggers.snapshot(); put_u64(out, triggers.size());
    for (const auto& trigger : triggers) { put_string(out, trigger.trigger_id); put_u64(out, static_cast<std::uint64_t>(trigger.kind)); put_string(out, trigger.expression); put_u64(out, trigger.enabled); }
    out += runtime::sha256(out); return out;
}

std::optional<PlatformSnapshot> AgentPlatformStore::deserialize(std::string_view bytes, PlatformStoreError* target) {
    try {
        if (bytes.size() < magic.size() + 64 || bytes.substr(0, magic.size()) != magic) throw std::runtime_error("invalid platform magic");
        const auto payload = bytes.substr(0, bytes.size() - 64);
        if (runtime::sha256(payload) != bytes.substr(bytes.size() - 64)) throw std::runtime_error("platform checksum mismatch");
        std::size_t offset = magic.size(); PlatformSnapshot value; value.owner_id = get_string(payload, offset);
        if (!safe_id(value.owner_id)) throw std::runtime_error("invalid platform owner ID");
        auto count = get_u64(payload, offset); if (count > maximum_items) throw std::runtime_error("too many agents");
        while (count--) {
            AgentDefinition agent; agent.schema_version = get_string(payload, offset); agent.agent_id = get_string(payload, offset);
            agent.display_name = get_string(payload, offset); agent.purpose = get_string(payload, offset); agent.model_route = get_string(payload, offset);
            const auto approval = get_u64(payload, offset); if (approval > static_cast<std::uint64_t>(ApprovalMode::always)) throw std::runtime_error("invalid approval mode");
            agent.approval_mode = static_cast<ApprovalMode>(approval); agent.limits.max_runtime_ms = get_u64(payload, offset);
            agent.limits.max_steps = get_u64(payload, offset); agent.limits.max_tool_calls = get_u64(payload, offset);
            agent.limits.max_memory_bytes = get_u64(payload, offset); agent.sandbox_required = get_bool(payload, offset);
            auto grants = get_u64(payload, offset); if (grants > maximum_items) throw std::runtime_error("too many grants");
            while (grants--) { ToolGrant grant; grant.tool_id = get_string(payload, offset); grant.network_allowed = get_bool(payload, offset); grant.filesystem_write_allowed = get_bool(payload, offset); auto actions = get_u64(payload, offset); if (actions > maximum_items) throw std::runtime_error("too many actions"); while (actions--) grant.allowed_actions.insert(get_string(payload, offset)); agent.tool_grants.push_back(std::move(grant)); }
            if (!value.agents.register_agent(std::move(agent))) throw std::runtime_error("invalid or duplicate agent");
        }
        count = get_u64(payload, offset); if (count > maximum_items) throw std::runtime_error("too many workflows");
        while (count--) { WorkflowDefinition workflow; workflow.workflow_id = get_string(payload, offset); auto steps = get_u64(payload, offset); if (steps > maximum_items) throw std::runtime_error("too many steps"); while (steps--) workflow.steps.push_back({get_string(payload, offset), get_string(payload, offset), get_string(payload, offset), get_bool(payload, offset)}); if (!value.workflows.register_workflow(std::move(workflow))) throw std::runtime_error("invalid or duplicate workflow"); }
        count = get_u64(payload, offset); if (count > maximum_items) throw std::runtime_error("too many triggers");
        while (count--) { TriggerDefinition trigger; trigger.trigger_id = get_string(payload, offset); const auto kind = get_u64(payload, offset); if (kind > static_cast<std::uint64_t>(TriggerKind::event)) throw std::runtime_error("invalid trigger kind"); trigger.kind = static_cast<TriggerKind>(kind); trigger.expression = get_string(payload, offset); trigger.enabled = get_bool(payload, offset); if (!value.triggers.register_trigger(std::move(trigger))) throw std::runtime_error("invalid or duplicate trigger"); }
        if (offset != payload.size()) throw std::runtime_error("trailing platform data");
        error(target, PlatformStoreErrorCode::none, {}); return value;
    } catch (const std::exception& exception) { error(target, PlatformStoreErrorCode::corrupt_record, exception.what()); return std::nullopt; }
}

bool AgentPlatformStore::write(const PlatformSnapshot& value, std::string_view version, PlatformStoreError* target) const {
    try {
        if (!safe_id(value.owner_id) || !safe_id(version)) { error(target, PlatformStoreErrorCode::invalid_identifier, "unsafe owner or version identifier"); return false; }
        const auto bytes = serialize(value); if (bytes.size() > maximum_record_bytes_) { error(target, PlatformStoreErrorCode::invalid_snapshot, "platform snapshot exceeds configured limit"); return false; }
        storage::ImmutableSnapshotFiles files(root_, maximum_record_bytes_); storage::ImmutableFileError file_error;
        if (!files.write(value.owner_id, version, ".agents", bytes, &file_error)) {
            const auto code = file_error.code == storage::ImmutableFileErrorCode::conflicting_version ? PlatformStoreErrorCode::conflicting_version : PlatformStoreErrorCode::io_error;
            error(target, code, file_error.message); return false;
        }
        error(target, PlatformStoreErrorCode::none, {}); return true;
    } catch (const std::invalid_argument& exception) { error(target, PlatformStoreErrorCode::invalid_snapshot, exception.what()); return false; }
}

std::optional<PlatformSnapshot> AgentPlatformStore::read(std::string_view owner, std::string_view version, PlatformStoreError* target) const {
    if (!safe_id(owner) || !safe_id(version)) { error(target, PlatformStoreErrorCode::invalid_identifier, "unsafe owner or version identifier"); return std::nullopt; }
    storage::ImmutableSnapshotFiles files(root_, maximum_record_bytes_); storage::ImmutableFileError file_error;
    auto bytes = files.read(owner, version, ".agents", &file_error);
    if (!bytes) { error(target, file_error.code == storage::ImmutableFileErrorCode::not_found ? PlatformStoreErrorCode::not_found : PlatformStoreErrorCode::io_error, file_error.message); return std::nullopt; }
    auto value = deserialize(*bytes, target); if (value && value->owner_id != owner) { error(target, PlatformStoreErrorCode::corrupt_record, "platform owner mismatch"); return std::nullopt; }
    return value;
}

} // namespace genesis::agents
