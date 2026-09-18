#pragma once

#include "genesis/memory/graph.hpp"
#include "genesis/memory/persistence.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace genesis::ledger {

// -- Bridge Ledger Data Types -------------------------------------------------

enum class LedgerOrigin {
    kDirectExperience,
    kTaught,
    kObserved,
    kInferred,
    kSimulated,
    kGenerated,
    kExternalSource,
    kInheritedParentA,
    kInheritedParentB,
};

enum class EvidenceKind {
    kObservation,
    kVerifiedResult,
    kTestResult,
    kBenchmark,
    kWorkProof,
    kProblemRecord,
    kResourceSnapshot,
};

struct ProblemRecord final {
    std::string problem_id;
    std::string project;
    std::string component;
    std::string symptoms;
    std::string error;
    std::string model;
    std::string workaround;
    std::string root_cause;
    double root_cause_confidence{};
    std::string result;  // WORKAROUND, FIXED, OPEN, WONT_FIX
    std::chrono::system_clock::time_point timestamp;
};

struct WorkProof final {
    std::string proof_id;
    std::string component_id;
    std::string component_version;
    std::string task_id;
    std::string builder_agent;
    std::string builder_model;
    std::string input_hash;
    std::string result_hash;
    std::string git_commit;
    std::string previous_version_proof;
    std::chrono::system_clock::time_point timestamp;
};

struct EvidenceRecord final {
    std::string evidence_id;
    std::string task_id;
    std::string kind;  // maps to EvidenceKind
    std::string producer;
    std::unordered_map<std::string, std::string> payload;
    bool verified{false};
    double confidence{};
    std::chrono::system_clock::time_point timestamp;
};

struct ResourceSnapshot final {
    std::string task_id;
    double wall_clock_s{};
    double cpu_util_percent{};
    double gpu_util_percent{};
    double ram_mb{};
    double vram_mb{};
    double tokens_per_sec{};
    std::size_t input_tokens{};
    std::size_t output_tokens{};
    std::size_t tool_calls{};
    std::size_t retries{};
    std::size_t timeouts{};
};

struct ProblemRecordBridge final {
    std::string problem_id;
    std::string project;
    std::string component;
    std::string symptoms;
    std::string error;
    std::string model;
    std::string workaround;
    std::string root_cause;
    double root_cause_confidence{};
    std::string result;
    std::chrono::system_clock::time_point timestamp;
};

struct WorkProofBridge final {
    std::string proof_id;
    std::string component_id;
    std::string component_version;
    std::string task_id;
    std::string builder_agent;
    std::string builder_model;
    std::string input_hash;
    std::string result_hash;
    std::string git_commit;
    std::string previous_version_proof;
    std::chrono::system_clock::time_point timestamp;
};

struct EvidenceBridge final {
    std::string evidence_id;
    std::string task_id;
    std::string kind;
    std::string producer;
    std::unordered_map<std::string, std::string> payload;
    bool verified{false};
    double confidence{};
    std::chrono::system_clock::time_point timestamp;
};

struct ResourceSnapshotBridge final {
    std::string task_id;
    double wall_clock_s{};
    double cpu_util_percent{};
    double gpu_util_percent{};
    double ram_mb{};
    double vram_mb{};
    double tokens_per_sec{};
    std::size_t input_tokens{};
    std::size_t output_tokens{};
    std::size_t tool_calls{};
    std::size_t retries{};
    std::size_t timeouts{};
};

struct TaskRecordBridge final {
    std::string task_id;
    std::string objective;
    std::vector<std::string> dependencies;
    std::string status;  // PLANNED, QUEUED, RUNNING, VERIFYING, VERIFIED_COMPLETE, FAILED, BLOCKED, CANCELLED
    std::string next_action;
};

struct TaskCenterNodeBridge final {
    std::string task_id;
    std::string parent_id;
    std::string objective;
    std::string status;
    std::string supervisor;
    std::string assigned_agent;
    std::string assigned_model;
    std::string provider;
    std::string execution_mode;
    std::vector<std::string> sources;
    std::string next_action;
    double progress_pct{};
    std::vector<std::string> children;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
};

struct ResourceSnapshotBridgeDetail final {
    std::string task_id;
    double wall_clock_s{};
    double cpu_util_percent{};
    double gpu_util_percent{};
    double ram_mb{};
    double vram_mb{};
    double tokens_per_sec{};
    std::size_t input_tokens{};
    std::size_t output_tokens{};
    std::size_t tool_calls{};
    std::size_t retries{};
    std::size_t timeouts{};
    std::chrono::system_clock::time_point timestamp;
};

struct WorkProofChain final {
    std::vector<WorkProofBridge> proofs;
    bool ok{true};
    std::vector<std::string> issues;
};

// -- Configuration ------------------------------------------------------------

struct LedgerConfig final {
    std::string project_id = "genesis";
    std::string component_id = "genesis-memory";
    std::string storage_root = "E:/OpenCode-Data/Genesis";
    bool enable_persistence = true;
    double checkpoint_interval_s = 30.0;
    std::size_t max_memory_nodes = 10000;
    std::size_t max_memory_edges = 50000;
};

// -- Genesis Ledger Bridge Interface ------------------------------------------

class ILedgerBridge {
public:
    virtual ~ILedgerBridge() = default;

    // -- Initialization -------------------------------------------------------
    virtual bool initialize() = 0;
    virtual bool is_initialized() const noexcept = 0;

    // -- ProblemMemory Integration -------------------------------------------
    virtual std::vector<ProblemRecordBridge> query_problem_memory(
        std::string_view error_contains = "",
        std::string_view component = "") = 0;

    virtual std::string record_problem(
        std::string_view symptoms,
        std::string_view error,
        std::string_view component,
        std::string_view model = "",
        std::string_view workaround = "") = 0;

    // -- WorkProofLedger Integration ------------------------------------------
    virtual std::string record_work_proof(
        std::string_view component_id,
        std::string_view component_version,
        std::string_view task_id,
        std::string_view builder_agent,
        std::string_view builder_model,
        const std::unordered_map<std::string, std::string>& source_hashes,
        std::string_view input_hash,
        std::string_view result_hash,
        std::string_view git_commit = "",
        const std::vector<std::string>& test_evidence = {},
        const std::vector<std::string>& benchmark_evidence = {},
        const std::vector<std::string>& resource_evidence = {},
        std::string_view previous_version_proof = "") = 0;

    virtual std::optional<WorkProofBridge> get_latest_work_proof(std::string_view component_id) = 0;
    virtual std::vector<WorkProofBridge> get_work_proof_chain(std::string_view component_id) = 0;

    // -- EvidenceLedger Integration -------------------------------------------
    virtual std::string add_evidence(
        std::string_view task_id,
        std::string_view kind,
        const std::unordered_map<std::string, std::string>& payload,
        bool verified = false,
        std::string_view producer = "genesis-memory") = 0;

    virtual std::vector<EvidenceBridge> get_verified_results(std::string_view task_id = "") = 0;
    virtual bool mark_evidence_verified(std::string_view evidence_id) = 0;

    // -- ResourceLedger Integration -------------------------------------------
    virtual void record_resource_usage(
        std::string_view task_id,
        const ResourceSnapshotBridge& snapshot) = 0;

    // -- TaskCenter Integration -----------------------------------------------
    virtual std::string task_center_add(
        std::string_view objective,
        std::string_view status = "PLANNED",
        std::string_view checkpoint = "") = 0;

    virtual std::optional<TaskCenterNodeBridge> task_center_get(std::string_view task_id) = 0;
    virtual bool task_center_update_progress(std::string_view task_id, double progress_pct) = 0;
    virtual bool task_center_complete(std::string_view task_id,
                                      std::string_view checkpoint = "") = 0;

    // -- DurableTaskGraph Integration -----------------------------------------
    virtual std::string task_graph_add(
        std::string_view task_id,
        std::string_view objective,
        const std::vector<std::string>& dependencies,
        std::string_view status,
        std::string_view next_action) = 0;

    virtual std::vector<TaskRecordBridge> task_graph_ready() = 0;
    virtual std::vector<TaskRecordBridge> task_graph_ordered() = 0;

    // -- CheckpointManager Integration ----------------------------------------
    virtual std::string checkpoint() = 0;
    virtual bool should_checkpoint() const noexcept = 0;

    // -- Recovery Integration -------------------------------------------------
    virtual std::vector<std::unordered_map<std::string, std::string>> plan_recovery() = 0;

    // -- Persistence ----------------------------------------------------------
    virtual bool save_all() = 0;
    virtual bool save_checkpoint() = 0;

    // -- Genesis Memory Record Operations -------------------------------------
    virtual std::string memory_record_to_evidence(
        const memory::MemoryNode& node,
        std::string_view organism_id) = 0;

    virtual std::optional<memory::MemoryNode> evidence_to_memory_record(
        const EvidenceBridge& evidence,
        std::string_view organism_id) = 0;
};

// -- Factory ------------------------------------------------------------------

std::unique_ptr<ILedgerBridge> create_ledger_bridge(
    const std::string& project_id = "genesis",
    const std::string& component_id = "genesis-memory",
    const std::string& storage_root = "E:/OpenCode-Data/Genesis",
    bool enable_persistence = true);

} // namespace genesis::ledger