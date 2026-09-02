#include "genesis/common/text.hpp"

#include <sstream>

namespace genesis {

std::vector<std::string> split(std::string_view value, char delimiter) {
    std::vector<std::string> result;
    std::stringstream stream{std::string(value)};
    std::string field;
    while (std::getline(stream, field, delimiter)) {
        if (!field.empty() && field != "-") {
            result.push_back(std::move(field));
        }
    }
    return result;
}

std::vector<std::string> split_fields(std::string_view value, char delimiter) {
    std::vector<std::string> result;
    std::stringstream stream{std::string(value)};
    std::string field;
    while (std::getline(stream, field, delimiter)) {
        result.push_back(std::move(field));
    }
    if (!value.empty() && value.back() == delimiter) {
        result.emplace_back();
    }
    return result;
}

} // namespace genesis

