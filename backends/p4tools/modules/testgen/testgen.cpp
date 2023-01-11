#include "backends/p4tools/modules/testgen/testgen.h"

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>

#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/none.hpp>
#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/util.h"
#include "frontends/common/parser_options.h"
#include "lib/cstring.h"
#include "lib/error.h"

#include "backends/p4tools/modules/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/inc_max_coverage_stack.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/incremental_stack.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/linear_enumeration.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/random_access_stack.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/rnd_access_max_coverage.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/selected_branches.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/logging.h"
#include "backends/p4tools/modules/testgen/lib/test_backend.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/register.h"

namespace fs = boost::filesystem;

namespace P4Tools {

namespace P4Testgen {

void Testgen::registerTarget() {
    // Register all available compiler targets.
    // These are discovered by CMAKE, which fills out the register.h.in file.
    registerCompilerTargets();
}

int Testgen::mainImpl(const IR::P4Program* program) {
    // Register all available testgen targets.
    // These are discovered by CMAKE, which fills out the register.h.in file.
    registerTestgenTargets();

    const auto* programInfo = TestgenTarget::initProgram(program);
    if (programInfo == nullptr) {
        ::error("Program not supported by target device and architecture.");
        return EXIT_FAILURE;
    }
    if (::errorCount() > 0) {
        ::error("Testgen: Encountered errors during preprocessing. Exiting");
        return EXIT_FAILURE;
    }

    // Print basic information for each test.
    enableInformationLogging();

    auto const inputFile = P4CContext::get().options().file;
    const auto& testgenOptions = TestgenOptions::get();
    cstring testDirStr = testgenOptions.outputDir;
    auto seed = Utils::getCurrentSeed();
    if (seed) {
        printFeature("test_info", 4, "============ Program seed %1% =============\n", *seed);
    }

    // Get the basename of the input file and remove the extension
    // This assumes that inputFile is not null.
    auto programName = fs::path(inputFile).filename().replace_extension("");
    // Create the directory, if the directory string is valid and if it does not exist.
    auto testPath = programName;
    if (!testDirStr.isNullOrEmpty()) {
        auto testDir = fs::path(testDirStr);
        fs::create_directories(testDir);
        testPath = fs::path(testDir) / testPath;
    }

    Z3Solver solver;

    auto* symExec = [&solver, &programInfo, &testgenOptions]() -> ExplorationStrategy* {
        auto explorationStrategy = testgenOptions.explorationStrategy;
        if (explorationStrategy == "RANDOM_ACCESS_STACK") {
            // If the user mistakenly specifies an invalid popLevel, we set it to 3.
            auto popLevel = testgenOptions.popLevel;
            if (popLevel <= 1) {
                ::warning("--pop-level must be greater than 1; using default value of 3.\n");
                popLevel = 3;
            }
            return new RandomAccessStack(solver, *programInfo, popLevel);
        }
        if (explorationStrategy == "LINEAR_ENUMERATION") {
            return new LinearEnumeration(solver, *programInfo, testgenOptions.linearEnumeration);
        }
        if (explorationStrategy == "MAX_COVERAGE") {
            return new IncrementalMaxCoverageStack(solver, *programInfo);
        }
        if (explorationStrategy == "RANDOM_ACCESS_MAX_COVERAGE") {
            return new RandomAccessMaxCoverage(solver, *programInfo, testgenOptions.saddlePoint);
        }
        if (!testgenOptions.selectedBranches.empty()) {
            std::string selectedBranchesStr = testgenOptions.selectedBranches;
            return new SelectedBranches(solver, *programInfo, selectedBranchesStr);
        }
        return new IncrementalStack(solver, *programInfo);
    }();

    // Define how to handle the final state for each test. This is target defined.
    auto* testBackend = TestgenTarget::getTestBackend(*programInfo, *symExec, testPath, seed);
    ExplorationStrategy::Callback callBack =
        std::bind(&TestBackEnd::run, testBackend, std::placeholders::_1);

    try {
        // Run the symbolic executor with given exploration strategy.
        symExec->run(callBack);
    } catch (...) {
        if (testgenOptions.trackBranches) {
            // Print list of the selected branches and store all information into
            // dumpFolder/selectedBranches.txt file.
            // This printed list could be used for repeat this bug in arguments of --input-branches
            // command line. For example, --input-branches "1,1".
            symExec->printCurrentTraceAndBranches(std::cerr);
        }
        throw;
    }

    if (testBackend->getTestCount() == 0) {
        ::warning(
            "Unable to generate tests with given inputs. Double-check provided options and "
            "parameters.\n");
    }

    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace P4Testgen

}  // namespace P4Tools
