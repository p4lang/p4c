#include "backends/p4tools/modules/testgen/testgen.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>

#include "backends/p4tools/common/compiler/context.h"
#include "backends/p4tools/common/core/z3_solver.h"
#include "frontends/common/parser_options.h"
#include "ir/solver.h"
#include "lib/cstring.h"
#include "lib/error.h"

#include "backends/p4tools/modules/testgen/core/compiler_result.h"
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
#include "backends/p4tools/modules/testgen/toolname.h"

namespace p4c::P4Tools::P4Testgen {

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
        ::p4c::warning(
            "Unable to generate tests with given inputs. Double-check provided options and "
            "parameters.\n");
    }
    if (testBackend.getCoverage() < testgenOptions.minCoverage) {
        ::p4c::error("The tests did not achieve requested coverage of %1%, the coverage is %2%.",
                     testgenOptions.minCoverage, testBackend.getCoverage());
    }

    return ::p4c::errorCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

std::optional<AbstractTestList> generateAndCollectAbstractTests(
    const TestgenOptions &testgenOptions, const ProgramInfo &programInfo) {
    if (!testgenOptions.testBaseName.has_value()) {
        ::p4c::error(
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
    std::filesystem::path testPath;
    /// If the test name is not provided, use the steam of the input file name as test name.
    if (testgenOptions.testBaseName.has_value()) {
        testPath = testgenOptions.testBaseName.value().c_str();
    } else if (!P4CContext::get().options().file.empty()) {
        testPath = P4CContext::get().options().file.stem();
    } else {
        ::p4c::error("Neither a file nor test base name was set. Can not infer a test name.");
    }

    // Create the directory, if the directory string is valid and if it does not exist.
    if (testgenOptions.outputDir.has_value()) {
        auto testDir = testgenOptions.outputDir.value();
        try {
            std::filesystem::create_directories(testDir);
        } catch (const std::exception &err) {
            ::p4c::error("Unable to create directory %1%: %2%", testDir.c_str(), err.what());
            return EXIT_FAILURE;
        }
        testPath = testDir / testPath;
    }

    // The test name is the stem of the output base path.
    TestBackendConfiguration testBackendConfiguration{
        cstring(testPath.c_str()), testgenOptions.maxTests, testPath, testgenOptions.seed};

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

std::optional<AbstractTestList> generateTestsImpl(std::optional<std::string_view> program,
                                                  const CompilerOptions &compilerOptions,
                                                  const TestgenOptions &testgenOptions,
                                                  bool writeTests) {
    P4Tools::Target::init(compilerOptions.target.c_str(), compilerOptions.arch.c_str());

    // Set up the compilation context.
    auto *compileContext = new CompileContext<CompilerOptions>();
    compileContext->options() = compilerOptions;
    AutoCompileContext autoContext(compileContext);
    CompilerResultOrError compilerResultOpt;
    if (program.has_value()) {
        // Run the compiler to get an IR and invoke the tool.
        compilerResultOpt =
            P4Tools::CompilerTarget::runCompiler(TOOL_NAME, std::string(program.value()));
    } else {
        if (compilerOptions.file.empty()) {
            ::p4c::error("Expected a file input.");
            return std::nullopt;
        }
        // Run the compiler to get an IR and invoke the tool.
        compilerResultOpt = P4Tools::CompilerTarget::runCompiler(TOOL_NAME);
    }

    if (!compilerResultOpt.has_value()) {
        ::p4c::error("Failed to run the compiler.");
        return std::nullopt;
    }

    const auto *testgenCompilerResult =
        compilerResultOpt.value().get().checkedTo<TestgenCompilerResult>();
    const auto *programInfo = TestgenTarget::produceProgramInfo(*testgenCompilerResult);
    if (programInfo == nullptr || ::p4c::errorCount() > 0) {
        ::p4c::error("P4Testgen encountered errors during preprocessing.");
        return std::nullopt;
    }

    if (writeTests) {
        int result = generateAndWriteAbstractTests(testgenOptions, *programInfo);
        if (result != EXIT_SUCCESS) {
            return std::nullopt;
        }
        return {};
    }
    return generateAndCollectAbstractTests(testgenOptions, *programInfo);
}

}  // namespace

void Testgen::registerTarget() {
    // Register all available P4Testgen targets.
    // These are discovered by CMAKE, which fills out the register.h.in file.
    registerTestgenTargets();
}

int Testgen::mainImpl(const CompilerResult &compilerResult) {
    // Make sure the input result corresponds to the result we expect.
    const auto *testgenCompilerResult = compilerResult.checkedTo<TestgenCompilerResult>();

    const auto *programInfo = TestgenTarget::produceProgramInfo(*testgenCompilerResult);
    if (programInfo == nullptr || ::p4c::errorCount() > 0) {
        ::p4c::error("P4Testgen encountered errors during preprocessing.");
        return EXIT_FAILURE;
    }
    return generateAndWriteAbstractTests(TestgenOptions::get(), *programInfo);
}

std::optional<AbstractTestList> Testgen::generateTests(std::string_view program,
                                                       const CompilerOptions &compilerOptions,
                                                       const TestgenOptions &testgenOptions) {
    try {
        return generateTestsImpl(program, compilerOptions, testgenOptions, false);
    } catch (const std::exception &e) {
        std::cerr << "Internal error: " << e.what() << "\n";
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
    return std::nullopt;
}

std::optional<AbstractTestList> Testgen::generateTests(const CompilerOptions &compilerOptions,
                                                       const TestgenOptions &testgenOptions) {
    try {
        return generateTestsImpl(std::nullopt, compilerOptions, testgenOptions, false);
    } catch (const std::exception &e) {
        std::cerr << "Internal error: " << e.what() << "\n";
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
    return std::nullopt;
}

int Testgen::writeTests(std::string_view program, const CompilerOptions &compilerOptions,
                        const TestgenOptions &testgenOptions) {
    try {
        if (generateTestsImpl(program, compilerOptions, testgenOptions, true).has_value()) {
            return EXIT_SUCCESS;
        }
    } catch (const std::exception &e) {
        std::cerr << "Internal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    } catch (...) {
        return EXIT_FAILURE;
    }
    return EXIT_FAILURE;
}

int Testgen::writeTests(const CompilerOptions &compilerOptions,
                        const TestgenOptions &testgenOptions) {
    try {
        if (generateTestsImpl(std::nullopt, compilerOptions, testgenOptions, true).has_value()) {
            return EXIT_SUCCESS;
        }
    } catch (const std::exception &e) {
        std::cerr << "Internal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    } catch (...) {
        return EXIT_FAILURE;
    }
    return EXIT_FAILURE;
}

}  // namespace p4c::P4Tools::P4Testgen
