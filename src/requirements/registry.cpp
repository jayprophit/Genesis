#include "genesis/requirements/registry.hpp"

#include "genesis/common/text.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <unordered_set>

namespace genesis {
namespace {

constexpr std::string_view requirement_header =
    "id\tname\tpurpose\tparent\tdependencies\tinterfaces\timplementation_files\ttests\tbenchmarks\t"
    "score_0_100\tstatus\tversion\tevidence\tprovenance\tlast_verified\taliases";
constexpr std::string_view domain_header = "id\tname\tpurpose\tdepends_on";

bool at_least(RequirementStatus actual, RequirementStatus threshold) {
    return static_cast<int>(actual) >= static_cast<int>(threshold)
        && actual != RequirementStatus::superseded;
}

int parse_score(std::string_view value) {
    int score = -1;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), score);
    if (error != std::errc{} || end != value.data() + value.size() || score < 0 || score > 100) {
        throw std::runtime_error("requirement score must be an integer from 0 through 100");
    }
    return score;
}

template <typename Exists, typename Dependencies>
void append_graph_errors(const std::vector<std::string>& ids,
                         Exists&& exists,
                         Dependencies&& dependencies,
                         std::vector<std::string>& errors) {
    enum class Mark { unseen, visiting, visited };
    std::unordered_map<std::string, Mark> marks;
    std::vector<std::string> path;

    std::function<void(const std::string&)> visit = [&](const std::string& id) {
        const auto mark = marks[id];
        if (mark == Mark::visited) {
            return;
        }
        if (mark == Mark::visiting) {
            const auto begin = std::find(path.begin(), path.end(), id);
            std::string cycle = "dependency cycle";
            for (auto it = begin; it != path.end(); ++it) {
                cycle += " -> " + *it;
            }
            cycle += " -> " + id;
            errors.push_back(std::move(cycle));
            return;
        }

        marks[id] = Mark::visiting;
        path.push_back(id);
        for (const auto& dependency : dependencies(id)) {
            if (exists(dependency)) {
                visit(dependency);
            }
        }
        path.pop_back();
        marks[id] = Mark::visited;
    };

    for (const auto& id : ids) {
        visit(id);
    }
}

} // namespace

std::string_view to_string(RequirementStatus status) noexcept {
    constexpr std::array values{
        std::string_view{"DISCOVERED"}, std::string_view{"SPECIFIED"},
        std::string_view{"SCAFFOLDED"}, std::string_view{"IMPLEMENTED"},
        std::string_view{"COMPILED"}, std::string_view{"UNIT_TESTED"},
        std::string_view{"INTEGRATION_TESTED"}, std::string_view{"BENCHMARKED"},
        std::string_view{"PROVEN"}, std::string_view{"OPTIMIZED"},
        std::string_view{"STABLE"}, std::string_view{"SUPERSEDED"},
    };
    return values[static_cast<std::size_t>(status)];
}

std::optional<RequirementStatus> requirement_status_from_string(std::string_view value) noexcept {
    for (int candidate = static_cast<int>(RequirementStatus::discovered);
         candidate <= static_cast<int>(RequirementStatus::superseded); ++candidate) {
        const auto status = static_cast<RequirementStatus>(candidate);
        if (to_string(status) == value) {
            return status;
        }
    }
    return std::nullopt;
}

DomainRegistry DomainRegistry::load(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open domain registry: " + path.string());
    }
    std::string line;
    if (!std::getline(input, line) || line != domain_header) {
        throw std::runtime_error("invalid domain registry header");
    }

    DomainRegistry registry;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const auto fields = split_fields(line, '\t');
        if (fields.size() != 4) {
            throw std::runtime_error("domain registry row must contain four fields");
        }
        Domain domain{fields[0], fields[1], fields[2], split(fields[3], ',')};
        if (!registry.data_.emplace(domain.id, std::move(domain)).second) {
            throw std::runtime_error("duplicate domain ID: " + fields[0]);
        }
    }
    return registry;
}

const Domain* DomainRegistry::find(std::string_view id) const noexcept {
    const auto iterator = data_.find(std::string(id));
    return iterator == data_.end() ? nullptr : &iterator->second;
}

std::vector<std::string> DomainRegistry::validate() const {
    std::vector<std::string> errors;
    std::vector<std::string> ids;
    ids.reserve(data_.size());
    for (const auto& [id, domain] : data_) {
        ids.push_back(id);
        if (id.empty() || domain.name.empty() || domain.purpose.empty()) {
            errors.push_back(id + " has an empty required domain field");
        }
        for (const auto& dependency : domain.dependencies) {
            if (find(dependency) == nullptr) {
                errors.push_back(id + " missing domain dependency " + dependency);
            }
        }
    }
    append_graph_errors(ids,
                        [&](const std::string& id) { return find(id) != nullptr; },
                        [&](const std::string& id) -> const std::vector<std::string>& {
                            return find(id)->dependencies;
                        },
                        errors);
    return errors;
}

std::size_t DomainRegistry::size() const noexcept {
    return data_.size();
}

RequirementRegistry RequirementRegistry::load(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open requirement registry: " + path.string());
    }
    std::string line;
    if (!std::getline(input, line) || line != requirement_header) {
        throw std::runtime_error("invalid requirement registry header");
    }

    RequirementRegistry registry;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const auto fields = split_fields(line, '\t');
        if (fields.size() != 16) {
            throw std::runtime_error("requirement registry row must contain sixteen fields");
        }
        const auto status = requirement_status_from_string(fields[10]);
        if (!status) {
            throw std::runtime_error("unknown requirement status: " + fields[10]);
        }
        Requirement requirement{
            fields[0], fields[1], fields[2], fields[3], split(fields[4], ','),
            split(fields[5], ','), split(fields[6], ','), split(fields[7], ','),
            split(fields[8], ','), parse_score(fields[9]), *status, fields[11],
            split(fields[12], ','), fields[13], fields[14], split(fields[15], ','),
        };
        if (!registry.index_.emplace(requirement.id, registry.ordered_.size()).second) {
            throw std::runtime_error("duplicate requirement ID: " + requirement.id);
        }
        registry.ordered_.push_back(std::move(requirement));
    }
    return registry;
}

const Requirement* RequirementRegistry::find(std::string_view id) const noexcept {
    const auto iterator = index_.find(std::string(id));
    return iterator == index_.end() ? nullptr : &ordered_[iterator->second];
}

std::vector<std::string> RequirementRegistry::validate(const DomainRegistry* domains) const {
    std::vector<std::string> errors;
    std::unordered_set<std::string> aliases;
    std::vector<std::string> ids;
    ids.reserve(ordered_.size());

    for (const auto& requirement : ordered_) {
        ids.push_back(requirement.id);
        if (requirement.id.empty() || requirement.name.empty() || requirement.purpose.empty()
            || requirement.parent.empty() || requirement.version.empty()
            || requirement.provenance.empty() || requirement.last_verified.empty()) {
            errors.push_back(requirement.id + " has an empty required field");
        }
        if (domains != nullptr && domains->find(requirement.parent) == nullptr) {
            errors.push_back(requirement.id + " has unknown parent domain " + requirement.parent);
        }
        for (const auto& dependency : requirement.dependencies) {
            if (find(dependency) == nullptr) {
                errors.push_back(requirement.id + " missing requirement dependency " + dependency);
            }
        }
        for (const auto& alias : requirement.aliases) {
            if (!aliases.insert(alias).second) {
                errors.push_back(requirement.id + " repeats alias " + alias);
            }
        }
        if (at_least(requirement.status, RequirementStatus::compiled)
            && requirement.implementation_files.empty()) {
            errors.push_back(requirement.id + " is COMPILED or later without implementation files");
        }
        if (at_least(requirement.status, RequirementStatus::unit_tested)
            && requirement.tests.empty()) {
            errors.push_back(requirement.id + " is UNIT_TESTED or later without tests");
        }
        if (at_least(requirement.status, RequirementStatus::benchmarked)
            && requirement.benchmarks.empty()) {
            errors.push_back(requirement.id + " is BENCHMARKED or later without benchmarks");
        }
        if (at_least(requirement.status, RequirementStatus::proven)
            && requirement.evidence.empty()) {
            errors.push_back(requirement.id + " is PROVEN or later without evidence");
        }
    }

    append_graph_errors(ids,
                        [&](const std::string& id) { return find(id) != nullptr; },
                        [&](const std::string& id) -> const std::vector<std::string>& {
                            return find(id)->dependencies;
                        },
                        errors);
    return errors;
}

std::size_t RequirementRegistry::size() const noexcept {
    return ordered_.size();
}

const std::vector<Requirement>& RequirementRegistry::records() const noexcept {
    return ordered_;
}

} // namespace genesis

