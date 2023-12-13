#ifndef BACKENDS_P4TOOLS_COMMON_LIB_LOGGING_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_LOGGING_H_

#include <filesystem>
#include <string>
#include <utility>

#include "backends/p4tools/common/lib/util.h"

namespace P4Tools {

/// Helper functions that prints strings associated with basic test generation information., for
/// example the covered nodes or tests number.
template <typename... Arguments>
void printInfo(const std::string &fmt, Arguments &&...args) {
    printFeature("test_info", 4, fmt, std::forward<Arguments>(args)...);
}

/// Enable the printing of basic run information.
void enableInformationLogging();

/// Enable printing of timing reports.
void enablePerformanceLogging();

/// Print a performance report if performance logging is enabled.
/// If a file is provided, it will be written to the file.
void printPerformanceReport(const std::optional<std::filesystem::path> &basePath = std::nullopt);

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_LOGGING_H_ */
