#include "genesis/common/immutable_snapshot.hpp"

#include "genesis/runtime/runtime.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace genesis::storage {
namespace {

class RecordTooLarge final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class UnsafeFileType final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void set_error(ImmutableFileError* error,
               ImmutableFileErrorCode code,
               std::string message) {
    if (error != nullptr) {
        error->code = code;
        error->message = std::move(message);
    }
}

void validate_identifier(std::string_view value, std::size_t maximum) {
    if (value.empty() || value.size() > maximum || value == "." || value == "..") {
        throw std::invalid_argument("unsafe or empty immutable-file identifier");
    }
    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (std::isalnum(byte) == 0 && byte != '_' && byte != '-' && byte != '.') {
            throw std::invalid_argument("unsafe immutable-file identifier character");
        }
    }
}

std::optional<std::filesystem::file_status> inspect_existing(
    const std::filesystem::path& path) {
    std::error_code error;
    const auto status = std::filesystem::symlink_status(path, error);
    if (error == std::errc::no_such_file_or_directory
        || status.type() == std::filesystem::file_type::not_found) {
        return std::nullopt;
    }
    if (error) {
        throw std::runtime_error("cannot inspect immutable snapshot record");
    }
    return status;
}

std::string read_regular_bounded(const std::filesystem::path& path,
                                 std::size_t maximum_record_bytes) {
    const auto status = inspect_existing(path);
    if (!status.has_value()) {
        throw std::runtime_error("immutable snapshot record disappeared during read");
    }
    if (std::filesystem::is_symlink(*status)
        || !std::filesystem::is_regular_file(*status)) {
        throw UnsafeFileType("immutable snapshot target is not a regular non-link file");
    }

    std::error_code error;
    const auto file_size = std::filesystem::file_size(path, error);
    if (error) {
        throw std::runtime_error("cannot determine immutable snapshot size");
    }
    if (file_size > maximum_record_bytes
        || file_size
               > static_cast<std::uintmax_t>(
                   std::numeric_limits<std::streamsize>::max())) {
        throw RecordTooLarge("immutable snapshot exceeds the configured read limit");
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open immutable snapshot record");
    }
    std::string bytes(static_cast<std::size_t>(file_size), '\0');
    input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (input.bad() || input.gcount() != static_cast<std::streamsize>(bytes.size())) {
        throw std::runtime_error("cannot read complete immutable snapshot record");
    }
    char unexpected{};
    if (input.get(unexpected)) {
        throw RecordTooLarge("immutable snapshot grew during its bounded read");
    }
    return bytes;
}

void durable_flush_file(const std::filesystem::path& path) {
#ifdef _WIN32
    const auto handle = CreateFileW(path.c_str(),
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    nullptr,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("cannot open immutable snapshot for durable flush");
    }
    const auto flushed = FlushFileBuffers(handle) != 0;
    CloseHandle(handle);
    if (!flushed) {
        throw std::runtime_error("cannot durably flush immutable snapshot");
    }
#else
    const auto descriptor = ::open(path.c_str(), O_RDONLY);
    if (descriptor < 0) {
        throw std::runtime_error("cannot open immutable snapshot for durable flush");
    }
    const auto flush_result = ::fsync(descriptor);
    const auto close_result = ::close(descriptor);
    if (flush_result != 0 || close_result != 0) {
        throw std::runtime_error("cannot durably flush immutable snapshot");
    }
#endif
}

bool publish_without_replacement(const std::filesystem::path& temporary,
                                 const std::filesystem::path& target) {
#ifdef _WIN32
    return MoveFileExW(temporary.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH) != 0;
#else
    if (::link(temporary.c_str(), target.c_str()) != 0) {
        return false;
    }
    if (::unlink(temporary.c_str()) != 0) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
    }
    auto directory_flags = O_RDONLY;
#ifdef O_DIRECTORY
    directory_flags |= O_DIRECTORY;
#endif
    const auto directory = ::open(target.parent_path().c_str(), directory_flags);
    if (directory >= 0) {
        static_cast<void>(::fsync(directory));
        static_cast<void>(::close(directory));
    }
    return true;
#endif
}

std::filesystem::path temporary_path_for(const std::filesystem::path& target) {
    static std::atomic<std::uint64_t> counter{0U};
    const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto sequence = counter.fetch_add(1U, std::memory_order_relaxed);
    const auto process_id =
#ifdef _WIN32
        static_cast<std::uint64_t>(GetCurrentProcessId());
#else
        static_cast<std::uint64_t>(::getpid());
#endif
    const auto token = runtime::sha256(std::to_string(process_id) + ":"
                                       + std::to_string(tick) + ":"
                                       + std::to_string(sequence));
    return std::filesystem::path(target.string() + ".tmp-" + token.substr(0U, 20U));
}

ImmutableFileErrorCode classify_exception(const std::exception& exception) {
    if (dynamic_cast<const RecordTooLarge*>(&exception) != nullptr) {
        return ImmutableFileErrorCode::record_too_large;
    }
    if (dynamic_cast<const UnsafeFileType*>(&exception) != nullptr) {
        return ImmutableFileErrorCode::unsafe_file_type;
    }
    if (dynamic_cast<const std::invalid_argument*>(&exception) != nullptr) {
        return ImmutableFileErrorCode::invalid_identifier;
    }
    return ImmutableFileErrorCode::io_error;
}

} // namespace

ImmutableSnapshotFiles::ImmutableSnapshotFiles(std::filesystem::path root,
                                               std::size_t maximum_record_bytes)
    : root_(std::move(root)), maximum_record_bytes_(maximum_record_bytes) {
    if (root_.empty() || maximum_record_bytes_ < 1024U
        || maximum_record_bytes_
               > static_cast<std::size_t>(
                   std::numeric_limits<std::streamsize>::max())) {
        throw std::invalid_argument("invalid immutable snapshot store configuration");
    }
}

std::filesystem::path ImmutableSnapshotFiles::record_path(std::string_view owner_id,
                                                          std::string_view version,
                                                          std::string_view suffix) const {
    validate_identifier(owner_id, 128U);
    validate_identifier(version, 128U);
    validate_identifier(suffix, 32U);
    return root_ / (std::string(owner_id) + "." + std::string(version) + "."
                    + std::string(suffix));
}

bool ImmutableSnapshotFiles::write(std::string_view owner_id,
                                   std::string_view version,
                                   std::string_view suffix,
                                   std::string_view bytes,
                                   ImmutableFileError* error) const {
    std::filesystem::path temporary;
    try {
        const auto target = record_path(owner_id, version, suffix);
        if (bytes.size() > maximum_record_bytes_) {
            throw RecordTooLarge("immutable snapshot exceeds the configured write limit");
        }

        std::error_code filesystem_error;
        std::filesystem::create_directories(root_, filesystem_error);
        if (filesystem_error || !std::filesystem::is_directory(root_)) {
            throw std::runtime_error("cannot create immutable snapshot directory");
        }

        if (inspect_existing(target).has_value()) {
            if (read_regular_bounded(target, maximum_record_bytes_) == bytes) {
                set_error(error, ImmutableFileErrorCode::none, {});
                return true;
            }
            set_error(error,
                      ImmutableFileErrorCode::conflicting_version,
                      "immutable snapshot version already contains different bytes");
            return false;
        }

        temporary = temporary_path_for(target);
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output) {
                throw std::runtime_error("cannot create temporary immutable snapshot");
            }
            output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            output.flush();
            if (!output) {
                throw std::runtime_error("cannot flush temporary immutable snapshot");
            }
        }
        durable_flush_file(temporary);

        if (!publish_without_replacement(temporary, target)) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            temporary.clear();
            if (inspect_existing(target).has_value()) {
                if (read_regular_bounded(target, maximum_record_bytes_) == bytes) {
                    set_error(error, ImmutableFileErrorCode::none, {});
                    return true;
                }
                set_error(error,
                          ImmutableFileErrorCode::conflicting_version,
                          "a concurrent writer published different immutable bytes");
                return false;
            }
            throw std::runtime_error("cannot atomically publish immutable snapshot");
        }
        temporary.clear();
        set_error(error, ImmutableFileErrorCode::none, {});
        return true;
    } catch (const std::exception& exception) {
        if (!temporary.empty()) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
        }
        set_error(error, classify_exception(exception), exception.what());
        return false;
    }
}

std::optional<std::string> ImmutableSnapshotFiles::read(
    std::string_view owner_id,
    std::string_view version,
    std::string_view suffix,
    ImmutableFileError* error) const {
    try {
        const auto target = record_path(owner_id, version, suffix);
        if (!inspect_existing(target).has_value()) {
            set_error(error,
                      ImmutableFileErrorCode::not_found,
                      "immutable snapshot version was not found");
            return std::nullopt;
        }
        auto bytes = read_regular_bounded(target, maximum_record_bytes_);
        set_error(error, ImmutableFileErrorCode::none, {});
        return bytes;
    } catch (const std::exception& exception) {
        set_error(error, classify_exception(exception), exception.what());
        return std::nullopt;
    }
}

const std::filesystem::path& ImmutableSnapshotFiles::root() const noexcept {
    return root_;
}

std::size_t ImmutableSnapshotFiles::maximum_record_bytes() const noexcept {
    return maximum_record_bytes_;
}

} // namespace genesis::storage
