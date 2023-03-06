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
#include "backends/p4tools/modules/testgen/core/exploration_strategy/greedy_potential.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/inc_max_coverage_stack.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/incremental_stack.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/linear_enumeration.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/random_access_stack.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/rnd_access_max_coverage.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/selected_branches.h"
#include "backends/p4tools/modules/testgen/core/exploration_strategy/unbounded_rnd_ac_stack.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/logging.h"
#include "backends/p4tools/modules/testgen/lib/test_backend.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/register.h"

namespace P4Tools::P4Testgen {

void Testgen::registerTarget() {
    // Register all available compiler targets.
    // These are discovered by CMAKE, which fills out the register.h.in file.
    registerCompilerTargets();
}

ExplorationStrategy *pickExecutionEngine(const TestgenOptions &testgenOptions,
                                         const ProgramInfo *programInfo, AbstractSolver &solver) {
    const auto &pathSelectionPolicy = testgenOptions.pathSelectionPolicy;
    if (pathSelectionPolicy == PathSelectionPolicy::RandomAccessStack) {
        // If the user mistakenly specifies an invalid popLevel, we set it to 3.
        auto popLevel = testgenOptions.popLevel;
        if (popLevel <= 1) {
            ::warning("--pop-level must be greater than 1; using default value of 3.\n");
            popLevel = 3;
        }
        return new RandomAccessStack(solver, *programInfo, popLevel);
    }
    if (pathSelectionPolicy == PathSelectionPolicy::GreedyPotential) {
        return new GreedyPotential(solver, *programInfo);
    }
    if (pathSelectionPolicy == PathSelectionPolicy::UnboundedRandomAccessStack) {
        return new UnboundedRandomAccessStack(solver, *programInfo);
    }
    if (pathSelectionPolicy == PathSelectionPolicy::LinearEnumeration) {
        return new LinearEnumeration(solver, *programInfo, testgenOptions.linearEnumeration);
    }
    if (pathSelectionPolicy == PathSelectionPolicy::MaxCoverage) {
        return new IncrementalMaxCoverageStack(solver, *programInfo);
    }
    if (pathSelectionPolicy == PathSelectionPolicy::RandomAccessMaxCoverage) {
        return new RandomAccessMaxCoverage(solver, *programInfo, testgenOptions.saddlePoint);
    }
    if (!testgenOptions.selectedBranches.empty()) {
        std::string selectedBranchesStr = testgenOptions.selectedBranches;
        return new SelectedBranches(solver, *programInfo, selectedBranchesStr);
    }
    return new IncrementalStack(solver, *programInfo);
}

int Testgen::mainImpl(const IR::P4Program *program) {
    // Register all available testgen targets.
    // These are discovered by CMAKE, which fills out the register.h.in file.
    registerTestgenTargets();

    const auto *programInfo = TestgenTarget::initProgram(program);
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
    const auto &testgenOptions = TestgenOptions::get();
    cstring testDirStr = testgenOptions.outputDir;
    auto seed = Utils::getCurrentSeed();
    if (seed) {
        printFeature("test_info", 4, "============ Program seed %1% =============\n", *seed);
    }

    // Get the basename of the input file and remove the extension
    // This assumes that inputFile is not null.
    auto programName = boost::filesystem::path(inputFile).filename().replace_extension("");
    // Create the directory, if the directory string is valid and if it does not exist.
    auto testPath = programName;
    if (!testDirStr.isNullOrEmpty()) {
        auto testDir = boost::filesystem::path(testDirStr);
        boost::filesystem::create_directories(testDir);
        testPath = boost::filesystem::path(testDir) / testPath;
    }
    // Need to declare the solver here to ensure its lifetime.
    Z3Solver solver;
    auto *symExec = pickExecutionEngine(testgenOptions, programInfo, solver);

    // Define how to handle the final state for each test. This is target defined.
    auto *testBackend = TestgenTarget::getTestBackend(*programInfo, *symExec, testPath, seed);
    // Each test back end has a different run function.
    // We delegate execution to the symbolic executor.
    auto callBack = [testBackend](auto &&finalState) {
        return testBackend->run(std::forward<decltype(finalState)>(finalState));
    };

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
    // Emit a performance report, if desired.
    testBackend->printPerformanceReport(true);

    // Do not print this warning if assertion mode is enabled.
    if (testBackend->getTestCount() == 0 && !testgenOptions.assertionModeEnabled) {
        ::warning(
            "Unable to generate tests with given inputs. Double-check provided options and "
            "parameters.\n");
    }

    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace P4Tools::P4Testgen
