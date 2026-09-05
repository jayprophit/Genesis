#include "genesis/security/crypto_persistence.hpp"

#include "genesis/common/immutable_snapshot.hpp"
#include "genesis/runtime/runtime.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace genesis::security {
namespace {

constexpr std::string_view kMagic = "GENESIS-CRYPTO-PROVIDER-REGISTRY";
constexpr std::string_view kFileSuffix = "crypto";
constexpr std::uint64_t kSchemaVersion = 1U;
constexpr std::uint64_t kMaximumItems = 1'000'000U;
constexpr std::uint64_t kMaximumAlgorithmsPerRecord = 256U;
constexpr std::uint64_t kMaximumThreatsPerRecord = 16U;
constexpr std::uint64_t kMaximumFieldBytes = 16U * 1024U * 1024U;
constexpr std::size_t kChecksumBytes = 64U;

void set_error(CryptoStoreError* error,
               CryptoStoreErrorCode code,
               std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->message = std::move(message);
    }
}

void map_file_error(const storage::ImmutableFileError& source,
                    bool writing,
                    CryptoStoreError* target) {
    switch (source.code) {
    case storage::ImmutableFileErrorCode::none:
        set_error(target, CryptoStoreErrorCode::none, {});
        return;
    case storage::ImmutableFileErrorCode::invalid_identifier:
        set_error(target, CryptoStoreErrorCode::invalid_identifier, source.message);
        return;
    case storage::ImmutableFileErrorCode::not_found:
        set_error(target, CryptoStoreErrorCode::not_found, source.message);
        return;
    case storage::ImmutableFileErrorCode::conflicting_version:
        set_error(target, CryptoStoreErrorCode::conflicting_version, source.message);
        return;
    case storage::ImmutableFileErrorCode::record_too_large:
        set_error(target,
                  writing ? CryptoStoreErrorCode::invalid_registry
                          : CryptoStoreErrorCode::corrupt_record,
                  source.message);
        return;
    case storage::ImmutableFileErrorCode::unsafe_file_type:
        set_error(target,
                  writing ? CryptoStoreErrorCode::io_error
                          : CryptoStoreErrorCode::corrupt_record,
                  source.message);
        return;
    case storage::ImmutableFileErrorCode::io_error:
        set_error(target, CryptoStoreErrorCode::io_error, source.message);
        return;
    }
    set_error(target, CryptoStoreErrorCode::io_error, "unknown immutable-file error");
}

void append_u64(std::string& output, std::uint64_t value) {
    for (unsigned shift = 0U; shift < 64U; shift += 8U) {
        output.push_back(static_cast<char>((value >> shift) & 0xffU));
    }
}

std::uint64_t read_u64(std::string_view bytes, std::size_t& offset) {
    if (offset > bytes.size() || bytes.size() - offset < 8U) {
        throw std::runtime_error("truncated cryptographic registry integer");
    }
    std::uint64_t value = 0U;
    for (unsigned shift = 0U; shift < 64U; shift += 8U) {
        value |= static_cast<std::uint64_t>(
                     static_cast<unsigned char>(bytes[offset++]))
                 << shift;
    }
    return value;
}

void append_string(std::string& output, std::string_view value) {
    append_u64(output, static_cast<std::uint64_t>(value.size()));
    output.append(value);
}

std::string read_string(std::string_view bytes, std::size_t& offset) {
    const auto length = read_u64(bytes, offset);
    const auto remaining = static_cast<std::uint64_t>(bytes.size() - offset);
    if (length > kMaximumFieldBytes || length > remaining
        || length > static_cast<std::uint64_t>(
                        std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("invalid cryptographic registry field length");
    }
    const auto size = static_cast<std::size_t>(length);
    std::string value(bytes.substr(offset, size));
    offset += size;
    return value;
}

std::uint64_t read_count(std::string_view bytes,
                         std::size_t& offset,
                         std::uint64_t maximum,
                         std::string_view label) {
    const auto count = read_u64(bytes, offset);
    if (count > maximum
        || count > static_cast<std::uint64_t>(
                       std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("invalid " + std::string(label) + " count");
    }
    return count;
}

template <typename Enum>
void append_enum(std::string& output, Enum value) {
    append_u64(output, static_cast<std::uint64_t>(value));
}

template <typename Enum>
Enum read_enum(std::string_view bytes,
               std::size_t& offset,
               Enum maximum,
               std::string_view label) {
    const auto value = read_u64(bytes, offset);
    if (value > static_cast<std::uint64_t>(maximum)) {
        throw std::runtime_error("invalid " + std::string(label));
    }
    return static_cast<Enum>(value);
}

void append_optional_time(std::string& output,
                          const std::optional<std::uint64_t>& value) {
    append_u64(output, value.has_value() ? 1U : 0U);
    append_u64(output, value.value_or(0U));
}

std::optional<std::uint64_t> read_optional_time(std::string_view bytes,
                                                std::size_t& offset,
                                                std::string_view label) {
    const auto present = read_u64(bytes, offset);
    const auto value = read_u64(bytes, offset);
    if (present > 1U || (present == 0U && value != 0U)) {
        throw std::runtime_error("invalid optional " + std::string(label));
    }
    return present == 1U ? std::optional<std::uint64_t>{value} : std::nullopt;
}

void append_threat(std::string& output,
                   const CryptoThreatModelRevision& revision) {
    append_string(output, revision.id);
    append_u64(output, static_cast<std::uint64_t>(revision.threats.size()));
    for (const auto threat : revision.threats) {
        append_enum(output, threat);
    }
    append_string(output, revision.asset_inventory_digest);
    append_string(output, revision.trust_boundaries_digest);
    append_string(output, revision.mitigation_plan_digest);
    append_string(output, revision.residual_risk_digest);
    append_string(output, revision.evidence_digest);
    append_u64(output, revision.revision);
    append_u64(output, revision.effective_from);
    append_u64(output, revision.review_at);
    append_u64(output, revision.recorded_at);
    append_string(output, revision.previous_revision_digest);
    append_string(output, revision.revision_digest);
}

CryptoThreatModelRevision read_threat(std::string_view bytes,
                                      std::size_t& offset) {
    CryptoThreatModelRevision revision;
    revision.id = read_string(bytes, offset);
    const auto threat_count = read_count(bytes,
                                         offset,
                                         kMaximumThreatsPerRecord,
                                         "threat category");
    revision.threats.reserve(static_cast<std::size_t>(threat_count));
    for (std::uint64_t index = 0U; index < threat_count; ++index) {
        revision.threats.push_back(read_enum(bytes,
                                             offset,
                                             CryptoThreatCategory::implementation_flaw,
                                             "threat category"));
    }
    revision.asset_inventory_digest = read_string(bytes, offset);
    revision.trust_boundaries_digest = read_string(bytes, offset);
    revision.mitigation_plan_digest = read_string(bytes, offset);
    revision.residual_risk_digest = read_string(bytes, offset);
    revision.evidence_digest = read_string(bytes, offset);
    revision.revision = read_u64(bytes, offset);
    revision.effective_from = read_u64(bytes, offset);
    revision.review_at = read_u64(bytes, offset);
    revision.recorded_at = read_u64(bytes, offset);
    revision.previous_revision_digest = read_string(bytes, offset);
    revision.revision_digest = read_string(bytes, offset);
    return revision;
}

void append_rule(std::string& output, const CryptoAlgorithmRule& rule) {
    append_string(output, rule.algorithm_id);
    append_enum(output, rule.function);
    append_enum(output, rule.quantum_readiness);
    append_enum(output, rule.disposition);
    append_u64(output, rule.security_strength_bits);
    append_string(output, rule.standard_reference);
    append_string(output, rule.standard_evidence_digest);
    append_optional_time(output, rule.deprecate_at);
    append_optional_time(output, rule.prohibit_at);
}

CryptoAlgorithmRule read_rule(std::string_view bytes, std::size_t& offset) {
    CryptoAlgorithmRule rule;
    rule.algorithm_id = read_string(bytes, offset);
    rule.function = read_enum(
        bytes, offset, CryptoFunction::key_wrapping, "cryptographic function");
    rule.quantum_readiness = read_enum(
        bytes, offset, QuantumReadiness::hybrid, "quantum-readiness state");
    rule.disposition = read_enum(
        bytes, offset, AlgorithmDisposition::prohibited, "algorithm disposition");
    const auto strength = read_u64(bytes, offset);
    if (strength > std::numeric_limits<std::uint16_t>::max()) {
        throw std::runtime_error("invalid algorithm security strength");
    }
    rule.security_strength_bits = static_cast<std::uint16_t>(strength);
    rule.standard_reference = read_string(bytes, offset);
    rule.standard_evidence_digest = read_string(bytes, offset);
    rule.deprecate_at = read_optional_time(bytes, offset, "algorithm deprecation time");
    rule.prohibit_at = read_optional_time(bytes, offset, "algorithm prohibition time");
    return rule;
}

void append_policy(std::string& output, const CryptoPolicyRevision& revision) {
    append_string(output, revision.id);
    append_string(output, revision.threat_model_id);
    append_u64(output, revision.threat_model_revision);
    append_string(output, revision.threat_model_digest);
    append_string(output, revision.source_set_digest);
    append_string(output, revision.change_evidence_digest);
    append_u64(output, static_cast<std::uint64_t>(revision.rules.size()));
    for (const auto& rule : revision.rules) {
        append_rule(output, rule);
    }
    append_u64(output, revision.revision);
    append_u64(output, revision.effective_from);
    append_u64(output, revision.review_at);
    append_u64(output, revision.recorded_at);
    append_string(output, revision.previous_revision_digest);
    append_string(output, revision.revision_digest);
}

CryptoPolicyRevision read_policy(std::string_view bytes, std::size_t& offset) {
    CryptoPolicyRevision revision;
    revision.id = read_string(bytes, offset);
    revision.threat_model_id = read_string(bytes, offset);
    revision.threat_model_revision = read_u64(bytes, offset);
    revision.threat_model_digest = read_string(bytes, offset);
    revision.source_set_digest = read_string(bytes, offset);
    revision.change_evidence_digest = read_string(bytes, offset);
    const auto rule_count = read_count(
        bytes, offset, kMaximumAlgorithmsPerRecord, "algorithm rule");
    revision.rules.reserve(static_cast<std::size_t>(rule_count));
    for (std::uint64_t index = 0U; index < rule_count; ++index) {
        revision.rules.push_back(read_rule(bytes, offset));
    }
    revision.revision = read_u64(bytes, offset);
    revision.effective_from = read_u64(bytes, offset);
    revision.review_at = read_u64(bytes, offset);
    revision.recorded_at = read_u64(bytes, offset);
    revision.previous_revision_digest = read_string(bytes, offset);
    revision.revision_digest = read_string(bytes, offset);
    return revision;
}

void append_capability(std::string& output,
                       const CryptoAlgorithmCapability& capability) {
    append_string(output, capability.algorithm_id);
    append_enum(output, capability.function);
    append_string(output, capability.implementation_route);
    append_string(output, capability.implementation_digest);
}

CryptoAlgorithmCapability read_capability(std::string_view bytes,
                                          std::size_t& offset) {
    CryptoAlgorithmCapability capability;
    capability.algorithm_id = read_string(bytes, offset);
    capability.function = read_enum(
        bytes, offset, CryptoFunction::key_wrapping, "cryptographic function");
    capability.implementation_route = read_string(bytes, offset);
    capability.implementation_digest = read_string(bytes, offset);
    return capability;
}

void append_provider(std::string& output,
                     const CryptoProviderManifest& provider) {
    append_string(output, provider.provider_id);
    append_string(output, provider.implementation_name);
    append_string(output, provider.implementation_version);
    append_string(output, provider.platform_id);
    append_string(output, provider.module_boundary_digest);
    append_string(output, provider.module_binary_digest);
    append_string(output, provider.source_provenance_digest);
    append_string(output, provider.license_evidence_digest);
    append_string(output, provider.build_evidence_digest);
    append_u64(output, static_cast<std::uint64_t>(provider.capabilities.size()));
    for (const auto& capability : provider.capabilities) {
        append_capability(output, capability);
    }
    append_u64(output, provider.declared_at);
}

CryptoProviderManifest read_provider(std::string_view bytes,
                                     std::size_t& offset) {
    CryptoProviderManifest provider;
    provider.provider_id = read_string(bytes, offset);
    provider.implementation_name = read_string(bytes, offset);
    provider.implementation_version = read_string(bytes, offset);
    provider.platform_id = read_string(bytes, offset);
    provider.module_boundary_digest = read_string(bytes, offset);
    provider.module_binary_digest = read_string(bytes, offset);
    provider.source_provenance_digest = read_string(bytes, offset);
    provider.license_evidence_digest = read_string(bytes, offset);
    provider.build_evidence_digest = read_string(bytes, offset);
    const auto capability_count = read_count(
        bytes, offset, kMaximumAlgorithmsPerRecord, "provider capability");
    provider.capabilities.reserve(static_cast<std::size_t>(capability_count));
    for (std::uint64_t index = 0U; index < capability_count; ++index) {
        provider.capabilities.push_back(read_capability(bytes, offset));
    }
    provider.declared_at = read_u64(bytes, offset);
    return provider;
}

void append_route(std::string& output, const CryptoRouteEvidence& route) {
    append_string(output, route.algorithm_id);
    append_enum(output, route.function);
    append_string(output, route.implementation_route);
    append_string(output, route.implementation_digest);
    append_string(output, route.functional_test_digest);
    append_string(output, route.platform_evidence_digest);
    append_string(output, route.algorithm_validation_reference);
    append_string(output, route.algorithm_validation_evidence_digest);
    append_string(output, route.key_custody_evidence_digest);
}

CryptoRouteEvidence read_route(std::string_view bytes, std::size_t& offset) {
    CryptoRouteEvidence route;
    route.algorithm_id = read_string(bytes, offset);
    route.function = read_enum(
        bytes, offset, CryptoFunction::key_wrapping, "cryptographic function");
    route.implementation_route = read_string(bytes, offset);
    route.implementation_digest = read_string(bytes, offset);
    route.functional_test_digest = read_string(bytes, offset);
    route.platform_evidence_digest = read_string(bytes, offset);
    route.algorithm_validation_reference = read_string(bytes, offset);
    route.algorithm_validation_evidence_digest = read_string(bytes, offset);
    route.key_custody_evidence_digest = read_string(bytes, offset);
    return route;
}

void append_assessment(std::string& output,
                       const CryptoProviderAssessment& assessment) {
    append_string(output, assessment.id);
    append_string(output, assessment.provider_id);
    append_enum(output, assessment.kind);
    append_string(output, assessment.policy_id);
    append_u64(output, assessment.policy_revision);
    append_string(output, assessment.evaluator_entity_id);
    append_string(output, assessment.evaluation_evidence_digest);
    append_string(output, assessment.module_validation_reference);
    append_string(output, assessment.module_validation_evidence_digest);
    append_u64(output, static_cast<std::uint64_t>(assessment.routes.size()));
    for (const auto& route : assessment.routes) {
        append_route(output, route);
    }
    append_u64(output, assessment.sequence);
    append_u64(output, assessment.observed_at);
    append_u64(output, assessment.recorded_at);
    append_optional_time(output, assessment.valid_until);
    append_string(output, assessment.previous_assessment_digest);
    append_string(output, assessment.assessment_digest);
}

CryptoProviderAssessment read_assessment(std::string_view bytes,
                                         std::size_t& offset) {
    CryptoProviderAssessment assessment;
    assessment.id = read_string(bytes, offset);
    assessment.provider_id = read_string(bytes, offset);
    assessment.kind = read_enum(bytes,
                                offset,
                                ProviderAssessmentKind::revocation,
                                "provider assessment kind");
    assessment.policy_id = read_string(bytes, offset);
    assessment.policy_revision = read_u64(bytes, offset);
    assessment.evaluator_entity_id = read_string(bytes, offset);
    assessment.evaluation_evidence_digest = read_string(bytes, offset);
    assessment.module_validation_reference = read_string(bytes, offset);
    assessment.module_validation_evidence_digest = read_string(bytes, offset);
    const auto route_count = read_count(
        bytes, offset, kMaximumAlgorithmsPerRecord, "assessment route");
    assessment.routes.reserve(static_cast<std::size_t>(route_count));
    for (std::uint64_t index = 0U; index < route_count; ++index) {
        assessment.routes.push_back(read_route(bytes, offset));
    }
    assessment.sequence = read_u64(bytes, offset);
    assessment.observed_at = read_u64(bytes, offset);
    assessment.recorded_at = read_u64(bytes, offset);
    assessment.valid_until = read_optional_time(bytes, offset, "assessment expiry");
    assessment.previous_assessment_digest = read_string(bytes, offset);
    assessment.assessment_digest = read_string(bytes, offset);
    return assessment;
}

std::size_t checked_capacity(std::uint64_t value) {
    if (value == 0U || value > kMaximumItems
        || value > static_cast<std::uint64_t>(
                       std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("invalid cryptographic registry capacity");
    }
    return static_cast<std::size_t>(value);
}

} // namespace

CryptoProviderStore::CryptoProviderStore(std::filesystem::path root,
                                         std::size_t maximum_record_bytes)
    : root_(std::move(root)), maximum_record_bytes_(maximum_record_bytes) {
    static_cast<void>(storage::ImmutableSnapshotFiles(root_, maximum_record_bytes_));
}

std::string CryptoProviderStore::serialize(
    const CryptoProviderRegistry& registry) {
    if (!registry.verify()) {
        throw std::invalid_argument(
            "cannot serialize an invalid cryptographic provider registry");
    }

    std::string output{kMagic};
    append_u64(output, kSchemaVersion);
    append_string(output, registry.registry_id_);
    append_string(output, registry.entity_namespace_id_);
    append_string(output, registry.owner_entity_id_);
    append_u64(output, static_cast<std::uint64_t>(registry.threat_revision_capacity_));
    append_u64(output, static_cast<std::uint64_t>(registry.policy_revision_capacity_));
    append_u64(output, static_cast<std::uint64_t>(registry.provider_capacity_));
    append_u64(output, static_cast<std::uint64_t>(registry.assessment_capacity_));

    append_u64(output, static_cast<std::uint64_t>(registry.threat_revision_count_));
    for (const auto& [unused_id, history] : registry.threat_models_) {
        static_cast<void>(unused_id);
        for (const auto& revision : history) {
            append_threat(output, revision);
        }
    }
    append_u64(output, static_cast<std::uint64_t>(registry.policy_revision_count_));
    for (const auto& [unused_id, history] : registry.policies_) {
        static_cast<void>(unused_id);
        for (const auto& revision : history) {
            append_policy(output, revision);
        }
    }
    append_u64(output, static_cast<std::uint64_t>(registry.providers_.size()));
    for (const auto& [unused_id, provider] : registry.providers_) {
        static_cast<void>(unused_id);
        append_provider(output, provider);
    }
    append_u64(output, static_cast<std::uint64_t>(registry.assessment_count_));
    for (const auto& [unused_id, history] : registry.assessments_) {
        static_cast<void>(unused_id);
        for (const auto& assessment : history) {
            append_assessment(output, assessment);
        }
    }
    output += runtime::sha256(output);
    return output;
}

std::optional<CryptoProviderRegistry> CryptoProviderStore::deserialize(
    std::string_view bytes,
    CryptoStoreError* error) {
    try {
        if (bytes.size() < kMagic.size() + 8U + kChecksumBytes
            || bytes.substr(0U, kMagic.size()) != kMagic) {
            throw std::runtime_error("invalid cryptographic registry magic");
        }
        const auto payload = bytes.substr(0U, bytes.size() - kChecksumBytes);
        const auto checksum = bytes.substr(bytes.size() - kChecksumBytes);
        if (runtime::sha256(payload) != checksum) {
            throw std::runtime_error("cryptographic registry checksum mismatch");
        }

        std::size_t offset = kMagic.size();
        if (read_u64(payload, offset) != kSchemaVersion) {
            set_error(error,
                      CryptoStoreErrorCode::unsupported_schema,
                      "unsupported cryptographic registry schema");
            return std::nullopt;
        }
        auto registry_id = read_string(payload, offset);
        auto namespace_id = read_string(payload, offset);
        auto owner_id = read_string(payload, offset);
        const auto threat_capacity = checked_capacity(read_u64(payload, offset));
        const auto policy_capacity = checked_capacity(read_u64(payload, offset));
        const auto provider_capacity = checked_capacity(read_u64(payload, offset));
        const auto assessment_capacity = checked_capacity(read_u64(payload, offset));
        CryptoProviderRegistry registry(std::move(registry_id),
                                        std::move(namespace_id),
                                        std::move(owner_id),
                                        threat_capacity,
                                        policy_capacity,
                                        provider_capacity,
                                        assessment_capacity);

        const auto threat_count = read_count(
            payload, offset, threat_capacity, "threat-model revision");
        for (std::uint64_t index = 0U; index < threat_count; ++index) {
            auto revision = read_threat(payload, offset);
            registry.threat_models_[revision.id].push_back(std::move(revision));
        }
        const auto policy_count = read_count(
            payload, offset, policy_capacity, "policy revision");
        for (std::uint64_t index = 0U; index < policy_count; ++index) {
            auto revision = read_policy(payload, offset);
            registry.policies_[revision.id].push_back(std::move(revision));
        }
        const auto provider_count = read_count(
            payload, offset, provider_capacity, "provider");
        for (std::uint64_t index = 0U; index < provider_count; ++index) {
            auto provider = read_provider(payload, offset);
            const auto id = provider.provider_id;
            if (!registry.providers_.emplace(id, std::move(provider)).second) {
                throw std::runtime_error("duplicate stored cryptographic provider");
            }
        }
        const auto assessment_count = read_count(
            payload, offset, assessment_capacity, "provider assessment");
        for (std::uint64_t index = 0U; index < assessment_count; ++index) {
            auto assessment = read_assessment(payload, offset);
            registry.assessments_[assessment.provider_id].push_back(
                std::move(assessment));
        }
        if (offset != payload.size() || !registry.rebuild_indexes()
            || !registry.verify()) {
            throw std::runtime_error(
                "trailing or internally inconsistent cryptographic registry data");
        }
        if (serialize(registry) != bytes) {
            throw std::runtime_error(
                "non-canonical cryptographic registry serialization");
        }
        set_error(error, CryptoStoreErrorCode::none, {});
        return registry;
    } catch (const std::exception& exception) {
        set_error(error, CryptoStoreErrorCode::corrupt_record, exception.what());
        return std::nullopt;
    }
}

bool CryptoProviderStore::write(const CryptoProviderRegistry& registry,
                                std::string_view version,
                                CryptoStoreError* error) const {
    try {
        const auto bytes = serialize(registry);
        storage::ImmutableSnapshotFiles files(root_, maximum_record_bytes_);
        storage::ImmutableFileError file_error;
        if (!files.write(
                registry.registry_id(), version, kFileSuffix, bytes, &file_error)) {
            map_file_error(file_error, true, error);
            return false;
        }
        set_error(error, CryptoStoreErrorCode::none, {});
        return true;
    } catch (const std::invalid_argument& exception) {
        set_error(error, CryptoStoreErrorCode::invalid_registry, exception.what());
        return false;
    } catch (const std::exception& exception) {
        set_error(error, CryptoStoreErrorCode::io_error, exception.what());
        return false;
    }
}

std::optional<CryptoProviderRegistry> CryptoProviderStore::read(
    std::string_view registry_id,
    std::string_view entity_namespace_id,
    std::string_view owner_entity_id,
    std::string_view version,
    CryptoStoreError* error) const {
    try {
        if (derive_crypto_registry_id(entity_namespace_id, owner_entity_id)
            != registry_id) {
            throw std::invalid_argument(
                "cryptographic registry identity binding is invalid");
        }
        storage::ImmutableSnapshotFiles files(root_, maximum_record_bytes_);
        storage::ImmutableFileError file_error;
        const auto bytes = files.read(
            registry_id, version, kFileSuffix, &file_error);
        if (!bytes.has_value()) {
            map_file_error(file_error, false, error);
            return std::nullopt;
        }
        auto registry = deserialize(*bytes, error);
        if (registry.has_value()
            && (registry->registry_id() != registry_id
                || registry->entity_namespace_id() != entity_namespace_id
                || registry->owner_entity_id() != owner_entity_id)) {
            set_error(error,
                      CryptoStoreErrorCode::corrupt_record,
                      "cryptographic registry identity binding mismatch");
            return std::nullopt;
        }
        return registry;
    } catch (const std::invalid_argument& exception) {
        set_error(error, CryptoStoreErrorCode::invalid_identifier, exception.what());
        return std::nullopt;
    } catch (const std::exception& exception) {
        set_error(error, CryptoStoreErrorCode::io_error, exception.what());
        return std::nullopt;
    }
}

} // namespace genesis::security
