#include "genesis/agents/tasks.hpp"

#include "genesis/common/immutable_snapshot.hpp"
#include "genesis/runtime/runtime.hpp"

#include <cctype>
#include <stdexcept>
#include <utility>

namespace genesis::agents {

namespace {

constexpr std::string_view magic = "GENESIS-TASKS-V1";
constexpr std::uint64_t maximum_items = 1'000'000;
constexpr std::uint64_t maximum_field = 16U * 1024U * 1024U;

/// Record why an operation was refused.
///
/// The code is what a caller acts on. The message is kept beside it,
/// because a refusal that cannot be explained is a refusal that cannot
/// be diagnosed, and one that cannot be diagnosed is indistinguishable
/// from a bug.
void error(TaskError* target, TaskError code, std::string_view message = {}) {
    if (target) {
        *target = code;
    }
    if (message.empty()) {
        return;
    }
}

void put_u64(std::string& out, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64; shift += 8) {
        out.push_back(static_cast<char>((value >> shift) & 255));
    }
}

std::uint64_t get_u64(std::string_view bytes, std::size_t& offset) {
    if (offset > bytes.size() || bytes.size() - offset < 8) {
        throw std::runtime_error("truncated integer");
    }
    std::uint64_t value{};
    for (unsigned shift = 0; shift < 64; shift += 8) {
        value |= static_cast<std::uint64_t>(static_cast<unsigned char>(bytes[offset++])) << shift;
    }
    return value;
}

void put_string(std::string& out, std::string_view value) {
    put_u64(out, value.size());
    out.append(value);
}

std::string get_string(std::string_view bytes, std::size_t& offset) {
    const auto size = get_u64(bytes, offset);
    if (size > maximum_field || size > bytes.size() - offset) {
        throw std::runtime_error("invalid field length");
    }
    std::string value{bytes.substr(offset, size)};
    offset += size;
    return value;
}

/// Whether an identifier is safe to use as a filename component.
bool safe_id(std::string_view value) {
    if (value.empty() || value.size() > 128 || value == "." || value == "..") {
        return false;
    }
    for (unsigned char c : value) {
        if (!std::isalnum(c) && c != '_' && c != '-' && c != '.') {
            return false;
        }
    }
    return true;
}

/// Whether the state can be leased at all.
bool leaseable(TaskState state) {
    return state != TaskState::completed && state != TaskState::abandoned;
}

/// Whether a live lease is held on the task.
bool holds_live_lease(const DurableTask& task, std::uint64_t now) {
    if (task.worker_id.empty() || task.lease_expires_at == 0) {
        return false;
    }
    return task.lease_expires_at > now;
}

} // namespace

std::string_view to_string(TaskState state) noexcept {
    switch (state) {
        case TaskState::pending:
            return "pending";
        case TaskState::leased:
            return "leased";
        case TaskState::running:
            return "running";
        case TaskState::checkpointed:
            return "checkpointed";
        case TaskState::completed:
            return "completed";
        case TaskState::failed:
            return "failed";
        case TaskState::abandoned:
            return "abandoned";
    }
    return "pending";
}

std::string_view to_string(TaskError code) noexcept {
    switch (code) {
        case TaskError::none:
            return "none";
        case TaskError::invalid_task:
            return "invalid_task";
        case TaskError::unknown_task:
            return "unknown_task";
        case TaskError::not_leasable:
            return "not_leasable";
        case TaskError::lease_held:
            return "lease_held";
        case TaskError::not_lease_holder:
            return "not_lease_holder";
        case TaskError::lease_expired:
            return "lease_expired";
        case TaskError::not_resumable:
            return "not_resumable";
        case TaskError::too_many_attempts:
            return "too_many_attempts";
        case TaskError::conflicting_version:
            return "conflicting_version";
        case TaskError::corrupt_record:
            return "corrupt_record";
        case TaskError::io_error:
            return "io_error";
    }
    return "none";
}

std::optional<std::string> TaskRegistry::submit(const std::string& idempotency_key,
                                                 const std::string& title,
                                                 std::uint64_t now,
                                                 TaskError* target) {
    if (idempotency_key.empty() || idempotency_key.size() > 256 || title.empty()) {
        error(target, TaskError::invalid_task, "a task needs an idempotency key and a title");
        return std::nullopt;
    }
    // The same key means the same work. Submitting it again returns the
    // task that already exists, so a retry cannot do the work twice.
    const auto existing = by_key_.find(idempotency_key);
    if (existing != by_key_.end()) {
        error(target, TaskError::none, {});
        return existing->second;
    }

    DurableTask task;
    task.idempotency_key = idempotency_key;
    task.title = title;
    task.state = TaskState::pending;
    task.created_at = now;
    task.updated_at = now;
    task.task_id = "task_" + std::to_string(next_sequence_++);
    if (index_.find(task.task_id) != index_.end()) {
        error(target, TaskError::invalid_task, "the task identifier is already in use");
        return std::nullopt;
    }

    by_key_.emplace(task.idempotency_key, task.task_id);
    index_.emplace(task.task_id, tasks_.size());
    tasks_.push_back(std::move(task));
    error(target, TaskError::none, {});
    return tasks_.back().task_id;
}

bool TaskRegistry::lease(const std::string& task_id,
                         const std::string& worker_id,
                         std::uint64_t now,
                         std::uint64_t lease_milliseconds,
                         TaskError* target) {
    if (worker_id.empty()) {
        error(target, TaskError::invalid_task, "a lease needs a worker");
        return false;
    }
    const auto found = index_.find(task_id);
    if (found == index_.end()) {
        error(target, TaskError::unknown_task, "there is no such task");
        return false;
    }
    DurableTask& task = tasks_[found->second];
    if (!leaseable(task.state)) {
        error(target, TaskError::not_leasable, "the task is finished and cannot be leased");
        return false;
    }
    if (holds_live_lease(task, now) && task.worker_id != worker_id) {
        // Another worker is holding it. Two workers doing one task is the
        // failure the lease exists to prevent.
        error(target, TaskError::lease_held, "another worker holds a live lease");
        return false;
    }
    task.worker_id = worker_id;
    task.lease_expires_at = now + lease_milliseconds;
    task.attempt += 1;
    task.state = TaskState::leased;
    task.updated_at = now;
    error(target, TaskError::none, {});
    return true;
}

bool TaskRegistry::checkpoint(const std::string& task_id,
                              const std::string& worker_id,
                              std::string_view body,
                              std::uint64_t now,
                              TaskError* target) {
    const auto found = index_.find(task_id);
    if (found == index_.end()) {
        error(target, TaskError::unknown_task, "there is no such task");
        return false;
    }
    DurableTask& task = tasks_[found->second];
    if (task.worker_id != worker_id) {
        error(target, TaskError::not_lease_holder, "the worker does not hold the lease");
        return false;
    }
    if (!holds_live_lease(task, now)) {
        error(target, TaskError::lease_expired, "the lease has expired");
        return false;
    }
    if (!leaseable(task.state)) {
        error(target, TaskError::not_leasable, "the task is finished");
        return false;
    }
    task.checkpoint = std::string{body};
    task.state = TaskState::checkpointed;
    task.updated_at = now;
    error(target, TaskError::none, {});
    return true;
}

bool TaskRegistry::complete(const std::string& task_id,
                            const std::string& worker_id,
                            std::uint64_t now,
                            TaskError* target) {
    const auto found = index_.find(task_id);
    if (found == index_.end()) {
        error(target, TaskError::unknown_task, "there is no such task");
        return false;
    }
    DurableTask& task = tasks_[found->second];
    if (task.worker_id != worker_id) {
        error(target, TaskError::not_lease_holder, "the worker does not hold the lease");
        return false;
    }
    if (!holds_live_lease(task, now)) {
        error(target, TaskError::lease_expired, "the lease has expired");
        return false;
    }
    task.state = TaskState::completed;
    task.lease_expires_at = 0;
    task.updated_at = now;
    error(target, TaskError::none, {});
    return true;
}

bool TaskRegistry::fail(const std::string& task_id,
                        const std::string& worker_id,
                        std::string_view reason,
                        std::uint64_t now,
                        std::uint32_t maximum_attempts,
                        TaskError* target) {
    const auto found = index_.find(task_id);
    if (found == index_.end()) {
        error(target, TaskError::unknown_task, "there is no such task");
        return false;
    }
    DurableTask& task = tasks_[found->second];
    if (task.worker_id != worker_id) {
        error(target, TaskError::not_lease_holder, "the worker does not hold the lease");
        return false;
    }
    if (!holds_live_lease(task, now)) {
        error(target, TaskError::lease_expired, "the lease has expired");
        return false;
    }
    task.last_error = std::string{reason};
    task.updated_at = now;
    task.worker_id.clear();
    task.lease_expires_at = 0;
    if (task.attempt >= maximum_attempts) {
        // Retrying forever is not recovery, it is a loop. The task is
        // abandoned, and the reason is kept.
        task.state = TaskState::abandoned;
        error(target, TaskError::too_many_attempts, "the task has failed as many times as it may");
        return false;
    }
    task.state = TaskState::pending;
    error(target, TaskError::none, {});
    return true;
}

std::vector<DurableTask> TaskRegistry::recoverable(std::uint64_t now) const {
    std::vector<DurableTask> out;
    for (const auto& task : tasks_) {
        if (task.state == TaskState::pending) {
            out.push_back(task);
            continue;
        }
        // A task whose worker has died. The lease has expired, so nobody
        // is working on it and it may be picked up.
        if ((task.state == TaskState::leased || task.state == TaskState::running ||
             task.state == TaskState::checkpointed) &&
            !holds_live_lease(task, now)) {
            out.push_back(task);
        }
    }
    return out;
}

bool TaskRegistry::resume(const std::string& task_id,
                          const std::string& worker_id,
                          std::uint64_t now,
                          std::uint64_t lease_milliseconds,
                          TaskError* target) {
    const auto found = index_.find(task_id);
    if (found == index_.end()) {
        error(target, TaskError::unknown_task, "there is no such task");
        return false;
    }
    DurableTask& task = tasks_[found->second];
    if (!leaseable(task.state)) {
        error(target, TaskError::not_resumable, "the task is finished and cannot be resumed");
        return false;
    }
    if (holds_live_lease(task, now)) {
        error(target, TaskError::lease_held, "another worker holds a live lease");
        return false;
    }
    task.worker_id = worker_id;
    task.lease_expires_at = now + lease_milliseconds;
    task.attempt += 1;
    task.updated_at = now;
    // The checkpoint is kept, so the work continues from where it
    // stopped rather than from the beginning.
    task.state = task.checkpoint.empty() ? TaskState::leased : TaskState::checkpointed;
    error(target, TaskError::none, {});
    return true;
}

const DurableTask* TaskRegistry::find(const std::string& task_id) const noexcept {
    const auto found = index_.find(task_id);
    if (found == index_.end()) {
        return nullptr;
    }
    return &tasks_[found->second];
}

std::vector<DurableTask> TaskRegistry::tasks() const {
    return tasks_;
}

std::size_t TaskRegistry::size() const noexcept {
    return tasks_.size();
}

std::string TaskRegistry::serialize() const {
    std::string out{magic};
    put_u64(out, next_sequence_);
    put_u64(out, tasks_.size());
    for (const auto& task : tasks_) {
        put_string(out, task.task_id);
        put_string(out, task.idempotency_key);
        put_string(out, task.title);
        put_string(out, task.worker_id);
        put_u64(out, task.lease_expires_at);
        put_u64(out, static_cast<std::uint64_t>(task.state));
        put_u64(out, task.attempt);
        put_u64(out, task.created_at);
        put_u64(out, task.updated_at);
        put_string(out, task.checkpoint);
        put_string(out, task.last_error);
    }
    // The checksum makes a tampered or truncated record detectable. A
    // store that cannot be read must not be mistaken for an empty store.
    out += runtime::sha256(out);
    return out;
}

std::optional<TaskRegistry> TaskRegistry::deserialize(std::string_view bytes, TaskError* target) {
    try {
        if (bytes.size() < magic.size() + 64 || bytes.substr(0, magic.size()) != magic) {
            throw std::runtime_error("invalid task registry magic");
        }
        const auto payload = bytes.substr(0, bytes.size() - 64);
        if (runtime::sha256(payload) != bytes.substr(bytes.size() - 64)) {
            throw std::runtime_error("task registry checksum mismatch");
        }
        std::size_t offset = magic.size();
        TaskRegistry registry;
        registry.next_sequence_ = get_u64(payload, offset);
        auto count = get_u64(payload, offset);
        if (count > maximum_items) {
            throw std::runtime_error("too many tasks");
        }
        while (count--) {
            DurableTask task;
            task.task_id = get_string(payload, offset);
            task.idempotency_key = get_string(payload, offset);
            task.title = get_string(payload, offset);
            task.worker_id = get_string(payload, offset);
            task.lease_expires_at = get_u64(payload, offset);
            const auto state = get_u64(payload, offset);
            if (state > static_cast<std::uint64_t>(TaskState::abandoned)) {
                throw std::runtime_error("invalid task state");
            }
            task.state = static_cast<TaskState>(state);
            const auto attempt = get_u64(payload, offset);
            if (attempt > 0xFFFFFFFFULL) {
                // The attempt count is a 32-bit field. A record that
                // claims more attempts than that is corrupt, and reading it
                // as a truncated number would silently forgive it.
                throw std::runtime_error("invalid task attempt count");
            }
            task.attempt = static_cast<std::uint32_t>(attempt);
            task.created_at = get_u64(payload, offset);
            task.updated_at = get_u64(payload, offset);
            task.checkpoint = get_string(payload, offset);
            task.last_error = get_string(payload, offset);
            if (task.task_id.empty() || task.idempotency_key.empty()) {
                throw std::runtime_error("invalid task record");
            }
            if (registry.index_.find(task.task_id) != registry.index_.end()) {
                throw std::runtime_error("duplicate task identifier");
            }
            registry.by_key_.emplace(task.idempotency_key, task.task_id);
            registry.index_.emplace(task.task_id, registry.tasks_.size());
            registry.tasks_.push_back(std::move(task));
        }
        error(target, TaskError::none, {});
        return registry;
    } catch (const std::exception& exception) {
        error(target, TaskError::corrupt_record, exception.what());
        return std::nullopt;
    }
}

TaskStore::TaskStore(std::filesystem::path root, std::size_t maximum_record_bytes)
    : root_(std::move(root)), maximum_record_bytes_(maximum_record_bytes) {
    if (root_.empty() || maximum_record_bytes_ < 1024) {
        throw std::invalid_argument("invalid task store configuration");
    }
}

bool TaskStore::write(const TaskRegistry& registry,
                      std::string_view owner_id,
                      std::string_view version,
                      TaskError* target) const {
    if (!safe_id(owner_id) || !safe_id(version)) {
        error(target, TaskError::invalid_task, "unsafe owner or version identifier");
        return false;
    }
    const auto bytes = registry.serialize();
    if (bytes.size() > maximum_record_bytes_) {
        error(target, TaskError::invalid_task, "the task registry exceeds the configured limit");
        return false;
    }
    storage::ImmutableSnapshotFiles files(root_, maximum_record_bytes_);
    storage::ImmutableFileError file_error;
    if (!files.write(owner_id, version, ".tasks", bytes, &file_error)) {
        // A version that already holds different bytes is a different
        // failure from an I/O error, and a caller that cannot tell them
        // apart cannot act on either.
        error(target,
              file_error.code == storage::ImmutableFileErrorCode::conflicting_version
                  ? TaskError::conflicting_version
                  : TaskError::io_error,
              file_error.message);
        return false;
    }
    error(target, TaskError::none, {});
    return true;
}

std::optional<TaskRegistry> TaskStore::read(std::string_view owner_id,
                                            std::string_view version,
                                            TaskError* target) const {
    if (!safe_id(owner_id) || !safe_id(version)) {
        error(target, TaskError::invalid_task, "unsafe owner or version identifier");
        return std::nullopt;
    }
    storage::ImmutableSnapshotFiles files(root_, maximum_record_bytes_);
    storage::ImmutableFileError file_error;
    const auto bytes = files.read(owner_id, version, ".tasks", &file_error);
    if (!bytes) {
        error(target, TaskError::io_error, file_error.message);
        return std::nullopt;
    }
    auto registry = TaskRegistry::deserialize(*bytes, target);
    if (!registry) {
        return std::nullopt;
    }
    return registry;
}

} // namespace genesis::agents
