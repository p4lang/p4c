#ifndef IR_PASS_UTILS_H_
#define IR_PASS_UTILS_H_

#include "ir/pass_manager.h"
#include "lib/compile_context.h"

/// @file
/// @brief Utilities for passes and pass managers.

namespace P4 {

inline const char *DIAGNOSTIC_COUNT_IN_PASS_TAG = "diagnosticCountInPass";

/// @brief This creates a debug hook that prints information about the number of diagnostics of
/// different levels (error, warning, info) printed by each of the pass after it ended. This is
/// intended to help in debuggig and testing.
///
/// NOTE: You either need to use one \ref DiagnosticCountInfo object (and its \ref
/// getPassManagerHook) for the entire compilation, create one pass and use its copies in all the
/// pass managers in the compilation, or create each of the following hooks only after the previous
/// compilations steps had already run. Otherwise, you can get spurious logs for a first pass of a
/// pass manager running after some diagnostics were emitted.
///
/// It logs the messages if the log level for "diagnosticCountInPass" is at least 1. If the log
/// level is at least 4, the logs also include the sequence number of the pass that printed the
/// message in the pass manager.
///
/// @param ctxt Optionally, you can provide a compilation context to take the diagnostic counts
/// from. If not provied BaseCompileContext::get() is used.
/// @return A debug hook suitable for using in pass managers.
DebugHook getDiagnosticCountInPassHook(BaseCompileContext &ctxt = BaseCompileContext::get());

struct DiagnosticCountInfoState;  // forward declaration

/// A guard useful for emitting diagnostic info about non-passes.
/// \see DiagnosticCountInfo::getInfoGuard.
struct DiagnosticCountInfoGuard {
    ~DiagnosticCountInfoGuard();

    /// avoid creating moved-from object and copies that could emit spurious logs.
    DiagnosticCountInfoGuard(DiagnosticCountInfoGuard &&) = delete;

    friend struct DiagnosticCountInfo;

 private:
    DiagnosticCountInfoGuard(std::string_view componentInfo,
                             std::shared_ptr<DiagnosticCountInfoState> state)
        : componentInfo(componentInfo), state(state) {}
    std::string_view componentInfo;
    std::shared_ptr<DiagnosticCountInfoState> state;
};

/// An alternative interface to the diagnostic count info that allows using it outside of pass
/// manager (but can also generate a pass manager hook).
/// All copies of one object and all hooks and guards derived from it share the same diagnostic
/// message counts so they can provide consistent logs.
///
/// \see getDiagnosticCountInPassHook
struct DiagnosticCountInfo {
    /// @param ctxt Optionally, you can provide a compilation context to take the diagnostic counts
    /// from. If not provied BaseCompileContext::get() is used.
    explicit DiagnosticCountInfo(BaseCompileContext &ctxt = BaseCompileContext::get());

    /// Very similar to \ref getDiagnosticCountInPassHook, but the state is tied with this instance
    /// so that calling the hook and the \ref emitInfo/\ref getInfoGuard during one compilation will
    /// produce the correct counts.
    DebugHook getPassManagerHook();

    /// Emits the information like printed by the debug hook, except using componentInfo as the info
    /// at the beginning of the line: the line will be in form "<componentInfo> emitted <counts>".
    /// This is useful e.g. for printing info about diagnostics in parser.
    void emitInfo(std::string_view componentInfo);

    /// Similar to \ref emitInfo, but prints the info at the moment the returned guard is
    /// destructed.
    DiagnosticCountInfoGuard getInfoGuard(std::string_view componentInfo);

 private:
    std::shared_ptr<DiagnosticCountInfoState> state;
};

}  // namespace P4

#endif  // IR_PASS_UTILS_H_
