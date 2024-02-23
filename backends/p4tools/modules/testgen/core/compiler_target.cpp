#include "backends/p4tools/modules/testgen/core/compiler_target.h"

#include <utility>

#include "backends/p4tools/common/compiler/reachability.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

TestgenCompilerResult::TestgenCompilerResult(CompilerResult compilerResult,
                                             P4::Coverage::CoverageSet coverableNodes,
                                             const NodesCallGraph *callGraph)
    : CompilerResult(std::move(compilerResult)),
      coverableNodes(std::move(coverableNodes)),
      callGraph(callGraph) {}

const NodesCallGraph &TestgenCompilerResult::getCallGraph() const {
    BUG_CHECK(callGraph != nullptr, "The call graph has not been initialized.");
    return *callGraph;
}

const P4::Coverage::CoverageSet &TestgenCompilerResult::getCoverableNodes() const {
    return coverableNodes;
}

TestgenCompilerTarget::TestgenCompilerTarget(std::string deviceName, std::string archName)
    : CompilerTarget(std::move(deviceName), std::move(archName)) {}

CompilerResultOrError TestgenCompilerTarget::runCompilerImpl(const IR::P4Program *program) const {
    program = runFrontend(program);
    if (program == nullptr) {
        return std::nullopt;
    }

    program = runMidEnd(program);
    if (program == nullptr) {
        return std::nullopt;
    }

    // Create DCG.
    NodesCallGraph *dcg = nullptr;
    if (TestgenOptions::get().dcg || !TestgenOptions::get().pattern.empty()) {
        dcg = new NodesCallGraph("NodesCallGraph");
        P4ProgramDCGCreator dcgCreator(dcg);
        program->apply(dcgCreator);
    }

    /// Collect coverage information about the program.
    auto coverage = P4::Coverage::CollectNodes(TestgenOptions::get().coverageOptions);
    program->apply(coverage);

    return {
        *new TestgenCompilerResult(CompilerResult(*program), coverage.getCoverableNodes(), dcg)};
}

}  // namespace P4Tools::P4Testgen
