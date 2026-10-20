#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace genesis::agents {

/**
 * The durable task registry: work that survives a restart.
 *
 * The gap analysis says what was missing, and it is worth reading in the
 * gap's own words: "A run exists only as an in-memory record. There is
 * no durable task registry, no checkpoint, no lease and no resume, so a
 * job cannot survive a restart or a crash." And: "A crash loses the run.
 * There is no checkpoint, no idempotency key and no recovery, so a
 * remote worker that dies leaves work half done with no way to resume
 * it."
 *
 * This module is that missing piece, and it answers four questions:
 *
 *  1. WAS THIS WORK ALREADY DONE? Every task carries an idempotency
 *     key. Submitting the same key twice returns the same task and does
 *     the work once. A retry after a network failure cannot double-spend
 *     a side effect.
 *
 *  2. WHO IS DOING IT? A worker holds a lease for a bounded period. A
 *     lease that expires releases the task, so a worker that dies does
 *     not hold work forever, and a second worker cannot take a task
 *     that is still live.
 *
 *  3. HOW FAR DID IT GET? A checkpoint records the worker's progress.
 *     The checkpoint is opaque to the registry, which stores and returns
 *     it and does not pretend to understand it.
 *
 *  4. CAN IT BE PICKED UP AGAIN? A task whose lease has expired is
 *     recoverable. Resuming it takes a fresh lease and keeps the
 *     checkpoint, so the work continues from where it stopped rather
 *     than from the beginning.
 *
 * The registry is honest about failure. A task that has failed too many
 * times is abandoned rather than retried forever, and the reason is
 * kept.
 *
 * Nothing here executes anything. The registry records what is owed and
 * who holds it; the workers do the work.
 */

/// Where a durable task is in its life.
enum class TaskState {
    /// Submitted, and no worker holds it.
    pending,
    /// A worker holds the lease and has not reported progress.
    leased,
    /// A worker is working on it.
    running,
    /// Progress was recorded, and the task can be resumed from it.
    checkpointed,
    /// Finished successfully.
    completed,
    /// Finished unsuccessfully, and may be retried.
    failed,
    /// Failed too many times, and is not to be retried.
    abandoned,
};

[[nodiscard]] std::string_view to_string(TaskState state) noexcept;

/// Why an operation on the task registry was refused.
enum class TaskError {
    /// The operation was accepted.
    none,
    /// The request was malformed.
    invalid_task,
    /// There is no such task.
    unknown_task,
    /// The task cannot be leased: it is finished or abandoned.
    not_leasable,
    /// Another worker holds a live lease on the task.
    lease_held,
    /// The worker does not hold the lease.
    not_lease_holder,
    /// The lease has expired.
    lease_expired,
    /// The task cannot be resumed.
    not_resumable,
    /// The task has failed as many times as it is allowed to.
    too_many_attempts,
    /// The stored record is corrupt and was refused.
    corrupt_record,
    /// The version already holds a different record. A published
    /// checkpoint cannot be silently overwritten.
    conflicting_version,
    /// The store could not be read or written.
    io_error,
};

[[nodiscard]] std::string_view to_string(TaskError error) noexcept;

/// A task that survives a restart.
struct DurableTask final {
    /// The task's own identifier.
    std::string task_id;
    /// The key that makes submitting this work idempotent.
    std::string idempotency_key;
    /// What the work is.
    std::string title;
    /// The worker that holds the lease.
    std::string worker_id;
    /// When the lease ends. Zero means there is no lease.
    std::uint64_t lease_expires_at{0};
    /// Where the task is in its life.
    TaskState state{TaskState::pending};
    /// How many times the task has been leased.
    std::uint32_t attempt{0};
    /// When the task was submitted.
    std::uint64_t created_at{0};
    /// When the task last changed.
    std::uint64_t updated_at{0};
    /// The last checkpoint. Opaque to the registry.
    std::string checkpoint;
    /// The reason the last attempt failed.
    std::string last_error;
};

/**
 * The registry of durable tasks.
 *
 * The registry is the single source of truth about what work is owed.
 * It holds no side effects and performs no work; it decides who may
 * work on what, and for how long.
 */
class TaskRegistry final {
public:
    /**
     * Submit a task.
     *
     * The idempotency key makes the submission idempotent. Submitting
     * the same key again returns the task that already exists, whether
     * that task is still pending or already finished, and the work is
     * not done twice.
     */
    [[nodiscard]] std::optional<std::string> submit(const std::string& idempotency_key,
                                                      const std::string& title,
                                                      std::uint64_t now,
                                                      TaskError* error = nullptr);

    /**
     * Take a lease on a task for a bounded period.
     *
     * The lease succeeds when the task is pending, when it has no live
     * lease, or when the previous holder's lease has expired. It fails
     * when another worker holds a live lease, because two workers doing
     * the same task is the failure this exists to prevent.
     */
    [[nodiscard]] bool lease(const std::string& task_id,
                             const std::string& worker_id,
                             std::uint64_t now,
                             std::uint64_t lease_milliseconds,
                             TaskError* error = nullptr);

    /**
     * Record progress against a task.
     *
     * The worker must hold a live lease. The body is opaque: the
     * registry stores it and hands it back, and does not pretend to
     * understand it.
     */
    [[nodiscard]] bool checkpoint(const std::string& task_id,
                                 const std::string& worker_id,
                                 std::string_view body,
                                 std::uint64_t now,
                                 TaskError* error = nullptr);

    /// Finish successfully. The worker must hold a live lease.
    [[nodiscard]] bool complete(const std::string& task_id,
                                const std::string& worker_id,
                                std::uint64_t now,
                                TaskError* error = nullptr);

    /**
     * Finish unsuccessfully.
     *
     * The task returns to pending and may be retried, unless it has
     * already been attempted as many times as it is allowed, in which
     * case it is abandoned and is not to be retried.
     */
    [[nodiscard]] bool fail(const std::string& task_id,
                            const std::string& worker_id,
                            std::string_view reason,
                            std::uint64_t now,
                            std::uint32_t maximum_attempts,
                            TaskError* error = nullptr);

    /**
     * The tasks that can be picked up.
     *
     * That is: the pending ones, and the ones whose worker has died,
     * which is the crash case. A finished task is not recoverable.
     */
    [[nodiscard]] std::vector<DurableTask> recoverable(std::uint64_t now) const;

    /**
     * Take over a task whose worker has died, and resume it.
     *
     * The checkpoint is kept, so the work continues from where it
     * stopped. A task with no checkpoint restarts, and the attempt count
     * says so.
     */
    [[nodiscard]] bool resume(const std::string& task_id,
                              const std::string& worker_id,
                              std::uint64_t now,
                              std::uint64_t lease_milliseconds,
                              TaskError* error = nullptr);

    /// The task, or null.
    [[nodiscard]] const DurableTask* find(const std::string& task_id) const noexcept;

    /// Every task, in submission order.
    [[nodiscard]] std::vector<DurableTask> tasks() const;

    /// How many tasks are held.
    [[nodiscard]] std::size_t size() const noexcept;

    /// Serialise for durable storage. The bytes carry a checksum.
    [[nodiscard]] std::string serialize() const;

    /**
     * Restore from durable storage.
     *
     * A record whose checksum does not match is refused, and so is one
     * that does not parse. A store that cannot be read must not be
     * mistaken for an empty store: an empty registry would silently
     * forget every task that was owed.
     */
    [[nodiscard]] static std::optional<TaskRegistry> deserialize(std::string_view bytes,
                                                                 TaskError* error = nullptr);

private:
    /// The tasks, in submission order.
    std::vector<DurableTask> tasks_;
    /// task id -> index into tasks_
    std::map<std::string, std::size_t, std::less<>> index_;
    /// idempotency key -> task id
    std::map<std::string, std::string, std::less<>> by_key_;
    /// The next task number.
    std::uint64_t next_sequence_{1};
};

/**
 * The durable store of the task registry.
 *
 * The store publishes an immutable, versioned, checksummed record. A
 * write that would overwrite an existing version is refused, so a
 * checkpoint cannot be lost to an accidental second write.
 */
class TaskStore final {
public:
    explicit TaskStore(std::filesystem::path root,
                       std::size_t maximum_record_bytes = 64U * 1024U * 1024U);

    /// Write the registry under an owner and version.
    [[nodiscard]] bool write(const TaskRegistry& registry,
                             std::string_view owner_id,
                             std::string_view version,
                             TaskError* error = nullptr) const;

    /// Read the registry published under an owner and version.
    [[nodiscard]] std::optional<TaskRegistry> read(std::string_view owner_id,
                                                   std::string_view version,
                                                   TaskError* error = nullptr) const;

private:
    std::filesystem::path root_;
    std::size_t maximum_record_bytes_{};
};

} // namespace genesis::agents
