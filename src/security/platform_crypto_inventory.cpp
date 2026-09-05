#include "genesis/security/platform_crypto_inventory.hpp"

#include "genesis/runtime/runtime.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace genesis::security {
namespace {

constexpr std::size_t kMaximumPlatformIdLength = 128U;
constexpr std::size_t kMaximumArchitectureLength = 64U;
constexpr std::size_t kMaximumProviderNameLength = 4096U;
constexpr std::size_t kMaximumProviderCount = 256U;

constexpr std::array<std::string_view, 3> kStatusNames{
    "observed",
    "unsupported_platform",
    "enumeration_failed",
};

void append_material(std::string& output, std::string_view value) {
    output.append(std::to_string(value.size()));
    output.push_back(':');
    output.append(value);
}

bool valid_utf8_evidence_text(std::string_view value) {
    const auto continuation = [](unsigned char byte) {
        return byte >= 0x80U && byte <= 0xbfU;
    };
    for (std::size_t index = 0U; index < value.size();) {
        const auto first = static_cast<unsigned char>(value[index]);
        if (first <= 0x7fU) {
            if (first < 0x20U || first == 0x7fU) {
                return false;
            }
            ++index;
            continue;
        }
        if (first >= 0xc2U && first <= 0xdfU) {
            if (index + 1U >= value.size()) {
                return false;
            }
            const auto second = static_cast<unsigned char>(value[index + 1U]);
            if (!continuation(second)
                || (first == 0xc2U && second <= 0x9fU)) {
                return false;
            }
            index += 2U;
            continue;
        }
        if (first >= 0xe0U && first <= 0xefU) {
            if (index + 2U >= value.size()) {
                return false;
            }
            const auto second = static_cast<unsigned char>(value[index + 1U]);
            const auto third = static_cast<unsigned char>(value[index + 2U]);
            if (!continuation(third)
                || (first == 0xe0U && (second < 0xa0U || second > 0xbfU))
                || (first == 0xedU && (second < 0x80U || second > 0x9fU))
                || ((first != 0xe0U && first != 0xedU)
                    && !continuation(second))) {
                return false;
            }
            index += 3U;
            continue;
        }
        if (first >= 0xf0U && first <= 0xf4U) {
            if (index + 3U >= value.size()) {
                return false;
            }
            const auto second = static_cast<unsigned char>(value[index + 1U]);
            const auto third = static_cast<unsigned char>(value[index + 2U]);
            const auto fourth = static_cast<unsigned char>(value[index + 3U]);
            if (!continuation(third) || !continuation(fourth)
                || (first == 0xf0U && (second < 0x90U || second > 0xbfU))
                || (first == 0xf4U && (second < 0x80U || second > 0x8fU))
                || ((first != 0xf0U && first != 0xf4U)
                    && !continuation(second))) {
                return false;
            }
            index += 4U;
            continue;
        }
        return false;
    }
    return true;
}

bool bounded_text(std::string_view value, std::size_t maximum) {
    return !value.empty() && value.size() <= maximum
           && valid_utf8_evidence_text(value);
}

bool portable_identifier(std::string_view value, std::size_t maximum) {
    return bounded_text(value, maximum)
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

std::string provider_name_digest(std::string_view provider_name) {
    std::string material{"genesis.security.registered_provider_name.v1"};
    append_material(material, provider_name);
    return runtime::sha256(material);
}

std::string inventory_digest(const CryptoPlatformInventory& inventory) {
    std::string material{"genesis.security.crypto_platform_inventory.v1"};
    append_material(material, inventory.platform_id);
    append_material(material, inventory.architecture);
    append_material(material, to_string(inventory.status));
    append_material(material, std::to_string(inventory.native_status));
    append_material(material, std::to_string(inventory.collected_at));
    append_material(material, std::to_string(inventory.elapsed_microseconds));
    append_material(material,
                    inventory.provider_registry_enumeration_attempted ? "1" : "0");
    append_material(material,
                    inventory.provider_registry_enumeration_succeeded ? "1" : "0");
    append_material(material, inventory.provider_opened ? "1" : "0");
    append_material(material, inventory.key_enumerated ? "1" : "0");
    append_material(material, inventory.key_opened ? "1" : "0");
    append_material(material, inventory.key_created ? "1" : "0");
    append_material(material,
                    inventory.cryptographic_operation_executed ? "1" : "0");
    append_material(material, inventory.provider_qualified ? "1" : "0");
    append_material(material, std::to_string(inventory.registered_providers.size()));
    for (const auto& provider : inventory.registered_providers) {
        append_material(material, provider.provider_name);
        append_material(material, provider.provider_name_digest);
    }
    return runtime::sha256(material);
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
        case '\b':
            output += "\\b";
            break;
        case '\f':
            output += "\\f";
            break;
        case '\n':
            output += "\\n";
            break;
        case '\r':
            output += "\\r";
            break;
        case '\t':
            output += "\\t";
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

} // namespace

std::string_view to_string(CryptoPlatformInventoryStatus value) noexcept {
    const auto index = static_cast<std::size_t>(value);
    return index < kStatusNames.size() ? kStatusNames[index] : std::string_view{};
}

bool crypto_platform_inventory_status_from_string(
    std::string_view text,
    CryptoPlatformInventoryStatus& value) noexcept {
    const auto found = std::find(kStatusNames.begin(), kStatusNames.end(), text);
    if (found == kStatusNames.end()) {
        return false;
    }
    value = static_cast<CryptoPlatformInventoryStatus>(
        std::distance(kStatusNames.begin(), found));
    return true;
}

CryptoPlatformInventory make_crypto_platform_inventory(
    CryptoPlatformInventoryDraft draft) {
    if (!portable_identifier(draft.platform_id, kMaximumPlatformIdLength)) {
        throw std::invalid_argument("platform_id is not a bounded portable identifier");
    }
    if (!portable_identifier(draft.architecture, kMaximumArchitectureLength)) {
        throw std::invalid_argument("architecture is not a bounded portable identifier");
    }
    if (to_string(draft.status).empty()) {
        throw std::invalid_argument("inventory status is outside the serialized range");
    }
    if (draft.registered_provider_names.size() > kMaximumProviderCount) {
        throw std::invalid_argument("registered provider count exceeds the evidence limit");
    }
    for (const auto& name : draft.registered_provider_names) {
        if (!bounded_text(name, kMaximumProviderNameLength)) {
            throw std::invalid_argument("provider name is empty, unbounded, or contains control text");
        }
    }
    std::sort(draft.registered_provider_names.begin(),
              draft.registered_provider_names.end());
    if (std::adjacent_find(draft.registered_provider_names.begin(),
                           draft.registered_provider_names.end())
        != draft.registered_provider_names.end()) {
        throw std::invalid_argument("duplicate registered provider name");
    }

    const bool observed = draft.status == CryptoPlatformInventoryStatus::observed;
    const bool failed = draft.status == CryptoPlatformInventoryStatus::enumeration_failed;
    const bool unsupported =
        draft.status == CryptoPlatformInventoryStatus::unsupported_platform;
    if ((observed && (!draft.provider_registry_enumeration_attempted
                      || draft.native_status != 0U))
        || (failed && (!draft.provider_registry_enumeration_attempted
                       || draft.native_status == 0U
                       || !draft.registered_provider_names.empty()))
        || (unsupported && (draft.provider_registry_enumeration_attempted
                            || draft.native_status != 0U
                            || !draft.registered_provider_names.empty()))) {
        throw std::invalid_argument("inventory status conflicts with enumeration evidence");
    }

    CryptoPlatformInventory inventory;
    inventory.platform_id = std::move(draft.platform_id);
    inventory.architecture = std::move(draft.architecture);
    inventory.status = draft.status;
    inventory.native_status = draft.native_status;
    inventory.collected_at = draft.collected_at;
    inventory.elapsed_microseconds = draft.elapsed_microseconds;
    inventory.provider_registry_enumeration_attempted =
        draft.provider_registry_enumeration_attempted;
    inventory.provider_registry_enumeration_succeeded = observed;
    inventory.registered_providers.reserve(draft.registered_provider_names.size());
    for (auto& name : draft.registered_provider_names) {
        const auto name_digest = provider_name_digest(name);
        inventory.registered_providers.push_back(
            {std::move(name), std::move(name_digest)});
    }
    inventory.evidence_digest = inventory_digest(inventory);
    return inventory;
}

bool CryptoPlatformInventory::verify() const {
    try {
        if (!portable_identifier(platform_id, kMaximumPlatformIdLength)
            || !portable_identifier(architecture, kMaximumArchitectureLength)
            || to_string(status).empty() || !digest(evidence_digest)
            || registered_providers.size() > kMaximumProviderCount
            || provider_opened || key_enumerated || key_opened || key_created
            || cryptographic_operation_executed || provider_qualified) {
            return false;
        }
        const bool observed = status == CryptoPlatformInventoryStatus::observed;
        const bool failed = status == CryptoPlatformInventoryStatus::enumeration_failed;
        const bool unsupported =
            status == CryptoPlatformInventoryStatus::unsupported_platform;
        if (provider_registry_enumeration_succeeded != observed
            || (observed && (!provider_registry_enumeration_attempted
                             || native_status != 0U))
            || (failed && (!provider_registry_enumeration_attempted
                           || provider_registry_enumeration_succeeded
                           || native_status == 0U || !registered_providers.empty()))
            || (unsupported && (provider_registry_enumeration_attempted
                                || provider_registry_enumeration_succeeded
                                || native_status != 0U
                                || !registered_providers.empty()))) {
            return false;
        }
        for (std::size_t index = 0U; index < registered_providers.size(); ++index) {
            const auto& provider = registered_providers[index];
            if (!bounded_text(provider.provider_name, kMaximumProviderNameLength)
                || !digest(provider.provider_name_digest)
                || provider.provider_name_digest
                       != provider_name_digest(provider.provider_name)
                || (index > 0U
                    && registered_providers[index - 1U].provider_name
                           >= provider.provider_name)) {
                return false;
            }
        }
        return evidence_digest == inventory_digest(*this);
    } catch (...) {
        return false;
    }
}

std::string crypto_platform_inventory_json(
    const CryptoPlatformInventory& inventory) {
    if (!inventory.verify()) {
        throw std::invalid_argument("cannot serialize an invalid platform inventory");
    }
    std::string output;
    output.reserve(1024U + inventory.registered_providers.size() * 256U);
    output += "{\"schema\":\"genesis.security.crypto_platform_inventory.v1\",\"platform_id\":";
    append_json_string(output, inventory.platform_id);
    output += ",\"architecture\":";
    append_json_string(output, inventory.architecture);
    output += ",\"status\":";
    append_json_string(output, to_string(inventory.status));
    output += ",\"native_status\":" + std::to_string(inventory.native_status);
    output += ",\"collected_at\":" + std::to_string(inventory.collected_at);
    output += ",\"elapsed_microseconds\":"
              + std::to_string(inventory.elapsed_microseconds);
    output += ",\"provider_registry_enumeration_attempted\":";
    append_json_boolean(output, inventory.provider_registry_enumeration_attempted);
    output += ",\"provider_registry_enumeration_succeeded\":";
    append_json_boolean(output, inventory.provider_registry_enumeration_succeeded);
    output += ",\"provider_opened\":";
    append_json_boolean(output, inventory.provider_opened);
    output += ",\"key_enumerated\":";
    append_json_boolean(output, inventory.key_enumerated);
    output += ",\"key_opened\":";
    append_json_boolean(output, inventory.key_opened);
    output += ",\"key_created\":";
    append_json_boolean(output, inventory.key_created);
    output += ",\"cryptographic_operation_executed\":";
    append_json_boolean(output, inventory.cryptographic_operation_executed);
    output += ",\"provider_qualified\":";
    append_json_boolean(output, inventory.provider_qualified);
    output += ",\"evidence_digest\":";
    append_json_string(output, inventory.evidence_digest);
    output += ",\"registered_providers\":[";
    for (std::size_t index = 0U; index < inventory.registered_providers.size();
         ++index) {
        if (index != 0U) {
            output.push_back(',');
        }
        output += "{\"provider_name\":";
        append_json_string(output,
                           inventory.registered_providers[index].provider_name);
        output += ",\"provider_name_digest\":";
        append_json_string(
            output,
            inventory.registered_providers[index].provider_name_digest);
        output.push_back('}');
    }
    output += "]}";
    return output;
}

} // namespace genesis::security
