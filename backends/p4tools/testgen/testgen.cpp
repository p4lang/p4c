#include "backends/p4tools/testgen/testgen.h"

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/none.hpp>
#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/timer.h"
#include "backends/p4tools/common/lib/util.h"
#include "frontends/common/parser_options.h"
#include "lib/error.h"

#include "backends/p4tools/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/testgen/core/exploration_strategy/incremental_max_coverage_stack.h"
#include "backends/p4tools/testgen/core/exploration_strategy/incremental_stack.h"
#include "backends/p4tools/testgen/core/exploration_strategy/linear_enumeration.h"
#include "backends/p4tools/testgen/core/exploration_strategy/random_access_max_coverage.h"
#include "backends/p4tools/testgen/core/exploration_strategy/random_access_stack.h"
#include "backends/p4tools/testgen/core/exploration_strategy/selected_branches.h"
#include "backends/p4tools/testgen/core/target.h"
#include "backends/p4tools/testgen/lib/logging.h"
#include "backends/p4tools/testgen/lib/test_backend.h"
#include "backends/p4tools/testgen/register.h"

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
    cstring testDirStr = TestgenOptions::get().outputDir;
    auto seed = TestgenOptions::get().seed;

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

    if (seed != boost::none) {
        // Initialize the global seed for randomness.
        TestgenUtils::setRandomSeed(*seed);
        printFeature("test_info", 4, "============ Program seed %1% =============\n", *seed);
    }

    Z3Solver solver;

    auto symExec = [&solver, &programInfo, seed]() -> ExplorationStrategy* {
        std::string explorationStrategy = TestgenOptions::get().explorationStrategy;
        if (explorationStrategy.compare("randomAccessStack") == 0) {
            // If the user mistakenly specifies an invalid popLevel, we set it to 3.
            int popLevel =
                (TestgenOptions::get().popLevel > 1) ? TestgenOptions::get().popLevel : 3;
            return new RandomAccessStack(solver, *programInfo, seed, popLevel);
        }
        if (explorationStrategy.compare("linearEnumeration") == 0) {
            // If the user mistakenly specifies an invalid bound, we set it to 2
            // to generate at least 2 tests.
            int linearBound = (TestgenOptions::get().linearEnumeration > 1)
                                  ? TestgenOptions::get().linearEnumeration
                                  : 2;
            return new LinearEnumeration(solver, *programInfo, seed, linearBound);
        }
        if (explorationStrategy.compare("maxCoverage") == 0) {
            return new IncrementalMaxCoverageStack(solver, *programInfo, seed);
        }
        if (explorationStrategy.compare("randomAccessMaxCoverage") == 0) {
            // If the user mistakenly sets and invalid saddlePoint, we set it to 5.
            int saddlePoint =
                (TestgenOptions::get().saddlePoint > 1) ? TestgenOptions::get().saddlePoint : 5;
            return new RandomAccessMaxCoverage(solver, *programInfo, seed, saddlePoint);
        }
        if (!TestgenOptions::get().selectedBranches.empty()) {
            std::string selectedBranchesStr = TestgenOptions::get().selectedBranches;
            return new SelectedBranches(solver, *programInfo, seed, selectedBranchesStr);
        }
        return new IncrementalStack(solver, *programInfo, seed);
    }();

    // Define how to handle the final state for each test. This is target defined.
    auto* testBackend = TestgenTarget::getTestBackend(*programInfo, *symExec, testPath, seed);
    ExplorationStrategy::Callback callBack =
        std::bind(&TestBackEnd::run, testBackend, std::placeholders::_1);

    try {
        // Run the symbolic executor with given exploration strategy.
        symExec->run(callBack);
    } catch (...) {
        if (TestgenOptions::get().trackBranches) {
            // Print list of the selected branches and store all information into
            // dumpFolder/selectedBranches.txt file.
            // This printed list could be used for repeat this bug in arguments of --input-branches
            // command line. For example, --input-branches "1,1".
            symExec->printCurrentTraceAndBranches(std::cerr);
        }
        throw;
    }

    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace P4Testgen

}  // namespace P4Tools
