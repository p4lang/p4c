#include "backends/p4tools/modules/testgen/core/compiler_result.h"

#include <utility>

#include "backends/p4tools/common/compiler/reachability.h"
#include "midend/coverage.h"

namespace p4c::P4Tools::P4Testgen {

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

}  // namespace p4c::P4Tools::P4Testgen
