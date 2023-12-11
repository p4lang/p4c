#include "backends/p4tools/modules/testgen/testgen.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/util.h"
#include "frontends/common/parser_options.h"
#include "ir/solver.h"
#include "lib/cstring.h"
#include "lib/error.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/depth_first.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/greedy_node_cov.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/path_selection.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/random_backtrack.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/selected_branches.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
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

SymbolicExecutor *pickExecutionEngine(const TestgenOptions &testgenOptions,
                                      const ProgramInfo *programInfo, AbstractSolver &solver) {
    const auto &pathSelectionPolicy = testgenOptions.pathSelectionPolicy;
    if (pathSelectionPolicy == PathSelectionPolicy::GreedyStmtCoverage) {
        return new GreedyNodeSelection(solver, *programInfo);
    }
    if (pathSelectionPolicy == PathSelectionPolicy::RandomBacktrack) {
        return new RandomBacktrack(solver, *programInfo);
    }
    if (!testgenOptions.selectedBranches.empty()) {
        std::string selectedBranchesStr = testgenOptions.selectedBranches;
        return new SelectedBranches(solver, *programInfo, selectedBranchesStr);
    }
    return new DepthFirstSearch(solver, *programInfo);
}

int generateAbstractTests(const TestgenOptions &testgenOptions, const ProgramInfo *programInfo,
                          SymbolicExecutor &symbex) {
    // Get the filename of the input file and remove the extension
    // This assumes that inputFile is not null.
    auto const inputFile = P4CContext::get().options().file;
    auto testPath = std::filesystem::path(inputFile.c_str()).stem();
    // Create the directory, if the directory string is valid and if it does not exist.
    cstring testDirStr = testgenOptions.outputDir;
    if (!testDirStr.isNullOrEmpty()) {
        auto testDir = std::filesystem::path(testDirStr.c_str());
        std::filesystem::create_directories(testDir);
        testPath = testDir / testPath;
    }
    // Each test back end has a different run function.
    auto *testBackend = TestgenTarget::getTestBackend(*programInfo, symbex, testPath);
    // Define how to handle the final state for each test. This is target defined.
    // We delegate execution to the symbolic executor.
    auto callBack = [testBackend](auto &&finalState) {
        return testBackend->run(std::forward<decltype(finalState)>(finalState));
    };

    symbex.run(callBack);

    // Emit a performance report, if desired.
    printPerformanceReport(testPath);

    // Do not print this warning if assertion mode is enabled.
    if (testBackend->getTestCount() == 0 && !testgenOptions.assertionModeEnabled) {
        ::warning(
            "Unable to generate tests with given inputs. Double-check provided options and "
            "parameters.\n");
    }
    if (testBackend->getCoverage() < testgenOptions.minCoverage) {
        ::error("The tests did not achieve requested coverage of %1%, the coverage is %2%.",
                testgenOptions.minCoverage, testBackend->getCoverage());
    }

    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
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

    // Get the options and the seed.
    const auto &testgenOptions = TestgenOptions::get();
    auto seed = Utils::getCurrentSeed();
    if (seed) {
        printFeature("test_info", 4, "============ Program seed %1% =============\n", *seed);
    }

    // Need to declare the solver here to ensure its lifetime.
    Z3Solver solver;
    auto *symbex = pickExecutionEngine(testgenOptions, programInfo, solver);

    return generateAbstractTests(testgenOptions, programInfo, *symbex);
}

}  // namespace P4Tools::P4Testgen
