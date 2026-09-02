#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace genesis {

[[nodiscard]] std::vector<std::string> split(std::string_view value, char delimiter);
[[nodiscard]] std::vector<std::string> split_fields(std::string_view value, char delimiter);

} // namespace genesis

