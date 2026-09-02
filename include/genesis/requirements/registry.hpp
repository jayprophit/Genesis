#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace genesis {

enum class RequirementStatus {
    discovered,
    specified,
    scaffolded,
    implemented,
    compiled,
    unit_tested,
    integration_tested,
    benchmarked,
    proven,
    optimized,
    stable,
    superseded,
};

[[nodiscard]] std::string_view to_string(RequirementStatus status) noexcept;
[[nodiscard]] std::optional<RequirementStatus> requirement_status_from_string(
    std::string_view value) noexcept;

struct Requirement final {
    std::string id;
    std::string name;
    std::string purpose;
    std::string parent;
    std::vector<std::string> dependencies;
    std::vector<std::string> interfaces;
    std::vector<std::string> implementation_files;
    std::vector<std::string> tests;
    std::vector<std::string> benchmarks;
    int score_0_100{};
    RequirementStatus status{RequirementStatus::discovered};
    std::string version;
    std::vector<std::string> evidence;
    std::string provenance;
    std::string last_verified;
    std::vector<std::string> aliases;
};

struct Domain final {
    std::string id;
    std::string name;
    std::string purpose;
    std::vector<std::string> dependencies;
};

class DomainRegistry final {
public:
    [[nodiscard]] static DomainRegistry load(const std::filesystem::path& path);
    [[nodiscard]] const Domain* find(std::string_view id) const noexcept;
    [[nodiscard]] std::vector<std::string> validate() const;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::unordered_map<std::string, Domain> data_;
};

class RequirementRegistry final {
public:
    [[nodiscard]] static RequirementRegistry load(const std::filesystem::path& path);
    [[nodiscard]] const Requirement* find(std::string_view id) const noexcept;
    [[nodiscard]] std::vector<std::string> validate(
        const DomainRegistry* domains = nullptr) const;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] const std::vector<Requirement>& records() const noexcept;

private:
    std::vector<Requirement> ordered_;
    std::unordered_map<std::string, std::size_t> index_;
};

} // namespace genesis

