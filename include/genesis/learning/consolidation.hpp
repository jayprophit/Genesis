#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>
namespace genesis::learning {
enum class TraceKind { declarative, procedural, episodic, semantic };
struct LearningTrace { std::string id; TraceKind kind{TraceKind::declarative}; double importance{}, uncertainty{}, accessibility{}; std::uint64_t last_used{}, rehearsal_count{}, estimated_cost{}; bool protected_trace{}; };
struct InterferenceEdge { std::string first, second; double severity{}; std::string evidence_digest; };
struct ConsolidationItem { std::string trace_id; double priority{}; std::uint64_t reserved_cost{}; std::vector<std::string> interference_ids; };
struct ConsolidationPlan { std::uint64_t logical_time{}, budget{}, reserved{}; std::vector<ConsolidationItem> items; std::vector<std::string> deferred; };
class ConsolidationScheduler final {
public:
 explicit ConsolidationScheduler(std::size_t trace_capacity,std::size_t interference_capacity);
 bool register_trace(LearningTrace trace,std::string* error=nullptr);
 bool record_interference(InterferenceEdge edge,std::string* error=nullptr);
 bool record_use(std::string_view trace_id,std::uint64_t logical_time);
 [[nodiscard]] ConsolidationPlan plan(std::uint64_t logical_time,std::uint64_t budget,std::size_t maximum_items)const;
 [[nodiscard]] bool verify()const;
 [[nodiscard]] std::size_t trace_count()const noexcept;
private:
 std::size_t trace_capacity_{},interference_capacity_{};
 std::map<std::string,LearningTrace> traces_;
 std::vector<InterferenceEdge> interference_;
};
}
