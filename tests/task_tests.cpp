#include "genesis/agents/tasks.hpp"
#include "test_support.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using genesis::agents::DurableTask;
using genesis::agents::TaskError;
using genesis::agents::TaskRegistry;
using genesis::agents::TaskState;
using genesis::agents::TaskStore;

// The counter is the harness's, so the summary line reports
// the checks that were really made.
int& g_checks = genesis::testing::g_checks;

void require(bool condition, const char* message) {
    ++g_checks;
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_equal(TaskError actual, TaskError expected, const char* message) {
    ++g_checks;
    if (actual != expected) {
        std::cerr << "expected " << genesis::agents::to_string(expected) << " but got "
                  << genesis::agents::to_string(actual) << ": " << message << "\n";
        throw std::runtime_error(message);
    }
}

void require_equal(TaskState actual, TaskState expected, const char* message) {
    ++g_checks;
    if (actual != expected) {
        std::cerr << "expected " << genesis::agents::to_string(expected) << " but got "
                  << genesis::agents::to_string(actual) << ": " << message << "\n";
        throw std::runtime_error(message);
    }
}

/// A temporary directory that cleans itself up.
class TempDir final {
public:
    TempDir() {
        path_ = std::filesystem::temp_directory_path() /
                ("genesis-task-tests-" + std::to_string(::rand()));
        std::filesystem::create_directories(path_);
    }
    ~TempDir() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    [[nodiscard]] const std::filesystem::path& path() const { return path_; }
private:
    std::filesystem::path path_;
};

// ---------------------------------------------------------------------------
// Idempotency: the same work is done once.
// ---------------------------------------------------------------------------

void test_a_task_is_submitted_and_is_pending() {
    TaskRegistry registry;
    TaskError error{TaskError::none};
    const auto id = registry.submit("key-1", "write the report", 1'000, &error);
    require(id.has_value(), "the task should be submitted");
    require_equal(error, TaskError::none, "the submission should be accepted");
    const auto* task = registry.find(*id);
    require(task != nullptr, "the task should be found");
    require_equal(task->state, TaskState::pending, "a new task is pending");
    require(task->attempt == 0, "a new task has not been attempted");
}

void test_the_same_key_returns_the_same_task() {
    TaskRegistry registry;
    const auto first = registry.submit("key-1", "write the report", 1'000);
    const auto second = registry.submit("key-1", "write the report", 2'000);
    require(first.has_value() && second.has_value(), "both submissions should succeed");
    require(*first == *second, "the same key must return the same task");
    require(registry.size() == 1, "the same key must not create a second task");
}

void test_a_different_key_creates_a_different_task() {
    TaskRegistry registry;
    const auto first = registry.submit("key-1", "one", 1'000);
    const auto second = registry.submit("key-2", "two", 1'000);
    require(first.has_value() && second.has_value(), "both submissions should succeed");
    require(*first != *second, "different keys must create different tasks");
    require(registry.size() == 2, "there should be two tasks");
}

void test_a_malformed_submission_is_refused() {
    TaskRegistry registry;
    TaskError error{TaskError::none};
    require(!registry.submit("", "a title", 1'000, &error).has_value(),
            "a task with no key must be refused");
    require_equal(error, TaskError::invalid_task, "the refusal must be reported");
    require(!registry.submit("key-1", "", 1'000, &error).has_value(),
            "a task with no title must be refused");
}

// ---------------------------------------------------------------------------
// Leases: one worker at a time, and not for ever.
// ---------------------------------------------------------------------------

void test_a_task_can_be_leased() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    TaskError error{TaskError::none};
    require(registry.lease(id, "worker-a", 1'000, 30'000, &error),
            "the lease should be accepted");
    require_equal(error, TaskError::none, "the lease should be accepted");
    const auto* task = registry.find(id);
    require(task != nullptr, "the task should be found");
    require_equal(task->state, TaskState::leased, "the task is leased");
    require(task->worker_id == "worker-a", "the worker should hold the lease");
    require(task->attempt == 1, "the attempt should be counted");
}

void test_a_second_worker_cannot_lease_a_live_task() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 30'000), "the first lease should be accepted");
    TaskError error{TaskError::none};
    require(!registry.lease(id, "worker-b", 2'000, 30'000, &error),
            "a second worker must not take a live lease");
    require_equal(error, TaskError::lease_held, "the refusal must say the lease is held");
}

void test_a_holder_may_renew_its_lease() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 30'000), "the lease should be accepted");
    require(registry.lease(id, "worker-a", 2'000, 30'000), "the holder should be able to renew");
    const auto* task = registry.find(id);
    require(task != nullptr, "the task should be found");
    require(task->worker_id == "worker-a", "the worker should still hold the lease");
}

void test_an_expired_lease_may_be_taken() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 1'000), "the lease should be accepted");
    // The worker dies. Its lease expires at 2,000.
    TaskError error{TaskError::none};
    require(registry.lease(id, "worker-b", 5'000, 30'000, &error),
            "an expired lease should be available to take");
    require_equal(error, TaskError::none, "the takeover should be accepted");
    const auto* task = registry.find(id);
    require(task != nullptr, "the task should be found");
    require(task->worker_id == "worker-b", "the new worker should hold the lease");
    require(task->attempt == 2, "the second attempt should be counted");
}

void test_leasing_an_unknown_task_is_refused() {
    TaskRegistry registry;
    TaskError error{TaskError::none};
    require(!registry.lease("task_nope", "worker-a", 1'000, 30'000, &error),
            "an unknown task must not be leased");
    require_equal(error, TaskError::unknown_task, "the refusal must be reported");
}

// ---------------------------------------------------------------------------
// Checkpoints: how far it got.
// ---------------------------------------------------------------------------

void test_a_checkpoint_is_recorded() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 30'000), "the lease should be accepted");
    require(registry.checkpoint(id, "worker-a", "half done", 2'000), "the checkpoint should be accepted");
    const auto* task = registry.find(id);
    require(task != nullptr, "the task should be found");
    require_equal(task->state, TaskState::checkpointed, "the task is checkpointed");
    require(task->checkpoint == "half done", "the checkpoint should be kept");
}

void test_a_worker_that_does_not_hold_the_lease_cannot_checkpoint() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 30'000), "the lease should be accepted");
    TaskError error{TaskError::none};
    require(!registry.checkpoint(id, "worker-b", "not mine", 2'000, &error),
            "a worker without the lease must not checkpoint");
    require_equal(error, TaskError::not_lease_holder, "the refusal must be reported");
}

void test_a_worker_whose_lease_expired_cannot_checkpoint() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 1'000), "the lease should be accepted");
    TaskError error{TaskError::none};
    require(!registry.checkpoint(id, "worker-a", "too late", 5'000, &error),
            "an expired lease must not accept a checkpoint");
    require_equal(error, TaskError::lease_expired, "the refusal must be reported");
}

// ---------------------------------------------------------------------------
// Completion and failure.
// ---------------------------------------------------------------------------

void test_a_task_completes() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 30'000), "the lease should be accepted");
    require(registry.complete(id, "worker-a", 2'000), "the completion should be accepted");
    const auto* task = registry.find(id);
    require(task != nullptr, "the task should be found");
    require_equal(task->state, TaskState::completed, "the task is completed");
}

void test_a_completed_task_cannot_be_leased() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 30'000), "the lease should be accepted");
    require(registry.complete(id, "worker-a", 2'000), "the completion should be accepted");
    TaskError error{TaskError::none};
    require(!registry.lease(id, "worker-b", 3'000, 30'000, &error),
            "a completed task must not be leased");
    require_equal(error, TaskError::not_leasable, "the refusal must be reported");
}

void test_a_failed_task_may_be_retried() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 30'000), "the lease should be accepted");
    require(registry.fail(id, "worker-a", "the tool failed", 2'000, 3),
            "the failure should be accepted");
    const auto* task = registry.find(id);
    require(task != nullptr, "the task should be found");
    require_equal(task->state, TaskState::pending, "a failed task returns to pending");
    require(task->last_error == "the tool failed", "the reason should be kept");
    require(registry.lease(id, "worker-b", 3'000, 30'000), "the task should be retryable");
    const auto* retried = registry.find(id);
    require(retried != nullptr, "the task should be found");
    require(retried->attempt == 2, "the retry should be counted");
}

void test_a_task_that_fails_too_often_is_abandoned() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 30'000), "the lease should be accepted");
    TaskError error{TaskError::none};
    require(!registry.fail(id, "worker-a", "still failing", 2'000, 1, &error),
            "the last failure must be refused");
    require_equal(error, TaskError::too_many_attempts, "the refusal must say so");
    const auto* task = registry.find(id);
    require(task != nullptr, "the task should be found");
    require_equal(task->state, TaskState::abandoned, "the task is abandoned");
    require(!registry.lease(id, "worker-b", 3'000, 30'000),
            "an abandoned task must not be leased");
}

// ---------------------------------------------------------------------------
// Recovery: a dead worker's task is picked up again.
// ---------------------------------------------------------------------------

void test_a_dead_workers_task_is_recoverable() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 1'000), "the lease should be accepted");
    // The lease runs to 2,000. While it is live the task is nobody
    // else's to take.
    const auto live = registry.recoverable(1'500);
    require(live.empty(), "a live lease is not recoverable");
    // At the moment it expires it is over: the lease ends when it ends,
    // not a moment after.
    const auto at_expiry = registry.recoverable(2'000);
    require(at_expiry.size() == 1, "an expired lease is recoverable");
    const auto dead = registry.recoverable(5'000);
    require(dead.size() == 1, "an expired lease is recoverable");
    require(dead.front().task_id == id, "the right task should be recoverable");
}

void test_a_pending_task_is_recoverable() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    const auto recoverable = registry.recoverable(1'000);
    require(recoverable.size() == 1, "a pending task is recoverable");
    require(recoverable.front().task_id == id, "the right task should be recoverable");
}

void test_a_finished_task_is_not_recoverable() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 30'000), "the lease should be accepted");
    require(registry.complete(id, "worker-a", 2'000), "the completion should be accepted");
    require(registry.recoverable(100'000).empty(), "a completed task is not recoverable");
}

void test_resuming_keeps_the_checkpoint() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 1'000), "the lease should be accepted");
    require(registry.checkpoint(id, "worker-a", "half done", 1'500), "the checkpoint should be accepted");
    // The worker dies.
    require(registry.resume(id, "worker-b", 5'000, 30'000), "the task should be resumable");
    const auto* task = registry.find(id);
    require(task != nullptr, "the task should be found");
    require_equal(task->state, TaskState::checkpointed, "the task resumes from its checkpoint");
    require(task->checkpoint == "half done", "the checkpoint must survive the crash");
    require(task->worker_id == "worker-b", "the new worker should hold the lease");
    require(task->attempt == 2, "the resumed attempt should be counted");
}

void test_resuming_a_live_task_is_refused() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 30'000), "the lease should be accepted");
    TaskError error{TaskError::none};
    require(!registry.resume(id, "worker-b", 2'000, 30'000, &error),
            "a live task must not be taken over");
    require_equal(error, TaskError::lease_held, "the refusal must say the lease is held");
}

void test_resuming_a_finished_task_is_refused() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 30'000), "the lease should be accepted");
    require(registry.complete(id, "worker-a", 2'000), "the completion should be accepted");
    TaskError error{TaskError::none};
    require(!registry.resume(id, "worker-b", 3'000, 30'000, &error),
            "a finished task must not be resumed");
    require_equal(error, TaskError::not_resumable, "the refusal must be reported");
}

// ---------------------------------------------------------------------------
// Durability: the registry survives a restart.
// ---------------------------------------------------------------------------

void test_the_registry_survives_a_round_trip() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 30'000), "the lease should be accepted");
    require(registry.checkpoint(id, "worker-a", "half done", 2'000), "the checkpoint should be accepted");

    const auto bytes = registry.serialize();
    TaskError error{TaskError::none};
    const auto restored = TaskRegistry::deserialize(bytes, &error);
    require(restored.has_value(), "the registry should be restored");
    require_equal(error, TaskError::none, "the restore should be accepted");
    require(restored->size() == 1, "the task should survive");
    const auto* task = restored->find(id);
    require(task != nullptr, "the task should be found after the restore");
    require_equal(task->state, TaskState::checkpointed, "the state should survive");
    require(task->checkpoint == "half done", "the checkpoint should survive");
    require(task->attempt == 1, "the attempt count should survive");
}

void test_a_corrupt_record_is_refused() {
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    (void)id;
    auto bytes = registry.serialize();
    // Change one byte of the payload. The checksum must catch it.
    bytes[10] = static_cast<char>(bytes[10] ^ 0xFF);
    TaskError error{TaskError::none};
    require(!TaskRegistry::deserialize(bytes, &error).has_value(),
            "a tampered record must be refused");
    require_equal(error, TaskError::corrupt_record, "the refusal must be reported");
}

void test_a_truncated_record_is_refused() {
    TaskRegistry registry;
    (void)registry.submit("key-1", "work", 1'000);
    const auto bytes = registry.serialize();
    TaskError error{TaskError::none};
    require(!TaskRegistry::deserialize(bytes.substr(0, bytes.size() / 2), &error).has_value(),
            "a truncated record must be refused");
    require_equal(error, TaskError::corrupt_record, "the refusal must be reported");
}

void test_the_store_writes_and_reads() {
    TempDir dir;
    TaskStore store{dir.path()};
    TaskRegistry registry;
    const auto id = *registry.submit("key-1", "work", 1'000);
    require(registry.lease(id, "worker-a", 1'000, 30'000), "the lease should be accepted");

    TaskError error{TaskError::none};
    require(store.write(registry, "owner", "v1", &error), "the write should be accepted");
    require_equal(error, TaskError::none, "the write should be accepted");

    const auto restored = store.read("owner", "v1", &error);
    require(restored.has_value(), "the registry should be read back");
    require(restored->size() == 1, "the task should survive the store");
    const auto* task = restored->find(id);
    require(task != nullptr, "the task should be found after the store");
    require(task->worker_id == "worker-a", "the lease should survive the store");
}

void test_writing_the_same_record_twice_is_idempotent() {
    TempDir dir;
    TaskStore store{dir.path()};
    TaskRegistry registry;
    (void)registry.submit("key-1", "work", 1'000);
    require(store.write(registry, "owner", "v1"), "the first write should be accepted");
    // The same snapshot under the same version is the same snapshot.
    // Writing it again is harmless, and refusing it would make a retry
    // after a crash fail for no reason.
    require(store.write(registry, "owner", "v1"),
            "writing the same record again should be accepted");
    const auto restored = store.read("owner", "v1");
    require(restored.has_value(), "the record should still be readable");
    require(restored->size() == 1, "the task should still be there");
}

void test_a_published_version_cannot_be_overwritten() {
    TempDir dir;
    TaskStore store{dir.path()};
    TaskRegistry registry;
    (void)registry.submit("key-1", "work", 1'000);
    require(store.write(registry, "owner", "v1"), "the first write should be accepted");
    // A different record under the same version would silently destroy
    // the checkpoint that was published there.
    TaskRegistry other;
    (void)other.submit("key-2", "different work", 2'000);
    TaskError error{TaskError::none};
    require(!store.write(other, "owner", "v1", &error),
            "a published version must not be overwritten");
    require_equal(error, TaskError::conflicting_version, "the refusal must be reported");
    const auto restored = store.read("owner", "v1");
    require(restored.has_value(), "the original record should still be readable");
    require(restored->size() == 1, "the original record should still be there");
}

void test_the_store_refuses_an_unsafe_owner() {
    TempDir dir;
    TaskStore store{dir.path()};
    TaskRegistry registry;
    (void)registry.submit("key-1", "work", 1'000);
    TaskError error{TaskError::none};
    require(!store.write(registry, "../escape", "v1", &error),
            "an unsafe owner must be refused");
    require_equal(error, TaskError::invalid_task, "the refusal must be reported");
}

void test_the_store_refuses_an_unknown_version() {
    TempDir dir;
    TaskStore store{dir.path()};
    TaskError error{TaskError::none};
    require(!store.read("owner", "v9", &error).has_value(),
            "an unknown version must not be read");
    require_equal(error, TaskError::io_error, "the refusal must be reported");
}

// ---------------------------------------------------------------------------
// The whole crash, end to end.
// ---------------------------------------------------------------------------

void test_a_crash_between_the_checkpoint_and_the_completion_loses_nothing() {
    // The worker checkpoints, the process dies, a new process restores the
    // registry and resumes. The work is not done twice, and it is not
    // lost.
    TaskRegistry first;
    const auto id = *first.submit("work-key", "the work", 1'000);
    require(first.lease(id, "worker-a", 1'000, 30'000), "the lease should be accepted");
    require(first.checkpoint(id, "worker-a", "step 3 of 7", 2'000), "the checkpoint should be accepted");

    // The process dies here. The registry is written and read back.
    auto restored = TaskRegistry::deserialize(first.serialize());
    require(restored.has_value(), "the registry should be restored");

    // The lease has expired by now, and the task can be resumed.
    require(restored->resume(id, "worker-b", 100'000, 30'000),
            "the task should be resumable after the crash");
    const auto* task = restored->find(id);
    require(task != nullptr, "the task should be found");
    require(task->checkpoint == "step 3 of 7", "the work should continue where it stopped");
    require(task->attempt == 2, "the resumed attempt should be counted");

    // And submitting the same key again does not do the work again.
    const auto again = restored->submit("work-key", "the work", 100'000);
    require(again.has_value() && *again == id,
            "the same key must return the same task after the crash");
}

} // namespace

/**
 * The suite, wrapped so that a failing check prints what failed and
 * returns a non-zero exit code. The Microsoft C++ runtime shows a modal
 * dialog on an unhandled exception, and a modal dialog waits for a key
 * press that never comes.
 */
static int run_suite() {
test_a_task_is_submitted_and_is_pending();
    test_the_same_key_returns_the_same_task();
    test_a_different_key_creates_a_different_task();
    test_a_malformed_submission_is_refused();
    test_a_task_can_be_leased();
    test_a_second_worker_cannot_lease_a_live_task();
    test_a_holder_may_renew_its_lease();
    test_an_expired_lease_may_be_taken();
    test_leasing_an_unknown_task_is_refused();
    test_a_checkpoint_is_recorded();
    test_a_worker_that_does_not_hold_the_lease_cannot_checkpoint();
    test_a_worker_whose_lease_expired_cannot_checkpoint();
    test_a_task_completes();
    test_a_completed_task_cannot_be_leased();
    test_a_failed_task_may_be_retried();
    test_a_task_that_fails_too_often_is_abandoned();
    test_a_dead_workers_task_is_recoverable();
    test_a_pending_task_is_recoverable();
    test_a_finished_task_is_not_recoverable();
    test_resuming_keeps_the_checkpoint();
    test_resuming_a_live_task_is_refused();
    test_resuming_a_finished_task_is_refused();
    test_the_registry_survives_a_round_trip();
    test_a_corrupt_record_is_refused();
    test_a_truncated_record_is_refused();
    test_the_store_writes_and_reads();
    test_writing_the_same_record_twice_is_idempotent();
    test_a_published_version_cannot_be_overwritten();
    test_the_store_refuses_an_unsafe_owner();
    test_the_store_refuses_an_unknown_version();
    test_a_crash_between_the_checkpoint_and_the_completion_loses_nothing();

    return 0;
}

int main() {
    return genesis::testing::run(run_suite, "task_tests");
}