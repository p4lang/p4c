#include "backends/p4tools/modules/smith/options.h"

#include <cstdlib>
#include <iostream>
#include <vector>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/core/target.h"

namespace P4Tools {

SmithOptions &SmithOptions::get() {
    static SmithOptions INSTANCE;
    return INSTANCE;
}

const char *SmithOptions::getIncludePath() {
    P4C_UNIMPLEMENTED("getIncludePath !implemented for P4Smith.");
}

void SmithOptions::processArgs(const std::vector<const char *> &args) {
    // // Compiler expects path to executable as first element in argument list.
    // compilerArgs.push_back(args.at(0));

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
    auto *remainingArgs = Util::Options::process(argc, argv);
    if ((remainingArgs == nullptr) || ::errorCount() > 0) {
        return;
    }
}

const char *defaultMessage = "Generate a P4 program";

SmithOptions::SmithOptions() : AbstractP4cToolOptions("P4Smith options.") {}

}  // namespace P4Tools
