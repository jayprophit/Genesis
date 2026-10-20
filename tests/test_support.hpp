#pragma once

/**
 * The shared test harness.
 *
 * A failing test must report itself and stop. It must not stop the
 * machine.
 *
 * On this host the Microsoft C++ runtime shows a modal dialog when a
 * program terminates with an unhandled exception, and a modal dialog
 * waits for a key that nobody is going to press. It blocked the
 * automation and it had to be dealt with: a test that fails now prints
 * what failed and returns a non-zero exit code, and the run continues.
 *
 * The checks themselves are the same shape as the ones the existing
 * suites use, so a new test reads like the tests beside it.
 */

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace genesis::testing {

/// The number of checks a run made.
inline int g_checks = 0;

/// A check that must hold.
inline void require(bool condition, const char* message) {
    ++g_checks;
    if (!condition) {
        throw std::runtime_error(message);
    }
}

/// A check that two values are equal, reporting both when they are not.
template <typename A, typename B>
inline void require_equal(const A& actual, const B& expected, const char* message) {
    ++g_checks;
    if (!(actual == expected)) {
        std::cerr << "expected " << expected << " but got " << actual << ": "
                  << message << "\n";
        throw std::runtime_error(message);
    }
}

/**
 * Run a suite and report the outcome.
 *
 * The suite is a callable that throws when a check fails. This catches
 * the failure, prints it, and returns a non-zero exit code, so that a
 * failure is a line of output rather than a dialog that waits.
 */
template <typename Suite>
int run(Suite suite, const char* name) {
    try {
        suite();
    } catch (const std::exception& exception) {
        std::cerr << name << " FAILED: " << exception.what() << "\n";
        std::cerr.flush();
        return 1;
    } catch (...) {
        std::cerr << name << " FAILED: an unknown exception\n";
        std::cerr.flush();
        return 1;
    }
    std::cout << name << ": " << g_checks << " checks passed\n";
    std::cout.flush();
    return 0;
}

} // namespace genesis::testing
