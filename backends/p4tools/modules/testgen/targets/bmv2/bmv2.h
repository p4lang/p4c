#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BMV2_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BMV2_H_

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "backends/p4tools/common/lib/variables.h"
#include "control-plane/p4RuntimeSerializer.h"
#include "frontends/common/options.h"

#include "backends/p4tools/modules/testgen/core/compiler_target.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/map_direct_externs.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/p4_asserts_parser.h"

namespace P4Tools::P4Testgen::Bmv2 {

/// Extends the CompilerResult with information specific to the V1Model running on BMv2.
class BMv2V1ModelCompilerResult : public TestgenCompilerResult {
 private:
    /// The P4RuntimeAPI inferred from this particular BMv2 V1Model P4 program.
    P4::P4RuntimeAPI p4runtimeApi;

    /// Map of direct extern declarations which are attached to a table.
    DirectExternMap directExternMap;

    // Vector containing vectors of P4Constraints restrictions and nodes to which these restrictions
    // apply.
    ConstraintsVector p4ConstraintsRestrictions;

 public:
    explicit BMv2V1ModelCompilerResult(TestgenCompilerResult compilerResult,
                                       P4::P4RuntimeAPI p4runtimeApi,
                                       DirectExternMap directExternMap,
                                       ConstraintsVector p4ConstraintsRestrictions);

    /// @returns the P4RuntimeAPI inferred from this particular BMv2 V1Model P4 program.
    [[nodiscard]] const P4::P4RuntimeAPI &getP4RuntimeApi() const;

    /// @returns the vector of pairs of P4Constraints restrictions and nodes to which these
    // apply.
    [[nodiscard]] ConstraintsVector getP4ConstraintsRestrictions() const;

    /// @returns the map of direct extern declarations which are attached to a table.
    [[nodiscard]] const DirectExternMap &getDirectExternMap() const;

    DECLARE_TYPEINFO(BMv2V1ModelCompilerResult, TestgenCompilerResult);
};

class Bmv2V1ModelCompilerTarget : public TestgenCompilerTarget {
 public:
    /// Registers this target.
    static void make();

 private:
    [[nodiscard]] MidEnd mkMidEnd(const CompilerOptions &options) const override;

    CompilerResultOrError runCompilerImpl(const IR::P4Program *program) const override;

    Bmv2V1ModelCompilerTarget();
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_BMV2_H_ */
