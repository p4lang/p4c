#include "backends/p4tools/modules/smith/options.h"

#include <cstdlib>
#include <tuple>
#include <vector>

#include "backends/p4tools/common/options.h"
#include "backends/p4tools/modules/smith/toolname.h"
#include "lib/compile_context.h"
#include "lib/error.h"
#include "lib/exceptions.h"

namespace P4Tools {

SmithOptions &SmithOptions::get() {
    static SmithOptions INSTANCE;
    return INSTANCE;
}

const char *SmithOptions::getIncludePath() const {
    P4C_UNIMPLEMENTED("getIncludePath is not implemented for P4Smith.");
}

void SmithOptions::processArgs(const std::vector<const char *> &args) {
    // Convert to the standard (argc, argv) pair.
    int argc = 0;
    char **argv = nullptr;
    std::tie(argc, argv) = convertArgs(args);

    // Establish a dummy compilation context so that we can use ::error to report errors while
    // processing command-line options.
    class DummyCompileContext : public BaseCompileContext {
    } dummyContext;
    AutoCompileContext autoDummyContext(&dummyContext);

    // Delegate to the hook.
    auto *remainingArgs = P4Tools::AbstractP4cToolOptions::process(argc, argv);
    if ((remainingArgs == nullptr) || ::errorCount() > 0) {
        return;
    }
}

SmithOptions::SmithOptions() : AbstractP4cToolOptions(P4Smith::TOOL_NAME, "P4Smith options.") {}

}  // namespace P4Tools
