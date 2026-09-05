#pragma once

#include "genesis/agents/platform.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace genesis::agents {

struct PlatformSnapshot final {
    std::string owner_id;
    AgentRegistry agents;
    WorkflowRegistry workflows;
    TriggerRegistry triggers;
};

enum class PlatformStoreErrorCode {
    none,
    invalid_snapshot,
    invalid_identifier,
    not_found,
    io_error,
    conflicting_version,
    corrupt_record,
};

struct PlatformStoreError final {
    PlatformStoreErrorCode code{PlatformStoreErrorCode::none};
    std::string message;
};

class AgentPlatformStore final {
public:
    explicit AgentPlatformStore(std::filesystem::path root,
                                std::size_t maximum_record_bytes = 16U * 1024U * 1024U);
    [[nodiscard]] bool write(const PlatformSnapshot& snapshot,
                             std::string_view version,
                             PlatformStoreError* error = nullptr) const;
    [[nodiscard]] std::optional<PlatformSnapshot> read(
        std::string_view owner_id,
        std::string_view version,
        PlatformStoreError* error = nullptr) const;
    [[nodiscard]] static std::string serialize(const PlatformSnapshot& snapshot);
    [[nodiscard]] static std::optional<PlatformSnapshot> deserialize(
        std::string_view bytes,
        PlatformStoreError* error = nullptr);
private:
    std::filesystem::path root_;
    std::size_t maximum_record_bytes_{};
};

} // namespace genesis::agents
