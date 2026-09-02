#include "genesis/runtime/runtime.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

genesis::runtime::EventEnvelope event(std::uint64_t sequence,
                                      std::uint64_t logical_time,
                                      std::string id,
                                      std::string topic,
                                      std::optional<std::string> parent = std::nullopt) {
    return genesis::runtime::EventEnvelope{sequence,
                                           logical_time,
                                           std::move(id),
                                           "test-source",
                                           std::move(topic),
                                           std::move(parent),
                                           genesis::runtime::sha256("payload")};
}

void test_hash_and_clock() {
    check(genesis::runtime::sha256("") ==
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
          "SHA-256 empty vector failed");
    check(genesis::runtime::sha256("abc") ==
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "SHA-256 abc vector failed");

    genesis::runtime::LogicalClock clock{4};
    check(clock.value() == 4 && clock.tick() == 5, "logical clock tick failed");
    check(clock.advance_to(10) == 10, "logical clock advance failed");
    bool rejected = false;
    try {
        static_cast<void>(clock.advance_to(10));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "logical clock accepted a non-advancing value");
}

void test_dispatcher() {
    genesis::runtime::DeterministicDispatcher dispatcher{2};
    std::vector<std::string> calls;
    const auto second = dispatcher.subscribe("start", [&](const auto& value) {
        calls.push_back("second:" + value.event_id());
    });
    const auto first = dispatcher.subscribe("start", [&](const auto& value) {
        calls.push_back("first:" + value.event_id());
    });
    check(first > second, "subscription IDs are not monotonic");

    auto first_outcome = dispatcher.publish({"e1", "source", "start", std::nullopt,
                                              genesis::runtime::sha256("one")});
    check(first_outcome.status == genesis::runtime::DispatchStatus::succeeded,
          "first event was not dispatched");
    check(calls.size() == 2 && calls[0] == "second:e1" && calls[1] == "first:e1",
          "subscription order is not deterministic");

    const auto one_shot = dispatcher.subscribe(
        "once", [&](const auto&) { calls.push_back("once"); }, {.one_shot = true});
    check(one_shot != 0, "one-shot subscription was not created");
    static_cast<void>(dispatcher.publish({"e2", "source", "once", std::string{"e1"},
                                          genesis::runtime::sha256("two")}));
    static_cast<void>(dispatcher.publish({"e3", "source", "start", std::string{"e2"},
                                          genesis::runtime::sha256("three")}));
    check(dispatcher.history_size() == 2 && dispatcher.history_eviction_count() == 1,
          "bounded history did not evict deterministically");
    check(std::count(calls.begin(), calls.end(), "once") == 1,
          "one-shot subscription fired more than once");

    bool rejected = false;
    try {
        static_cast<void>(dispatcher.publish({"e3", "source", "start", std::string{"e2"},
                                              genesis::runtime::sha256("duplicate")}));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "duplicate event ID was accepted");

    genesis::runtime::DeterministicDispatcher replay_target;
    std::size_t replay_calls = 0;
    static_cast<void>(replay_target.subscribe("start", [&](const auto&) { ++replay_calls; }));
    const auto e1 = event(1, 1, "r1", "start");
    const auto e2 = event(2, 2, "r2", "start", std::string{"r1"});
    const std::array batch{e1, e2};
    const auto replayed = replay_target.replay(batch);
    check(replayed.size() == 2 && replay_calls == 2 && replay_target.next_sequence() == 3,
          "dispatcher replay failed");

    const auto bad = event(4, 4, "bad", "start");
    rejected = false;
    try {
        static_cast<void>(replay_target.replay(std::span<const genesis::runtime::EventEnvelope>{&bad, 1}));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && replay_target.next_sequence() == 3 && replay_calls == 2,
          "invalid replay changed dispatcher state");
}

void test_resources() {
    genesis::runtime::ResourceAccounts accounts{{{"cpu", 10}, {"memory", 8}}};
    genesis::runtime::ReservationError error;
    auto reservation = accounts.try_reserve({{"cpu", 4}, {"memory", 3}}, &error);
    check(reservation.has_value() && error.code == genesis::runtime::ReservationErrorCode::none,
          "valid resource reservation failed");
    const auto snapshot = accounts.snapshot("cpu");
    check(snapshot && snapshot->reserved == 4 && snapshot->available() == 6,
          "resource reservation accounting failed");

    auto insufficient = accounts.try_reserve({{"cpu", 7}}, &error);
    check(!insufficient && error.code == genesis::runtime::ReservationErrorCode::insufficient_capacity,
          "over-allocation was accepted");
    check(reservation->commit(), "resource commit failed");
    check(accounts.snapshot("cpu")->current == 4, "committed usage was not accounted");
    check(reservation->release(), "resource release failed");
    check(accounts.active_reservations() == 0, "released reservation remained active");

    auto rollback = accounts.try_reserve({{"cpu", 2}}, &error);
    check(rollback && rollback->commit(), "second resource commit failed");
    check(!accounts.set_capacity("cpu", 1, &error)
              && error.code == genesis::runtime::ReservationErrorCode::capacity_below_usage,
          "capacity reduction below usage was accepted");
    check(rollback->release(), "second resource release failed");
    auto rollback_after_release = accounts.try_reserve({{"cpu", 2}}, &error);
    check(rollback_after_release && rollback_after_release->rollback(), "resource rollback failed");
    check(accounts.snapshot("cpu")->available() == 10, "rollback did not restore capacity");
}

void test_state_machine() {
    genesis::runtime::CausalStateMachine machine{
        "boot", {{"boot", "start", "ready"}, {"ready", "stop", "stopped"}}};
    const auto start = event(1, 1, "s1", "start");
    const auto stop = event(2, 2, "s2", "stop", std::string{"s1"});
    check(machine.apply(start).applied(), "state machine start failed");
    check(machine.apply(stop).applied() && machine.current_state() == "stopped",
          "state machine stop failed");
    check(machine.verify() && machine.history_snapshot().size() == 2,
          "state machine replay chain verification failed");
    const auto duplicate = machine.apply(stop);
    check(duplicate.status == genesis::runtime::StateTransitionStatus::sequence_mismatch,
          "state machine accepted an out-of-sequence event");

    genesis::runtime::CausalStateMachine replay_machine{
        "boot", {{"boot", "start", "ready"}, {"ready", "stop", "stopped"}}};
    const auto bad_topic = event(2, 2, "bad", "unknown", std::string{"s1"});
    const std::array invalid_batch{start, bad_topic};
    const auto replay_outcomes = replay_machine.replay(invalid_batch);
    check(replay_outcomes.size() == 2 && replay_outcomes[0].applied()
              && !replay_outcomes[1].applied() && replay_machine.current_state() == "boot"
              && replay_machine.next_sequence() == 1 && replay_machine.history_snapshot().empty(),
          "invalid state-machine replay was not atomic");
}

} // namespace

int main() {
    try {
        test_hash_and_clock();
        test_dispatcher();
        test_resources();
        test_state_machine();
        std::cout << "All runtime tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
