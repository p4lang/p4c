#ifndef IR_PASS_UTILS_H_
#define IR_PASS_UTILS_H_

#include "ir/pass_manager.h"
#include "lib/compile_context.h"

/// @file
/// @authors Vladimír Štill
/// @brief Utilities for passes and pass managers.

namespace P4 {

inline const char *DIAGNOSTIC_COUNT_IN_PASS_TAG = "diagnosticCountInPass";

/// @brief This creates a debug hook that prints information about the number of diagnostics of
/// different levels (error, warning, info) printed by each of the pass after it ended. This is
/// intended to help in debuggig and testing.
///
/// NOTE: You either need to create one pass and use its copies in all the pass managers in the
/// compilation, or create each of the following hooks only after the previous compilations teps had
/// already run. Otherwise, you can get spurious logs for a first pass of a pass manager running
/// after some diagnostics were emitted.
///
/// It logs the messages if the log level for "diagnosticCountInPass" is at least 1. If the log
/// level is at least 4, the logs also include the sequence number of the pass that printed the
/// message in the pass manager.
///
/// @param ctxt Optionally, you can provide a compilation context to take the diagnostic counts
/// from. If not provied BaseCompileContext::get() is used.
/// @return A debug hook suitable for using in pass managers.
DebugHook getDiagnosticCountInPassHook(BaseCompileContext &ctxt = BaseCompileContext::get());

}  // namespace P4

#endif  // IR_PASS_UTILS_H_
