#include "genesis/memory/origin.hpp"

#include "genesis/runtime/runtime.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace genesis::memory {
namespace {

void append_field(std::string& target, std::string_view value) {
    target.append(std::to_string(value.size()));
    target.push_back(':');
    target.append(value);
}

} // namespace

std::string_view to_string(Origin origin) noexcept {
    constexpr std::array values{
        std::string_view{"INHERITED_PARENT_A"}, std::string_view{"INHERITED_PARENT_B"},
        std::string_view{"DIRECT_EXPERIENCE"}, std::string_view{"TAUGHT"},
        std::string_view{"OBSERVED"}, std::string_view{"INFERRED"},
        std::string_view{"SIMULATED"}, std::string_view{"GENERATED"},
        std::string_view{"EXTERNAL_SOURCE"},
    };
    const auto index = static_cast<std::size_t>(origin);
    return index < values.size() ? values[index] : std::string_view{"UNKNOWN"};
}

bool origin_from_string(std::string_view value, Origin& result) noexcept {
    for (std::size_t index = 0; index < 9; ++index) {
        const auto candidate = static_cast<Origin>(index);
        if (to_string(candidate) == value) {
            result = candidate;
            return true;
        }
    }
    return false;
}

std::vector<std::string> validate(const InheritedMemory& memory) {
    std::vector<std::string> errors;
    if (memory.memory_id.empty()) {
        errors.emplace_back("memory_id required");
    }
    if ((memory.origin == Origin::inherited_parent_a || memory.origin == Origin::inherited_parent_b)
        && memory.source_parent.empty()) {
        errors.emplace_back("inherited memory requires source_parent");
    }
    if (memory.source_digest.empty()) {
        errors.emplace_back("source_digest required");
    }
    if (!std::isfinite(memory.confidence) || memory.confidence < 0.0 || memory.confidence > 1.0) {
        errors.emplace_back("memory confidence must be finite and in [0,1]");
    }
    if (memory.inheritance_timestamp < memory.source_timestamp) {
        errors.emplace_back("inheritance timestamp cannot precede source timestamp");
    }
    return errors;
}

std::string canonical_form(const InheritedMemory& memory) {
    std::string result{"genesis.memory.origin.v1"};
    append_field(result, memory.memory_id);
    append_field(result, to_string(memory.origin));
    append_field(result, memory.source_parent);
    append_field(result, memory.source_digest);
    append_field(result, std::to_string(memory.source_timestamp));
    append_field(result, std::to_string(memory.inheritance_timestamp));
    append_field(result, std::to_string(memory.confidence));
    append_field(result, std::to_string(memory.transformation_history.size()));
    for (const auto& value : memory.transformation_history) {
        append_field(result, value);
    }
    append_field(result, std::to_string(memory.compression_history.size()));
    for (const auto& value : memory.compression_history) {
        append_field(result, value);
    }
    return result;
}

std::string digest(const InheritedMemory& memory) {
    return runtime::sha256(canonical_form(memory));
}

} // namespace genesis::memory
