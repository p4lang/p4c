/*
 * SPDX-FileCopyrightText: 2023 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_P4TOOLS_COMMON_LIB_LOGGING_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_LOGGING_H_

#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include "lib/format.h"
#include "lib/log.h"

namespace P4::P4Tools {

namespace detail {

// Logging uses operator<<, including overloads for trace events and pointers.
// Keep that behavior instead of the diagnostic helpers' toString/dbprint dispatch.
template <typename T>
struct LogArgument {
    const T &value;
    friend std::ostream &operator<<(std::ostream &out, const LogArgument &arg) {
        return out << arg.value;
    }
};

template <typename T>
decltype(auto) logArgument(const T &arg) {
    if constexpr (std::is_arithmetic_v<T>) {
        return arg;
    } else {
        return LogArgument<T>{arg};
    }
}

}  // namespace detail

/// A helper function that allows us to configure logging for a particular feature. This code is
/// taken from
// https://stackoverflow.com/a/25859856
template <typename... Arguments>
void printFeature(const std::string &label, int level, const std::string &fmt,
                  Arguments &&...args) {
    // Do not print logging messages when logging is not enabled.
    if (!Log::fileLogLevelIsAtLeast(label.c_str(), level)) {
        return;
    }

    LOG_FEATURE(label.c_str(), level,
                createFormattedMessage(fmt.c_str(), detail::logArgument(args)...));
}

/// Helper functions that prints strings associated with basic tool information.
/// For example, the seed, status of test case generation, etc.
template <typename... Arguments>
void printInfo(const std::string &fmt, Arguments &&...args) {
    printFeature("tools_info", 4, fmt, std::forward<Arguments>(args)...);
}

/// Convenience function for printing debug information.
/// Easier to use then LOG(XX) since we only need one specific log-level across all files.
/// Can be invoked with "-T tools_debug:4".
template <typename... Arguments>
void printDebug(const std::string &fmt, Arguments &&...args) {
    printFeature("tools_debug", 4, fmt, std::forward<Arguments>(args)...);
}

/// Enable the printing of basic run information.
void enableInformationLogging();

/// Enable printing of timing reports.
void enablePerformanceLogging();

/// Print a performance report if performance logging is enabled.
/// If a file is provided, it will be written to the file.
void printPerformanceReport(const std::optional<std::filesystem::path> &basePath = std::nullopt);

}  // namespace P4::P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_LOGGING_H_ */
