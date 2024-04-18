#include "ir/pass_utils.h"

#include <initializer_list>
#include <tuple>

#include "lib/log.h"

/// @file
/// @authors Vladimír Štill
/// @brief Utilities for passes and pass managers.

namespace P4 {

// The state needs to be shared between all hook copies as the hook is copied in the pass manager.
struct DiagnosticCountInPassHookState {
    explicit DiagnosticCountInPassHookState(BaseCompileContext &ctxt)
        : lastDiagCount(ctxt.errorReporter().getDiagnosticCount()),
          lastErrorCount(ctxt.errorReporter().getErrorCount()),
          lastWarningCount(ctxt.errorReporter().getWarningCount()),
          lastInfoCount(ctxt.errorReporter().getInfoCount()) {}

    unsigned lastDiagCount, lastErrorCount, lastWarningCount, lastInfoCount;
};

DebugHook getDiagnosticCountInPassHook(BaseCompileContext &ctxt) {
    return [state = std::make_shared<DiagnosticCountInPassHookState>(ctxt), &ctxt](
               const char *manager, unsigned seqNo, const char *pass, const IR::Node *) mutable {
        unsigned diag = ctxt.errorReporter().getDiagnosticCount();

        if (diag != state->lastDiagCount) {
            unsigned errs = ctxt.errorReporter().getErrorCount(),
                     warns = ctxt.errorReporter().getWarningCount(),
                     infos = ctxt.errorReporter().getInfoCount();
            std::stringstream summary;
            const char *sep = "";
            for (auto [newCnt, oldCnt, kind] :
                 {std::tuple(errs, state->lastErrorCount, "error"),
                  std::tuple(warns, state->lastWarningCount, "warning"),
                  std::tuple(infos, state->lastInfoCount, "info")}) {
                if (newCnt > oldCnt) {
                    auto diff = newCnt - oldCnt;
                    summary << sep << diff << " " << kind << (diff > 1 ? "s" : "");
                    sep = ", ";
                }
            }

            // if the log level is high enough, emit also pass count
            if (Log::fileLogLevelIsAtLeast(DIAGNOSTIC_COUNT_IN_PASS_TAG, 4)) {
                LOG_FEATURE(DIAGNOSTIC_COUNT_IN_PASS_TAG, 4,
                            "PASS " << manager << "[" << seqNo << "] ~> " << pass << " emitted "
                                    << summary.str());
            } else {
                LOG_FEATURE(DIAGNOSTIC_COUNT_IN_PASS_TAG, 1,
                            "PASS " << manager << " ~> " << pass << " emitted " << summary.str());
            }
            state->lastDiagCount = diag;
            state->lastErrorCount = errs;
            state->lastWarningCount = warns;
            state->lastInfoCount = infos;
        }
    };
}

}  // namespace P4
