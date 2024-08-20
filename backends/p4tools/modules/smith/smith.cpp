#include "backends/p4tools/modules/smith/smith.h"

#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include "backends/p4tools/common/compiler/compiler_result.h"
#include "backends/p4tools/common/lib/logging.h"
#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/smith/common/probabilities.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/core/target.h"
#include "backends/p4tools/modules/smith/options.h"
#include "backends/p4tools/modules/smith/register.h"
#include "frontends/common/parser_options.h"
#include "frontends/p4/toP4/toP4.h"
#include "ir/ir.h"
#include "lib/compile_context.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/nullstream.h"

namespace P4::P4Tools::P4Smith {

void Smith::registerTarget() { registerSmithTargets(); }

int Smith::main(const std::vector<const char *> &args) {
    // Register supported compiler targets.
    registerTarget();

    // Process command-line options.
    auto &toolOptions = SmithOptions::get();
    auto compileContext = toolOptions.process(args);
    if (!compileContext) {
        return EXIT_FAILURE;
    }

    // If not explicitly disabled, print basic information to standard output.
    if (!toolOptions.disableInformationLogging) {
        enableInformationLogging();
    }

    // Set up the compilation context.
    AutoCompileContext autoContext(*compileContext);
    // Instantiate a dummy program for now. In the future this can be a skeleton.
    const IR::P4Program program;
    return mainImpl(CompilerResult(program));
}

int Smith::mainImpl(const CompilerResult & /*result*/) {
    registerSmithTargets();

    auto outputFile = P4CContext::get().options().file;

    auto &smithOptions = P4Tools::SmithOptions::get();

    // Use a default name if no specific output name is provided.
    if (outputFile.empty()) {
        outputFile = "out.p4";
    }
    auto *ostream = openFile(outputFile, false);
    if (ostream == nullptr) {
        ::P4::error("must have [file]");
        exit(EXIT_FAILURE);
    }
    if (smithOptions.seed.has_value()) {
        printInfo("Using provided seed");
    } else {
        printInfo("Generating seed...");
        // No seed provided, we generate our own.
        std::random_device r;
        smithOptions.seed = r();
        Utils::setRandomSeed(*smithOptions.seed);
    }
    // TODO(fruffy): Remove this. We are setting the seed in two frameworks.
    printInfo("============ Program seed %1% =============\n", *smithOptions.seed);
    const auto &smithTarget = SmithTarget::get();

    auto result = smithTarget.writeTargetPreamble(ostream);
    if (result != EXIT_SUCCESS) {
        return result;
    }
    const auto *generatedProgram = smithTarget.generateP4Program();
    // Use ToP4 to write the P4 program to the specified stream.
    P4::ToP4 top4(ostream, false);
    generatedProgram->apply(top4);
    ostream->flush();
    P4Scope::endLocalScope();

    return EXIT_SUCCESS;
}

}  // namespace P4::P4Tools::P4Smith
