#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_COMPILER_RESULT_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_COMPILER_RESULT_H_

#include "backends/p4tools/modules/testgen/core/compiler_result.h"
#include "backends/p4tools/modules/testgen/targets/tofino/map_direct_externs.h"

namespace P4::P4Tools::P4Testgen::Tofino {

/// Extends the CompilerResult with information specific to the V1Model running on BMv2.
class TofinoCompilerResult : public TestgenCompilerResult {
 private:
    /// The map of direct extern declarations which are attached to a table.
    DirectExternMap directExternMap;

 public:
    explicit TofinoCompilerResult(TestgenCompilerResult compilerResult,
                                  DirectExternMap directExternMap);

    /// @returns the map of direct extern declarations which are attached to a table.
    [[nodiscard]] const DirectExternMap &getDirectExternMap() const;

    DECLARE_TYPEINFO(TofinoCompilerResult, TestgenCompilerResult);
};

}  // namespace P4::P4Tools::P4Testgen::Tofino

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_TOFINO_COMPILER_RESULT_H_ */
