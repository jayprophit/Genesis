#include "genesis/genesis.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

std::vector<std::vector<std::string>> read_rows(const char* path) {
    std::ifstream input(path);
    check(input.good(), "research registry cannot be opened");
    std::vector<std::vector<std::string>> rows;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            rows.push_back(genesis::split_fields(line, '\t'));
        }
    }
    return rows;
}

void test_research_schema(const char* path) {
    const auto rows = read_rows(path);
    check(rows.size() > 20, "research registry unexpectedly small");
    constexpr std::size_t expected_columns = 31;
    check(rows.front().size() == expected_columns, "research schema column count changed");
    check(rows.front().front() == "research_id" && rows.front().back() == "last_reviewed",
          "research schema boundary columns changed");

    std::unordered_set<std::string> ids;
    constexpr const char* required[] = {
        "RES-UNIVERSAL", "RES-FICTION", "RES-SCIFI", "RES-PATENT", "RES-UAP",
        "RES-SPECULATION", "RES-TECH-RADAR", "RES-DEADEND", "RES-PATENT-INTELLIGENCE", "SCI-ANOMALY",
        "COG-PERSPECTIVE-COUNCIL", "DAT-SHARED-DOMAIN", "SIM-DIVERGENCE",
        "SLF-NET-BOUNDARY",
    };
    for (std::size_t index = 1; index < rows.size(); ++index) {
        check(rows[index].size() == expected_columns, "research row has wrong column count");
        const auto& row = rows[index];
        check(ids.insert(row[0]).second, "research ID is duplicated");
        check(!row[1].empty() && !row[2].empty() && !row[8].empty(),
              "research row is missing identity or description");
        check(row[11].size() == 2 && row[11][0] == 'E' && row[11][1] >= '0' && row[11][1] <= '7',
              "research evidence class is invalid");
        check(row[12].size() == 2 && row[12][0] == 'U' && row[12][1] >= '0' && row[12][1] <= '6',
              "research utility class is invalid");
    }
    for (const auto* required_id : required) {
        check(ids.contains(required_id), "required research lane is missing");
    }
}

void test_dead_end_schema(const char* path, const std::unordered_set<std::string>& research_ids) {
    const auto rows = read_rows(path);
    check(rows.size() >= 10, "dead-end registry unexpectedly small");
    constexpr std::size_t expected_columns = 15;
    check(rows.front().size() == expected_columns, "dead-end schema column count changed");
    check(rows.front().front() == "dead_end_id" && rows.front()[1] == "research_id",
          "dead-end schema identity columns changed");
    std::unordered_set<std::string> ids;
    for (std::size_t index = 1; index < rows.size(); ++index) {
        check(rows[index].size() == expected_columns, "dead-end row has wrong column count");
        check(ids.insert(rows[index][0]).second, "dead-end ID is duplicated");
        check(research_ids.contains(rows[index][1]), "dead-end references an unknown research item");
        check(!rows[index][2].empty() && !rows[index][8].empty() && !rows[index][10].empty(),
              "dead-end row is missing rejection or reopen evidence");
    }
}

void test_class_schema(const char* path) {
    const auto rows = read_rows(path);
    check(rows.size() == 16, "research class registry must define E0-E7 and U0-U6");
    check(rows.front().size() == 6 && rows.front().front() == "class_type"
              && rows.front()[1] == "code",
          "research class schema changed");
    std::unordered_set<std::string> codes;
    for (std::size_t index = 1; index < rows.size(); ++index) {
        check(rows[index].size() == 6, "research class row has wrong column count");
        check(codes.insert(rows[index][1]).second, "research class code is duplicated");
        check(!rows[index][2].empty() && !rows[index][3].empty(),
              "research class row is incomplete");
        const auto expected_type = rows[index][1].front() == 'E' ? "EVIDENCE" : "UTILITY";
        check(rows[index][0] == expected_type, "research class type does not match code");
    }
    for (int value = 0; value <= 7; ++value) {
        check(codes.contains("E" + std::to_string(value)), "evidence class definition is missing");
    }
    for (int value = 0; value <= 6; ++value) {
        check(codes.contains("U" + std::to_string(value)), "utility class definition is missing");
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        check(argc == 4, "research item, class and dead-end registry paths required");
        const auto rows = read_rows(argv[1]);
        test_research_schema(argv[1]);
        std::unordered_set<std::string> research_ids;
        for (std::size_t index = 1; index < rows.size(); ++index) {
            research_ids.insert(rows[index][0]);
        }
        test_class_schema(argv[2]);
        test_dead_end_schema(argv[3], research_ids);
        std::cout << "Genesis research registry tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
