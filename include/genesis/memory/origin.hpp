#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::memory {

enum class Origin {
    inherited_parent_a,
    inherited_parent_b,
    direct_experience,
    taught,
    observed,
    inferred,
    simulated,
    generated,
    external_source,
};

[[nodiscard]] std::string_view to_string(Origin origin) noexcept;
[[nodiscard]] bool origin_from_string(std::string_view value, Origin& result) noexcept;

struct InheritedMemory final {
    std::string memory_id;
    Origin origin{Origin::inherited_parent_a};
    std::string source_parent;
    std::string source_digest;
    std::int64_t source_timestamp{};
    std::int64_t inheritance_timestamp{};
    double confidence{};
    std::vector<std::string> transformation_history;
    std::vector<std::string> compression_history;
};

[[nodiscard]] std::vector<std::string> validate(const InheritedMemory& memory);
[[nodiscard]] std::string canonical_form(const InheritedMemory& memory);
[[nodiscard]] std::string digest(const InheritedMemory& memory);

} // namespace genesis::memory
