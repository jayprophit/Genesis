#include "genesis/security/platform_crypto_inventory.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using genesis::security::CryptoPlatformInventoryDraft;
using genesis::security::CryptoPlatformInventoryStatus;
using genesis::security::crypto_platform_inventory_json;
using genesis::security::crypto_platform_inventory_status_from_string;
using genesis::security::make_crypto_platform_inventory;
using genesis::security::probe_registered_crypto_providers;
using genesis::security::to_string;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Function>
void require_throws(Function&& function, const char* message) {
    try {
        std::forward<Function>(function)();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(message);
}

CryptoPlatformInventoryDraft observed_draft() {
    return {"windows-cng-ncrypt",
            "x86_64",
            CryptoPlatformInventoryStatus::observed,
            0U,
            1'725'000'000U,
            912U,
            true,
            {"Provider Z", "Provider A"}};
}

void test_status_codec() {
    for (const auto expected : {CryptoPlatformInventoryStatus::observed,
                                CryptoPlatformInventoryStatus::unsupported_platform,
                                CryptoPlatformInventoryStatus::enumeration_failed}) {
        CryptoPlatformInventoryStatus restored{};
        require(!to_string(expected).empty()
                    && crypto_platform_inventory_status_from_string(
                        to_string(expected), restored)
                    && restored == expected,
                "platform inventory status did not roundtrip");
    }
    CryptoPlatformInventoryStatus unchanged =
        CryptoPlatformInventoryStatus::observed;
    require(!crypto_platform_inventory_status_from_string("qualified", unchanged)
                && unchanged == CryptoPlatformInventoryStatus::observed
                && to_string(static_cast<CryptoPlatformInventoryStatus>(255U)).empty(),
            "unknown platform inventory status was accepted");
}

void test_canonical_observation_and_tamper_detection() {
    const auto inventory = make_crypto_platform_inventory(observed_draft());
    require(inventory.verify() && inventory.registered_providers.size() == 2U
                && inventory.registered_providers[0].provider_name == "Provider A"
                && inventory.registered_providers[1].provider_name == "Provider Z"
                && inventory.provider_registry_enumeration_attempted
                && inventory.provider_registry_enumeration_succeeded
                && !inventory.provider_opened && !inventory.key_enumerated
                && !inventory.key_opened && !inventory.key_created
                && !inventory.cryptographic_operation_executed
                && !inventory.provider_qualified,
            "observed inventory did not retain its canonical evidence boundary");
    const auto equivalent = make_crypto_platform_inventory(observed_draft());
    require(inventory == equivalent,
            "equivalent platform inventory did not produce deterministic evidence");

    auto tampered = inventory;
    tampered.registered_providers[0].provider_name = "Substituted Provider";
    require(!tampered.verify(), "provider-name substitution was not detected");
    tampered = inventory;
    tampered.provider_opened = true;
    require(!tampered.verify(), "provider-open claim crossed the inventory boundary");
    tampered = inventory;
    tampered.provider_qualified = true;
    require(!tampered.verify(), "qualification claim crossed the inventory boundary");
    tampered = inventory;
    tampered.evidence_digest[0] = tampered.evidence_digest[0] == '0' ? '1' : '0';
    require(!tampered.verify(), "inventory digest corruption was not detected");
}

void test_invalid_state_rejection() {
    auto duplicate = observed_draft();
    duplicate.registered_provider_names = {"Provider A", "Provider A"};
    require_throws([&] { static_cast<void>(make_crypto_platform_inventory(duplicate)); },
                   "duplicate provider name was accepted");
    auto control = observed_draft();
    control.registered_provider_names = {"bad\nprovider"};
    require_throws([&] { static_cast<void>(make_crypto_platform_inventory(control)); },
                   "control-bearing provider name was accepted");
    auto c1_control = observed_draft();
    c1_control.registered_provider_names = {
        std::string{"c1-control-"} + static_cast<char>(0xc2U)
        + static_cast<char>(0x85U)};
    require_throws(
        [&] { static_cast<void>(make_crypto_platform_inventory(c1_control)); },
        "C1-control-bearing provider name was accepted");
    auto invalid_utf8 = observed_draft();
    invalid_utf8.registered_provider_names = {
        std::string{"invalid-utf8-"} + static_cast<char>(0xc0U)
        + static_cast<char>(0xafU)};
    require_throws(
        [&] { static_cast<void>(make_crypto_platform_inventory(invalid_utf8)); },
        "non-canonical UTF-8 provider name was accepted");
    auto false_success = observed_draft();
    false_success.provider_registry_enumeration_attempted = false;
    require_throws(
        [&] { static_cast<void>(make_crypto_platform_inventory(false_success)); },
        "observed state without an API attempt was accepted");
    auto partial_failure = observed_draft();
    partial_failure.status = CryptoPlatformInventoryStatus::enumeration_failed;
    partial_failure.native_status = 5U;
    require_throws(
        [&] { static_cast<void>(make_crypto_platform_inventory(partial_failure)); },
        "failed enumeration retained partial provider data");
    auto unsupported = observed_draft();
    unsupported.status = CryptoPlatformInventoryStatus::unsupported_platform;
    unsupported.provider_registry_enumeration_attempted = false;
    unsupported.registered_provider_names.clear();
    require(make_crypto_platform_inventory(std::move(unsupported)).verify(),
            "valid unsupported-platform evidence was rejected");
}

void test_json_evidence() {
    const auto inventory = make_crypto_platform_inventory(observed_draft());
    const auto json = crypto_platform_inventory_json(inventory);
    require(json.find("\"schema\":\"genesis.security.crypto_platform_inventory.v1\"")
                    != std::string::npos
                && json.find("\"provider_opened\":false") != std::string::npos
                && json.find("\"key_created\":false") != std::string::npos
                && json.find("\"cryptographic_operation_executed\":false")
                       != std::string::npos
                && json.find("\"provider_qualified\":false")
                       != std::string::npos,
            "JSON evidence omitted its schema or negative capability claims");
    auto invalid = inventory;
    invalid.key_created = true;
    require_throws([&] { static_cast<void>(crypto_platform_inventory_json(invalid)); },
                   "invalid platform evidence was serialized");
}

void test_native_probe() {
    const auto inventory = probe_registered_crypto_providers(1'725'000'001U);
    require(inventory.verify() && inventory.collected_at == 1'725'000'001U
                && !inventory.provider_opened && !inventory.key_enumerated
                && !inventory.key_opened && !inventory.key_created
                && !inventory.cryptographic_operation_executed
                && !inventory.provider_qualified,
            "native platform probe crossed its read-only evidence boundary");
#if defined(_WIN32)
    require(inventory.status == CryptoPlatformInventoryStatus::observed
                && inventory.platform_id == "windows-cng-ncrypt"
                && inventory.provider_registry_enumeration_attempted
                && inventory.provider_registry_enumeration_succeeded
                && !inventory.registered_providers.empty(),
            "Windows CNG provider registry was not observed");
#else
    require(inventory.status
                    == CryptoPlatformInventoryStatus::unsupported_platform
                && inventory.platform_id == "unsupported"
                && !inventory.provider_registry_enumeration_attempted
                && inventory.registered_providers.empty(),
            "non-Windows platform did not report an explicit unsupported state");
#endif
}

} // namespace

int main() {
    try {
        test_status_codec();
        test_canonical_observation_and_tamper_detection();
        test_invalid_state_rejection();
        test_json_evidence();
        test_native_probe();
        std::cout << "platform crypto inventory tests passed\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "platform crypto inventory tests failed: " << exception.what()
                  << '\n';
        return 1;
    }
}
