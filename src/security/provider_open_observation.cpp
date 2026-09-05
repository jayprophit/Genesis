#include "genesis/security/provider_open_observation.hpp"

#include "genesis/runtime/runtime.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace genesis::security {
namespace {

constexpr std::uint32_t kHardwareFlag = 0x00000001U;
constexpr std::uint32_t kSoftwareFlag = 0x00000002U;
constexpr std::uint32_t kRemovableFlag = 0x00000008U;
constexpr std::uint32_t kHardwareRngFlag = 0x00000010U;
constexpr std::uint32_t kVirtualIsolationFlag = 0x00000020U;
constexpr std::uint32_t kKnownImplementationFlags =
    kHardwareFlag | kSoftwareFlag | kRemovableFlag | kHardwareRngFlag
    | kVirtualIsolationFlag;

constexpr std::array<std::string_view, 7> kStatusNames{
    "observed",
    "unsupported_platform",
    "inventory_unavailable",
    "not_registered",
    "open_failed",
    "property_query_failed",
    "release_failed",
};

void append_material(std::string& output, std::string_view value) {
    output.append(std::to_string(value.size()));
    output.push_back(':');
    output.append(value);
}

bool portable_identifier(std::string_view value, std::size_t maximum) {
    return !value.empty() && value.size() <= maximum
           && std::all_of(value.begin(), value.end(), [](unsigned char character) {
                  return (character >= 'a' && character <= 'z')
                         || (character >= 'A' && character <= 'Z')
                         || (character >= '0' && character <= '9')
                         || character == '_' || character == '-'
                         || character == '.';
              });
}

bool digest(std::string_view value) {
    return value.size() == 64U
           && std::all_of(value.begin(), value.end(), [](unsigned char character) {
                  return (character >= '0' && character <= '9')
                         || (character >= 'a' && character <= 'f');
              });
}

bool no_native_activity(const CryptoProviderOpenObservationDraft& draft) {
    return draft.open_native_status == 0U && draft.property_native_status == 0U
           && draft.release_native_status == 0U
           && draft.implementation_type_flags == 0U
           && !draft.registration_matched && !draft.provider_open_attempted
           && !draft.provider_open_succeeded
           && !draft.implementation_type_query_attempted
           && !draft.implementation_type_query_succeeded
           && !draft.provider_handle_released;
}

bool valid_state(const CryptoProviderOpenObservationDraft& draft) {
    switch (draft.status) {
    case CryptoProviderOpenStatus::observed:
        return draft.registration_matched && draft.provider_open_attempted
               && draft.provider_open_succeeded
               && draft.implementation_type_query_attempted
               && draft.implementation_type_query_succeeded
               && draft.provider_handle_released
               && draft.open_native_status == 0U
               && draft.property_native_status == 0U
               && draft.release_native_status == 0U;
    case CryptoProviderOpenStatus::unsupported_platform:
    case CryptoProviderOpenStatus::inventory_unavailable:
    case CryptoProviderOpenStatus::not_registered:
        return no_native_activity(draft);
    case CryptoProviderOpenStatus::open_failed:
        return draft.registration_matched && draft.provider_open_attempted
               && !draft.provider_open_succeeded
               && !draft.implementation_type_query_attempted
               && !draft.implementation_type_query_succeeded
               && !draft.provider_handle_released
               && draft.open_native_status != 0U
               && draft.property_native_status == 0U
               && draft.release_native_status == 0U
               && draft.implementation_type_flags == 0U;
    case CryptoProviderOpenStatus::property_query_failed:
        return draft.registration_matched && draft.provider_open_attempted
               && draft.provider_open_succeeded
               && draft.implementation_type_query_attempted
               && !draft.implementation_type_query_succeeded
               && draft.provider_handle_released
               && draft.open_native_status == 0U
               && draft.property_native_status != 0U
               && draft.release_native_status == 0U
               && draft.implementation_type_flags == 0U;
    case CryptoProviderOpenStatus::release_failed:
        return draft.registration_matched && draft.provider_open_attempted
               && draft.provider_open_succeeded
               && draft.implementation_type_query_attempted
               && !draft.provider_handle_released
               && draft.open_native_status == 0U
               && draft.release_native_status != 0U
               && ((draft.implementation_type_query_succeeded
                    && draft.property_native_status == 0U)
                   || (!draft.implementation_type_query_succeeded
                       && draft.property_native_status != 0U
                       && draft.implementation_type_flags == 0U));
    }
    return false;
}

std::string observation_digest(const CryptoProviderOpenObservation& observation) {
    std::string material{"genesis.security.crypto_provider_open_observation.v1"};
    append_material(material, observation.platform_id);
    append_material(material, observation.architecture);
    append_material(material, observation.provider_name);
    append_material(material, observation.provider_name_digest);
    append_material(material, observation.registration_inventory_digest);
    append_material(material, to_string(observation.status));
    append_material(material, std::to_string(observation.open_native_status));
    append_material(material, std::to_string(observation.property_native_status));
    append_material(material, std::to_string(observation.release_native_status));
    append_material(material, std::to_string(observation.implementation_type_flags));
    append_material(
        material,
        std::to_string(observation.unknown_implementation_type_flags));
    append_material(material, std::to_string(observation.observed_at));
    append_material(material, std::to_string(observation.elapsed_microseconds));
    append_material(material, observation.registration_matched ? "1" : "0");
    append_material(material, observation.provider_open_attempted ? "1" : "0");
    append_material(material, observation.provider_open_succeeded ? "1" : "0");
    append_material(material,
                    observation.implementation_type_query_attempted ? "1" : "0");
    append_material(material,
                    observation.implementation_type_query_succeeded ? "1" : "0");
    append_material(material, observation.provider_handle_released ? "1" : "0");
    append_material(material, observation.provider_reports_hardware ? "1" : "0");
    append_material(material, observation.provider_reports_software ? "1" : "0");
    append_material(material, observation.provider_reports_removable ? "1" : "0");
    append_material(material, observation.provider_reports_hardware_rng ? "1" : "0");
    append_material(material,
                    observation.provider_reports_virtual_isolation ? "1" : "0");
    append_material(material, observation.key_enumerated ? "1" : "0");
    append_material(material, observation.key_opened ? "1" : "0");
    append_material(material, observation.key_created ? "1" : "0");
    append_material(material, observation.key_exported ? "1" : "0");
    append_material(material,
                    observation.cryptographic_operation_executed ? "1" : "0");
    append_material(material, observation.provider_qualified ? "1" : "0");
    return runtime::sha256(material);
}

CryptoProviderOpenObservationDraft draft_from(
    const CryptoProviderOpenObservation& observation) {
    return {observation.platform_id,
            observation.architecture,
            observation.provider_name,
            observation.registration_inventory_digest,
            observation.status,
            observation.open_native_status,
            observation.property_native_status,
            observation.release_native_status,
            observation.implementation_type_flags,
            observation.observed_at,
            observation.elapsed_microseconds,
            observation.registration_matched,
            observation.provider_open_attempted,
            observation.provider_open_succeeded,
            observation.implementation_type_query_attempted,
            observation.implementation_type_query_succeeded,
            observation.provider_handle_released};
}

void append_json_string(std::string& output, std::string_view value) {
    static constexpr char hex[] = "0123456789abcdef";
    output.push_back('"');
    for (const unsigned char character : value) {
        switch (character) {
        case '"':
            output += "\\\"";
            break;
        case '\\':
            output += "\\\\";
            break;
        default:
            if (character < 0x20U) {
                output += "\\u00";
                output.push_back(hex[character >> 4U]);
                output.push_back(hex[character & 0x0fU]);
            } else {
                output.push_back(static_cast<char>(character));
            }
            break;
        }
    }
    output.push_back('"');
}

void append_json_boolean(std::string& output, bool value) {
    output += value ? "true" : "false";
}

void append_json_boolean_field(std::string& output,
                               std::string_view name,
                               bool value) {
    output += ",\"";
    output.append(name);
    output += "\":";
    append_json_boolean(output, value);
}

} // namespace

std::string_view to_string(CryptoProviderOpenStatus value) noexcept {
    const auto index = static_cast<std::size_t>(value);
    return index < kStatusNames.size() ? kStatusNames[index] : std::string_view{};
}

bool crypto_provider_open_status_from_string(
    std::string_view text,
    CryptoProviderOpenStatus& value) noexcept {
    const auto found = std::find(kStatusNames.begin(), kStatusNames.end(), text);
    if (found == kStatusNames.end()) {
        return false;
    }
    value = static_cast<CryptoProviderOpenStatus>(
        std::distance(kStatusNames.begin(), found));
    return true;
}

CryptoProviderOpenObservation make_crypto_provider_open_observation(
    CryptoProviderOpenObservationDraft draft) {
    if (!portable_identifier(draft.platform_id, 128U)
        || !portable_identifier(draft.architecture, 64U)
        || !valid_registered_crypto_provider_name(draft.provider_name)
        || !digest(draft.registration_inventory_digest)
        || to_string(draft.status).empty() || !valid_state(draft)) {
        throw std::invalid_argument("provider-open draft violates its evidence boundary");
    }
    CryptoProviderOpenObservation observation;
    observation.platform_id = std::move(draft.platform_id);
    observation.architecture = std::move(draft.architecture);
    observation.provider_name = std::move(draft.provider_name);
    observation.provider_name_digest =
        derive_registered_crypto_provider_name_digest(observation.provider_name);
    observation.registration_inventory_digest =
        std::move(draft.registration_inventory_digest);
    observation.status = draft.status;
    observation.open_native_status = draft.open_native_status;
    observation.property_native_status = draft.property_native_status;
    observation.release_native_status = draft.release_native_status;
    observation.implementation_type_flags = draft.implementation_type_flags;
    observation.unknown_implementation_type_flags =
        draft.implementation_type_flags & ~kKnownImplementationFlags;
    observation.observed_at = draft.observed_at;
    observation.elapsed_microseconds = draft.elapsed_microseconds;
    observation.registration_matched = draft.registration_matched;
    observation.provider_open_attempted = draft.provider_open_attempted;
    observation.provider_open_succeeded = draft.provider_open_succeeded;
    observation.implementation_type_query_attempted =
        draft.implementation_type_query_attempted;
    observation.implementation_type_query_succeeded =
        draft.implementation_type_query_succeeded;
    observation.provider_handle_released = draft.provider_handle_released;
    observation.provider_reports_hardware =
        (draft.implementation_type_flags & kHardwareFlag) != 0U;
    observation.provider_reports_software =
        (draft.implementation_type_flags & kSoftwareFlag) != 0U;
    observation.provider_reports_removable =
        (draft.implementation_type_flags & kRemovableFlag) != 0U;
    observation.provider_reports_hardware_rng =
        (draft.implementation_type_flags & kHardwareRngFlag) != 0U;
    observation.provider_reports_virtual_isolation =
        (draft.implementation_type_flags & kVirtualIsolationFlag) != 0U;
    observation.evidence_digest = observation_digest(observation);
    return observation;
}

bool CryptoProviderOpenObservation::verify() const {
    try {
        const auto draft = draft_from(*this);
        return portable_identifier(platform_id, 128U)
               && portable_identifier(architecture, 64U)
               && valid_registered_crypto_provider_name(provider_name)
               && provider_name_digest
                      == derive_registered_crypto_provider_name_digest(provider_name)
               && digest(registration_inventory_digest) && digest(evidence_digest)
               && !to_string(status).empty() && valid_state(draft)
               && unknown_implementation_type_flags
                      == (implementation_type_flags & ~kKnownImplementationFlags)
               && provider_reports_hardware
                      == ((implementation_type_flags & kHardwareFlag) != 0U)
               && provider_reports_software
                      == ((implementation_type_flags & kSoftwareFlag) != 0U)
               && provider_reports_removable
                      == ((implementation_type_flags & kRemovableFlag) != 0U)
               && provider_reports_hardware_rng
                      == ((implementation_type_flags & kHardwareRngFlag) != 0U)
               && provider_reports_virtual_isolation
                      == ((implementation_type_flags & kVirtualIsolationFlag) != 0U)
               && !key_enumerated && !key_opened && !key_created && !key_exported
               && !cryptographic_operation_executed && !provider_qualified
               && evidence_digest == observation_digest(*this);
    } catch (...) {
        return false;
    }
}

std::string crypto_provider_open_observation_json(
    const CryptoProviderOpenObservation& observation) {
    if (!observation.verify()) {
        throw std::invalid_argument("cannot serialize invalid provider-open evidence");
    }
    std::string output;
    output.reserve(1536U);
    output += "{\"schema\":\"genesis.security.crypto_provider_open_observation.v1\",\"platform_id\":";
    append_json_string(output, observation.platform_id);
    output += ",\"architecture\":";
    append_json_string(output, observation.architecture);
    output += ",\"provider_name\":";
    append_json_string(output, observation.provider_name);
    output += ",\"provider_name_digest\":";
    append_json_string(output, observation.provider_name_digest);
    output += ",\"registration_inventory_digest\":";
    append_json_string(output, observation.registration_inventory_digest);
    output += ",\"status\":";
    append_json_string(output, to_string(observation.status));
    output += ",\"open_native_status\":"
              + std::to_string(observation.open_native_status);
    output += ",\"property_native_status\":"
              + std::to_string(observation.property_native_status);
    output += ",\"release_native_status\":"
              + std::to_string(observation.release_native_status);
    output += ",\"implementation_type_flags\":"
              + std::to_string(observation.implementation_type_flags);
    output += ",\"unknown_implementation_type_flags\":"
              + std::to_string(observation.unknown_implementation_type_flags);
    output += ",\"observed_at\":" + std::to_string(observation.observed_at);
    output += ",\"elapsed_microseconds\":"
              + std::to_string(observation.elapsed_microseconds);
    append_json_boolean_field(
        output, "registration_matched", observation.registration_matched);
    append_json_boolean_field(
        output, "provider_open_attempted", observation.provider_open_attempted);
    append_json_boolean_field(
        output, "provider_open_succeeded", observation.provider_open_succeeded);
    append_json_boolean_field(output,
                              "implementation_type_query_attempted",
                              observation.implementation_type_query_attempted);
    append_json_boolean_field(output,
                              "implementation_type_query_succeeded",
                              observation.implementation_type_query_succeeded);
    append_json_boolean_field(output,
                              "provider_handle_released",
                              observation.provider_handle_released);
    append_json_boolean_field(output,
                              "provider_reports_hardware",
                              observation.provider_reports_hardware);
    append_json_boolean_field(output,
                              "provider_reports_software",
                              observation.provider_reports_software);
    append_json_boolean_field(output,
                              "provider_reports_removable",
                              observation.provider_reports_removable);
    append_json_boolean_field(output,
                              "provider_reports_hardware_rng",
                              observation.provider_reports_hardware_rng);
    append_json_boolean_field(output,
                              "provider_reports_virtual_isolation",
                              observation.provider_reports_virtual_isolation);
    append_json_boolean_field(output, "key_enumerated", observation.key_enumerated);
    append_json_boolean_field(output, "key_opened", observation.key_opened);
    append_json_boolean_field(output, "key_created", observation.key_created);
    append_json_boolean_field(output, "key_exported", observation.key_exported);
    append_json_boolean_field(output,
                              "cryptographic_operation_executed",
                              observation.cryptographic_operation_executed);
    append_json_boolean_field(
        output, "provider_qualified", observation.provider_qualified);
    output += ",\"evidence_digest\":";
    append_json_string(output, observation.evidence_digest);
    output.push_back('}');
    return output;
}

} // namespace genesis::security
