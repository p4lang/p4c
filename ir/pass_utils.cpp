#include "ir/pass_utils.h"

#include <initializer_list>
#include <tuple>

#include "absl/strings/str_cat.h"
#include "lib/log.h"
#include "pass_utils.h"

/// @file
/// @brief Utilities for passes and pass managers.

namespace P4 {

// The state needs to be shared between all hook copies as the hook is copied in the pass manager.
struct DiagnosticCountInfoState {
    explicit DiagnosticCountInfoState(BaseCompileContext &ctxt)
        : ctxt(ctxt),
          lastDiagCount(ctxt.errorReporter().getDiagnosticCount()),
          lastErrorCount(ctxt.errorReporter().getErrorCount()),
          lastWarningCount(ctxt.errorReporter().getWarningCount()),
          lastInfoCount(ctxt.errorReporter().getInfoCount()) {}

    void info(std::string_view componentInfo) {
        if (!Log::fileLogLevelIsAtLeast(DIAGNOSTIC_COUNT_IN_PASS_TAG, 1)) return;

        unsigned diag = ctxt.errorReporter().getDiagnosticCount();

        if (diag != lastDiagCount) {
            unsigned errs = ctxt.errorReporter().getErrorCount(),
                     warns = ctxt.errorReporter().getWarningCount(),
                     infos = ctxt.errorReporter().getInfoCount();
            std::stringstream summary;
            const char *sep = "";
            for (auto [newCnt, oldCnt, kind] : {std::tuple(errs, lastErrorCount, "error"),
                                                std::tuple(warns, lastWarningCount, "warning"),
                                                std::tuple(infos, lastInfoCount, "info")}) {
                if (newCnt > oldCnt) {
                    auto diff = newCnt - oldCnt;
                    summary << sep << diff << " " << kind << (diff > 1 ? "s" : "");
                    sep = ", ";
                }
            }

            LOG_FEATURE(DIAGNOSTIC_COUNT_IN_PASS_TAG, 1,
                        componentInfo << " emitted " << summary.str());
            lastDiagCount = diag;
            lastErrorCount = errs;
            lastWarningCount = warns;
            lastInfoCount = infos;
        }
    }

    BaseCompileContext &ctxt;
    unsigned lastDiagCount, lastErrorCount, lastWarningCount, lastInfoCount;
};

static DebugHook hook(std::shared_ptr<DiagnosticCountInfoState> state) {
    return [state](const char *manager, unsigned seqNo, const char *pass, const IR::Node *) {
        if (!Log::fileLogLevelIsAtLeast(DIAGNOSTIC_COUNT_IN_PASS_TAG, 1)) return;

        std::string compInfo = absl::StrCat("PASS ", manager);
        // If the log level is high enough, emit also pass count.
        if (Log::fileLogLevelIsAtLeast(DIAGNOSTIC_COUNT_IN_PASS_TAG, 4)) {
            absl::StrAppend(&compInfo, "[", seqNo, "]");
        }
        absl::StrAppend(&compInfo, " ~> ", pass);

        state->info(compInfo);
    };
}

DebugHook getDiagnosticCountInPassHook(BaseCompileContext &ctxt) {
    return hook(std::make_shared<DiagnosticCountInfoState>(ctxt));
}

DiagnosticCountInfoGuard::~DiagnosticCountInfoGuard() { state->info(componentInfo); }

DiagnosticCountInfo::DiagnosticCountInfo(BaseCompileContext &ctxt)
    : state(std::make_shared<DiagnosticCountInfoState>(ctxt)) {}

DebugHook DiagnosticCountInfo::getPassManagerHook() { return hook(state); }

void DiagnosticCountInfo::emitInfo(std::string_view componentInfo) { state->info(componentInfo); }

DiagnosticCountInfoGuard DiagnosticCountInfo::getInfoGuard(std::string_view componentInfo) {
    return {componentInfo, state};
}

}  // namespace P4
