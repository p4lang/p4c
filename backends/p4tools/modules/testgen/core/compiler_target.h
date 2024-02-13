#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_COMPILER_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_COMPILER_TARGET_H_

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/reachability.h"
#include "midend/coverage.h"

namespace P4Tools::P4Testgen {

/// Extends the CompilerResult with the associated P4RuntimeApi
class TestgenCompilerResult : public CompilerResult {
 private:
    /// The coverabled Nodes in the analyzed P4 program.
    P4::Coverage::CoverageSet coverableNodes;

    /// The call graph of the analyzed P4 program, if flag --dcg is set.
    const NodesCallGraph *callGraph;

 public:
    explicit TestgenCompilerResult(CompilerResult compilerResult,
                                   P4::Coverage::CoverageSet coverableNodes,
                                   const NodesCallGraph *callGraph = nullptr);

    /// @returns the call graph of the analyzed P4 program, if flag --dcg is set.
    /// If this function is called when the call graph is not set, if will throw an exception.
    /// TODO: Replace this with std::nullopt?
    [[nodiscard]] const NodesCallGraph &getCallGraph() const;

    /// @returns the coverable nodes in the analyzed P4 program.
    [[nodiscard]] const P4::Coverage::CoverageSet &getCoverableNodes() const;

    DECLARE_TYPEINFO(TestgenCompilerResult, CompilerResult);
};

class TestgenCompilerTarget : public CompilerTarget {
 protected:
    explicit TestgenCompilerTarget(std::string deviceName, std::string archName);

 private:
    CompilerResultOrError runCompilerImpl(const IR::P4Program *program) const override;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_COMPILER_TARGET_H_ */
