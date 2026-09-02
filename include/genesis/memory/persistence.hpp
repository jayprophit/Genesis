#pragma once
#include "genesis/memory/graph.hpp"
#include <filesystem>
#include <optional>
#include <string>
namespace genesis::memory {
enum class MemoryStoreErrorCode { none, invalid_graph, invalid_identifier, not_found, io_error, conflicting_version, corrupt_record };
struct MemoryStoreError { MemoryStoreErrorCode code{MemoryStoreErrorCode::none}; std::string message; };
class MemoryStore final {
public:
 MemoryStore(std::filesystem::path root,std::size_t maximum_record_bytes=64U*1024U*1024U);
 [[nodiscard]] bool write(const MemoryGraph& graph,std::string_view version,MemoryStoreError* error=nullptr)const;
 [[nodiscard]] std::optional<MemoryGraph> read(std::string_view organism_id,std::string_view version,MemoryStoreError* error=nullptr)const;
 [[nodiscard]] static std::string serialize(const MemoryGraph& graph);
 [[nodiscard]] static std::optional<MemoryGraph> deserialize(std::string_view bytes,MemoryStoreError* error=nullptr);
private:
 std::filesystem::path root_; std::size_t maximum_record_bytes_{};
};
}
