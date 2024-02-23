#include "backends/p4tools/modules/testgen/testgen.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>

#include "backends/p4tools/common/compiler/context.h"
#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/util.h"
#include "frontends/common/parser_options.h"
#include "ir/solver.h"
#include "lib/cstring.h"
#include "lib/error.h"

#include "backends/p4tools/modules/testgen/core/compiler_target.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/depth_first.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/greedy_node_cov.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/path_selection.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/random_backtrack.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/selected_branches.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/test_backend.h"
#include "backends/p4tools/modules/testgen/lib/test_framework.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/register.h"

namespace P4Tools::P4Testgen {

namespace {

/// Pick the path selection algorithm for the symbolic executor.
SymbolicExecutor *pickExecutionEngine(const TestgenOptions &testgenOptions,
                                      const ProgramInfo &programInfo, AbstractSolver &solver) {
    const auto &pathSelectionPolicy = testgenOptions.pathSelectionPolicy;
    if (pathSelectionPolicy == PathSelectionPolicy::GreedyStmtCoverage) {
        return new GreedyNodeSelection(solver, programInfo);
    }
    if (pathSelectionPolicy == PathSelectionPolicy::RandomBacktrack) {
        return new RandomBacktrack(solver, programInfo);
    }
    if (!testgenOptions.selectedBranches.empty()) {
        std::string selectedBranchesStr = testgenOptions.selectedBranches;
        return new SelectedBranches(solver, programInfo, selectedBranchesStr);
    }
    return new DepthFirstSearch(solver, programInfo);
}

/// Analyse the results of the symbolic execution and generate diagnostic messages.
int postProcess(const TestgenOptions &testgenOptions, const TestBackEnd &testBackend) {
    // Do not print this warning if assertion mode is enabled.
    if (testBackend.getTestCount() == 0 && !testgenOptions.assertionModeEnabled) {
        ::warning(
            "Unable to generate tests with given inputs. Double-check provided options and "
            "parameters.\n");
    }
    if (testBackend.getCoverage() < testgenOptions.minCoverage) {
        ::error("The tests did not achieve requested coverage of %1%, the coverage is %2%.",
                testgenOptions.minCoverage, testBackend.getCoverage());
    }

    return ::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

std::optional<AbstractTestList> generateAndCollectAbstractTests(
    const TestgenOptions &testgenOptions, const ProgramInfo &programInfo) {
    if (!testgenOptions.testBaseName.has_value()) {
        ::error(
            "Test collection requires a test name. No name was provided as part of the "
            "P4Testgen options.");
        return std::nullopt;
    }

    // The test name is the stem of the output base path.
    TestBackendConfiguration testBackendConfiguration{testgenOptions.testBaseName.value(),
                                                      testgenOptions.maxTests, std::nullopt,
                                                      testgenOptions.seed};
    // Need to declare the solver here to ensure its lifetime.
    Z3Solver solver;
    auto *symbolicExecutor = pickExecutionEngine(testgenOptions, programInfo, solver);

    // Each test back end has a different run function.
    auto *testBackend =
        TestgenTarget::getTestBackend(programInfo, testBackendConfiguration, *symbolicExecutor);

    // Define how to handle the final state for each test. This is target defined.
    // We delegate execution to the symbolic executor.
    symbolicExecutor->run([testBackend](auto &&finalState) {
        return testBackend->run(std::forward<decltype(finalState)>(finalState));
    });
    auto result = postProcess(testgenOptions, *testBackend);
    if (result != EXIT_SUCCESS) {
        return std::nullopt;
    }
    return testBackend->getTests();
}

int generateAndWriteAbstractTests(const TestgenOptions &testgenOptions,
                                  const ProgramInfo &programInfo) {
    cstring inputFile = P4CContext::get().options().file;
    if (inputFile == nullptr) {
        ::error("No input file provided.");
        return EXIT_FAILURE;
    }

    /// If the test name is not provided, use the steam of the input file name as test name.
    auto testPath = std::filesystem::path(inputFile.c_str()).stem();
    if (testgenOptions.testBaseName.has_value()) {
        testPath = testPath.replace_filename(testgenOptions.testBaseName.value().c_str());
    }

    // Create the directory, if the directory string is valid and if it does not exist.
    cstring testDirStr = testgenOptions.outputDir;
    if (!testDirStr.isNullOrEmpty()) {
        auto testDir = std::filesystem::path(testDirStr.c_str());
        try {
            std::filesystem::create_directories(testDir);
        } catch (const std::exception &err) {
            ::error("Unable to create directory %1%: %2%", testDir.c_str(), err.what());
            return EXIT_FAILURE;
        }
        testPath = testDir / testPath;
    }

    // The test name is the stem of the output base path.
    TestBackendConfiguration testBackendConfiguration{testPath.c_str(), testgenOptions.maxTests,
                                                      testPath, testgenOptions.seed};

    // Need to declare the solver here to ensure its lifetime.
    Z3Solver solver;
    auto *symbolicExecutor = pickExecutionEngine(testgenOptions, programInfo, solver);

    // Each test back end has a different run function.
    auto *testBackend =
        TestgenTarget::getTestBackend(programInfo, testBackendConfiguration, *symbolicExecutor);

    // Define how to handle the final state for each test. This is target defined.
    // We delegate execution to the symbolic executor.
    symbolicExecutor->run([testBackend](auto &&finalState) {
        return testBackend->run(std::forward<decltype(finalState)>(finalState));
    });
    return postProcess(testgenOptions, *testBackend);
}

std::optional<AbstractTestList> generateTestsImpl(const std::string &program,
                                                  const CompilerOptions &compilerOptions,
                                                  const TestgenOptions &testgenOptions) {
    // Register supported compiler targets.
    registerCompilerTargets();

    // Register supported Testgen targets.
    registerTestgenTargets();

    P4Tools::Target::init(compilerOptions.target.c_str(), compilerOptions.arch.c_str());

    // Set up the compilation context.
    auto *compileContext = new CompileContext<CompilerOptions>();
    compileContext->options() = compilerOptions;
    AutoCompileContext autoContext(compileContext);
    // Run the compiler to get an IR and invoke the tool.
    const auto compilerResultOpt = P4Tools::CompilerTarget::runCompiler(program);
    if (!compilerResultOpt.has_value()) {
        return std::nullopt;
    }
    const auto *testgenCompilerResult =
        compilerResultOpt.value().get().checkedTo<TestgenCompilerResult>();

    const auto *programInfo = TestgenTarget::produceProgramInfo(*testgenCompilerResult);
    if (programInfo == nullptr || ::errorCount() > 0) {
        ::error("P4Testgen encountered errors during preprocessing.");
        return std::nullopt;
    }

    return generateAndCollectAbstractTests(testgenOptions, *programInfo);
}

}  // namespace

void Testgen::registerTarget() {
    // Register all available compiler targets.
    // These are discovered by CMAKE, which fills out the register.h.in file.
    registerCompilerTargets();
}

int Testgen::mainImpl(const CompilerResult &compilerResult) {
    // Register all available P4Testgen targets.
    // These are discovered by CMAKE, which fills out the register.h.in file.
    registerTestgenTargets();

    // Make sure the input result corresponds to the result we expect.
    const auto *testgenCompilerResult = compilerResult.checkedTo<TestgenCompilerResult>();

    const auto *programInfo = TestgenTarget::produceProgramInfo(*testgenCompilerResult);
    if (programInfo == nullptr || ::errorCount() > 0) {
        ::error("P4Testgen encountered errors during preprocessing.");
        return EXIT_FAILURE;
    }

    const auto &testgenOptions = TestgenOptions::get();
    return generateAndWriteAbstractTests(testgenOptions, *programInfo);
}

std::optional<AbstractTestList> Testgen::generateTests(const std::string &program,
                                                       const CompilerOptions &compilerOptions,
                                                       const TestgenOptions &testgenOptions) {
    try {
        return generateTestsImpl(program, compilerOptions, testgenOptions);
    } catch (const std::exception &e) {
        std::cerr << "Internal error: " << e.what() << "\n";
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
    return std::nullopt;
}

}  // namespace P4Tools::P4Testgen
