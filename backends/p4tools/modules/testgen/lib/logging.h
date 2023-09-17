#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_LOGGING_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_LOGGING_H_

#include <string>
#include <utility>

#include "backends/p4tools/common/lib/util.h"

namespace P4Tools::P4Testgen {

/// Helper functions that prints strings associated with verbose test information, for example
/// traces, or the test values themselves.
template <typename... Arguments>
void printTraces(const std::string &fmt, Arguments &&...args) {
    printFeature("test_traces", 4, fmt, std::forward<Arguments>(args)...);
}

/// Helper functions that prints strings associated with basic test generation information., for
/// example the covered nodes or tests number.
template <typename... Arguments>
void printInfo(const std::string &fmt, Arguments &&...args) {
    printFeature("test_info", 4, fmt, std::forward<Arguments>(args)...);
}

/// Enable trace printing.
void enableTraceLogging();

/// Enable the printing of basic test case generation information, for example the covered
/// nodes or test number.
void enableInformationLogging();

/// Enable printing of the individual program node steps of the interpreter.
void enableStepLogging();

/// Enable printing of the statement coverage statistics the interpreter collects.
void enableCoverageLogging();

/// Enable printing of timing reports.
void enablePerformanceLogging();

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_LOGGING_H_ */
