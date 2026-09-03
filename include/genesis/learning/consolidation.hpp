#pragma once
#include "genesis/memory/graph.hpp"
#include "genesis/runtime/runtime.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
namespace genesis::learning {
enum class TraceKind { declarative, procedural, episodic, semantic };
struct LearningTrace { std::string id; TraceKind kind{TraceKind::declarative}; double importance{}, uncertainty{}, accessibility{}; std::uint64_t last_used{}, rehearsal_count{}, estimated_cost{}; bool protected_trace{}; };
struct InterferenceEdge { std::string first, second; double severity{}; std::string evidence_digest; };
struct ConsolidationItem { std::string trace_id; double priority{}; std::uint64_t reserved_cost{}; std::vector<std::string> interference_ids; };
struct ConsolidationPlan { std::uint64_t logical_time{}, budget{}, reserved{}; std::vector<ConsolidationItem> items; std::vector<std::string> deferred; };
enum class ConsolidationStatus { applied, invalid_plan, resource_denied, stale_memory, record_capacity_exceeded };
struct ConsolidationRecord {
 std::uint64_t sequence{},logical_time{},reserved_cost{};
 std::string organism_id,previous_digest,digest;
 std::vector<memory::ReinforcementChange> changes;
};
struct ConsolidationOutcome { ConsolidationStatus status{ConsolidationStatus::invalid_plan}; std::string message; std::string record_digest; [[nodiscard]] bool applied()const noexcept{return status==ConsolidationStatus::applied;} };
enum class RetentionTrend { improved, stable, regressed };
struct RetentionObservation {
 std::string trace_id,evidence_digest,consolidation_record_digest;
 TraceKind kind{TraceKind::declarative};
 std::uint64_t logical_time{};
 double recall_score{},latency_score{},confidence{};
 std::vector<std::string> active_interference_ids;
};
struct RetentionRecord {
 std::uint64_t sequence{};
 RetentionObservation observation;
 std::string previous_digest,digest;
};
struct RetentionReport {
 std::string trace_id;
 TraceKind kind{TraceKind::declarative};
 std::uint64_t baseline_time{},follow_up_time{},elapsed{};
 double baseline_score{},follow_up_score{},delta{};
 std::optional<double> retention_ratio;
 RetentionTrend trend{RetentionTrend::stable};
 bool consolidation_observed{},interference_associated{};
 std::vector<std::string> interference_ids;
};
struct RetentionFeedback { std::string trace_id; double accessibility_adjustment{},uncertainty_adjustment{},urgency{}; std::uint64_t observed_at{}; };
class ConsolidationScheduler final {
public:
 explicit ConsolidationScheduler(std::size_t trace_capacity,std::size_t interference_capacity);
 bool register_trace(LearningTrace trace,std::string* error=nullptr);
 bool record_interference(InterferenceEdge edge,std::string* error=nullptr);
 bool record_use(std::string_view trace_id,std::uint64_t logical_time);
 bool apply_feedback(const RetentionFeedback& feedback,std::string* error=nullptr);
 [[nodiscard]] ConsolidationPlan plan(std::uint64_t logical_time,std::uint64_t budget,std::size_t maximum_items)const;
 [[nodiscard]] bool verify()const;
 [[nodiscard]] std::size_t trace_count()const noexcept;
private:
 std::size_t trace_capacity_{},interference_capacity_{};
 std::map<std::string,LearningTrace> traces_;
 std::vector<InterferenceEdge> interference_;
};
class RetentionEvaluator final {
public:
 RetentionEvaluator(std::string organism_id,std::size_t record_capacity,double stable_tolerance=.02);
 bool observe(RetentionObservation observation,std::string* error=nullptr);
 [[nodiscard]] std::optional<RetentionReport> report(std::string_view trace_id)const;
 [[nodiscard]] std::optional<RetentionFeedback> feedback(std::string_view trace_id)const;
 [[nodiscard]] bool verify()const;
 [[nodiscard]] const std::vector<RetentionRecord>& records()const noexcept;
 [[nodiscard]] const std::string& organism_id()const noexcept;
 [[nodiscard]] std::size_t record_capacity()const noexcept;
 [[nodiscard]] double stable_tolerance()const noexcept;
private:
 std::string organism_id_;
 std::size_t record_capacity_{};
 double stable_tolerance_{};
 std::vector<RetentionRecord> records_;
 std::map<std::string,std::vector<std::size_t>> trace_index_;
};
class ConsolidationExecutor final {
public:
 explicit ConsolidationExecutor(std::size_t record_capacity,double reinforcement_scale=.2);
 [[nodiscard]] ConsolidationOutcome execute(const ConsolidationPlan& plan,memory::MemoryGraph& graph,runtime::ResourceAccounts& resources);
 [[nodiscard]] bool verify()const;
 [[nodiscard]] const std::vector<ConsolidationRecord>& records()const noexcept;
private:
 std::size_t record_capacity_{};
 double reinforcement_scale_{};
 std::vector<ConsolidationRecord> records_;
};
}
