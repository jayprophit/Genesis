#include "genesis/security/provider_open_observation.hpp"

#include "genesis/runtime/runtime.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

using namespace genesis::security;

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

std::string digest(std::string_view value) {
    return genesis::runtime::sha256(value);
}

CryptoProviderOpenObservationDraft observed_draft() {
    return {"windows-cng-ncrypt",
            "x86_64",
            "Synthetic Provider",
            digest("registration inventory"),
            CryptoProviderOpenStatus::observed,
            0U,
            0U,
            0U,
            0x80000023U,
            1'725'100'000U,
            340U,
            true,
            true,
            true,
            true,
            true,
            true};
}

void test_provider_name_contract() {
    const std::string valid = "Microsoft Software Key Storage Provider";
    require(valid_registered_crypto_provider_name(valid)
                && derive_registered_crypto_provider_name_digest(valid)
                       == "ae1c88200ab1f3cf1b6bc49bebbf00f8bec874c2a1fb1c0dd230dcaa937e8f11",
            "valid provider name did not retain stable identity evidence");
    const std::string malformed =
        std::string{"invalid-"} + static_cast<char>(0xc0U)
        + static_cast<char>(0xafU);
    require(!valid_registered_crypto_provider_name("")
                && !valid_registered_crypto_provider_name("bad\nprovider")
                && !valid_registered_crypto_provider_name(malformed),
            "invalid provider evidence text was accepted");
    require_throws(
        [&] {
            static_cast<void>(derive_registered_crypto_provider_name_digest(
                malformed));
        },
        "invalid provider text received a derived identity digest");
}

void test_status_codec() {
    for (const auto status : {CryptoProviderOpenStatus::observed,
                              CryptoProviderOpenStatus::unsupported_platform,
                              CryptoProviderOpenStatus::inventory_unavailable,
                              CryptoProviderOpenStatus::not_registered,
                              CryptoProviderOpenStatus::open_failed,
                              CryptoProviderOpenStatus::property_query_failed,
                              CryptoProviderOpenStatus::release_failed}) {
        CryptoProviderOpenStatus restored{};
        require(!to_string(status).empty()
                    && crypto_provider_open_status_from_string(
                        to_string(status), restored)
                    && restored == status,
                "provider-open status did not roundtrip");
    }
    CryptoProviderOpenStatus unchanged = CryptoProviderOpenStatus::observed;
    require(!crypto_provider_open_status_from_string("qualified", unchanged)
                && unchanged == CryptoProviderOpenStatus::observed
                && to_string(static_cast<CryptoProviderOpenStatus>(255U)).empty(),
            "unknown provider-open status was accepted");
}

void test_canonical_observation() {
    const auto observation = make_crypto_provider_open_observation(observed_draft());
    require(observation.verify() && observation.provider_open_succeeded
                && observation.implementation_type_query_succeeded
                && observation.provider_handle_released
                && observation.provider_reports_hardware
                && observation.provider_reports_software
                && observation.provider_reports_virtual_isolation
                && !observation.provider_reports_removable
                && !observation.provider_reports_hardware_rng
                && observation.unknown_implementation_type_flags == 0x80000000U
                && !observation.key_enumerated && !observation.key_opened
                && !observation.key_created && !observation.key_exported
                && !observation.cryptographic_operation_executed
                && !observation.provider_qualified,
            "provider-open observation lost flags or crossed the key boundary");
    require(observation
                == make_crypto_provider_open_observation(observed_draft()),
            "provider-open evidence is not deterministic");

    auto tampered = observation;
    tampered.provider_reports_hardware = false;
    require(!tampered.verify(), "implementation-type flag tampering was accepted");
    tampered = observation;
    tampered.key_enumerated = true;
    require(!tampered.verify(), "key-enumeration claim crossed the open boundary");
    tampered = observation;
    tampered.provider_handle_released = false;
    require(!tampered.verify(), "unreleased observed handle was accepted");
    tampered = observation;
    tampered.provider_name_digest[0] =
        tampered.provider_name_digest[0] == '0' ? '1' : '0';
    require(!tampered.verify(), "provider-name digest tampering was accepted");
    tampered = observation;
    tampered.evidence_digest[0] = tampered.evidence_digest[0] == '0' ? '1' : '0';
    require(!tampered.verify(), "provider-open evidence tampering was accepted");
}

void test_failure_state_boundaries() {
    auto open_failed = observed_draft();
    open_failed.status = CryptoProviderOpenStatus::open_failed;
    open_failed.open_native_status = 0x80090030U;
    open_failed.property_native_status = 0U;
    open_failed.implementation_type_flags = 0U;
    open_failed.provider_open_succeeded = false;
    open_failed.implementation_type_query_attempted = false;
    open_failed.implementation_type_query_succeeded = false;
    open_failed.provider_handle_released = false;
    require(make_crypto_provider_open_observation(open_failed).verify(),
            "valid provider-open failure evidence was rejected");
    open_failed.implementation_type_query_attempted = true;
    require_throws(
        [&] { static_cast<void>(make_crypto_provider_open_observation(open_failed)); },
        "property query was accepted after provider-open failure");

    auto property_failed = observed_draft();
    property_failed.status = CryptoProviderOpenStatus::property_query_failed;
    property_failed.property_native_status = 0x80090029U;
    property_failed.implementation_type_flags = 0U;
    property_failed.implementation_type_query_succeeded = false;
    require(make_crypto_provider_open_observation(property_failed).verify(),
            "valid property-query failure evidence was rejected");

    auto release_failed_after_query = observed_draft();
    release_failed_after_query.status = CryptoProviderOpenStatus::release_failed;
    release_failed_after_query.release_native_status = 0x80090026U;
    release_failed_after_query.provider_handle_released = false;
    require(make_crypto_provider_open_observation(release_failed_after_query).verify(),
            "valid release failure after a successful query was rejected");

    auto release_failed_after_property = observed_draft();
    release_failed_after_property.status = CryptoProviderOpenStatus::release_failed;
    release_failed_after_property.property_native_status = 0x80090029U;
    release_failed_after_property.release_native_status = 0x80090026U;
    release_failed_after_property.implementation_type_flags = 0U;
    release_failed_after_property.implementation_type_query_succeeded = false;
    release_failed_after_property.provider_handle_released = false;
    require(make_crypto_provider_open_observation(release_failed_after_property)
                .verify(),
            "combined property and release failure evidence was rejected");

    auto not_registered = observed_draft();
    not_registered.status = CryptoProviderOpenStatus::not_registered;
    not_registered.implementation_type_flags = 0U;
    not_registered.registration_matched = false;
    not_registered.provider_open_attempted = false;
    not_registered.provider_open_succeeded = false;
    not_registered.implementation_type_query_attempted = false;
    not_registered.implementation_type_query_succeeded = false;
    not_registered.provider_handle_released = false;
    require(make_crypto_provider_open_observation(not_registered).verify(),
            "valid not-registered evidence was rejected");

    not_registered.registration_matched = true;
    require_throws(
        [&] { static_cast<void>(make_crypto_provider_open_observation(not_registered)); },
        "not-registered evidence falsely retained a registration match");
}

void test_json_boundary() {
    const auto observation = make_crypto_provider_open_observation(observed_draft());
    const auto json = crypto_provider_open_observation_json(observation);
    require(json.find("\"provider_open_succeeded\":true") != std::string::npos
                && json.find("\"provider_handle_released\":true")
                       != std::string::npos
                && json.find("\"key_enumerated\":false") != std::string::npos
                && json.find("\"key_created\":false") != std::string::npos
                && json.find("\"cryptographic_operation_executed\":false")
                       != std::string::npos
                && json.find("\"provider_qualified\":false")
                       != std::string::npos,
            "provider-open JSON omitted its positive or negative evidence");
}

void test_preflight_denial_states() {
    const auto unavailable_inventory = make_crypto_platform_inventory(
        {"windows-cng-ncrypt",
         "x86_64",
         CryptoPlatformInventoryStatus::enumeration_failed,
         5U,
         1'725'099'900U,
         10U,
         true,
         {}});
    const auto unavailable = probe_registered_crypto_provider_open(
        unavailable_inventory,
        "Microsoft Software Key Storage Provider",
        1'725'099'901U);
    require(unavailable.verify()
                && unavailable.status
                       == CryptoProviderOpenStatus::inventory_unavailable
                && !unavailable.registration_matched
                && !unavailable.provider_open_attempted,
            "unavailable inventory did not deny provider open before native use");

    const auto unsupported_inventory = make_crypto_platform_inventory(
        {"unsupported",
         "x86_64",
         CryptoPlatformInventoryStatus::unsupported_platform,
         0U,
         1'725'099'902U,
         1U,
         false,
         {}});
    const auto unsupported = probe_registered_crypto_provider_open(
        unsupported_inventory,
        "Microsoft Software Key Storage Provider",
        1'725'099'903U);
    require(unsupported.verify()
                && unsupported.status
                       == CryptoProviderOpenStatus::unsupported_platform
                && !unsupported.provider_open_attempted,
            "unsupported platform did not retain a no-native-activity record");
}

void test_native_observation() {
    const auto inventory = probe_registered_crypto_providers(1'725'100'001U);
    require_throws(
        [&] {
            static_cast<void>(probe_registered_crypto_provider_open(
                inventory,
                "Microsoft Software Key Storage Provider",
                1'725'100'000U));
        },
        "provider-open observation was allowed to predate its inventory");

    auto tampered_inventory = inventory;
    tampered_inventory.evidence_digest[0] =
        tampered_inventory.evidence_digest[0] == '0' ? '1' : '0';
    require_throws(
        [&] {
            static_cast<void>(probe_registered_crypto_provider_open(
                tampered_inventory,
                "Microsoft Software Key Storage Provider",
                1'725'100'002U));
        },
        "provider-open probe accepted a corrupt inventory");
#if defined(_WIN32)
    const std::string provider = "Microsoft Software Key Storage Provider";
    const auto observation = probe_registered_crypto_provider_open(
        inventory, provider, 1'725'100'002U);
    require(observation.verify()
                && observation.status == CryptoProviderOpenStatus::observed
                && observation.registration_matched
                && observation.provider_open_succeeded
                && observation.implementation_type_query_succeeded
                && observation.provider_handle_released
                && observation.provider_reports_software
                && !observation.key_enumerated && !observation.key_opened
                && !observation.key_created && !observation.key_exported
                && !observation.cryptographic_operation_executed
                && !observation.provider_qualified,
            "Windows software KSP open observation failed or crossed its boundary");
    const auto missing = probe_registered_crypto_provider_open(
        inventory, "Genesis Deliberately Missing Provider", 1'725'100'003U);
    require(missing.verify()
                && missing.status == CryptoProviderOpenStatus::not_registered
                && !missing.provider_open_attempted,
            "unregistered provider reached the native open API");
#else
    const auto observation = probe_registered_crypto_provider_open(
        inventory, "Microsoft Software Key Storage Provider", 1'725'100'002U);
    require(observation.verify()
                && observation.status
                       == CryptoProviderOpenStatus::unsupported_platform
                && !observation.provider_open_attempted,
            "non-Windows provider-open route was not explicitly unsupported");
#endif
}

} // namespace

int main() {
    try {
        test_provider_name_contract();
        test_status_codec();
        test_canonical_observation();
        test_failure_state_boundaries();
        test_json_boundary();
        test_preflight_denial_states();
        test_native_observation();
        std::cout << "provider open observation tests passed\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "provider open observation tests failed: " << exception.what()
                  << '\n';
        return 1;
    }
}
