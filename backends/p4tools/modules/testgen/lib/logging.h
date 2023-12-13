#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_LOGGING_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_LOGGING_H_

#include <string>
#include <utility>

#include "backends/p4tools/common/lib/logging.h"  // IWYU pragma: export
#include "backends/p4tools/common/lib/util.h"

namespace P4Tools::P4Testgen {

/// Helper functions that prints strings associated with verbose test information, for example
/// traces, or the test values themselves.
template <typename... Arguments>
void printTraces(const std::string &fmt, Arguments &&...args) {
    printFeature("test_traces", 4, fmt, std::forward<Arguments>(args)...);
}

/// Enable trace printing.
void enableTraceLogging();

/// Enable printing of the individual program node steps of the interpreter.
void enableStepLogging();

/// Enable printing of the statement coverage statistics the interpreter collects.
void enableCoverageLogging();

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_LOGGING_H_ */
