#include "backends/p4tools/modules/testgen/targets/bmv2/target.h"

#include <cstddef>
#include <map>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "ir/ir.h"
#include "ir/solver.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/bmv2.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/constants.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/program_info.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend.h"

namespace P4Tools::P4Testgen::Bmv2 {

/* =============================================================================================
 *  Bmv2V1ModelTestgenTarget implementation
 * ============================================================================================= */

Bmv2V1ModelTestgenTarget::Bmv2V1ModelTestgenTarget() : TestgenTarget("bmv2", "v1model") {}

void Bmv2V1ModelTestgenTarget::make() {
    static Bmv2V1ModelTestgenTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new Bmv2V1ModelTestgenTarget();
    }
}

const Bmv2V1ModelProgramInfo *Bmv2V1ModelTestgenTarget::produceProgramInfoImpl(
    const CompilerResult &compilerResult, const IR::Declaration_Instance *mainDecl) const {
    // The blocks in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of blocks, represented as constructor-call
    // expressions.
    const auto blocks =
        argumentsToTypeDeclarations(&compilerResult.getProgram(), mainDecl->arguments);

    // We should have six arguments.
    BUG_CHECK(blocks.size() == 6, "%1%: The BMV2 architecture requires 6 pipes. Received %2%.",
              mainDecl, blocks.size());

    ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;
    std::map<int, int> declIdToGress;

    // Add to parserDeclIdToGress, mauDeclIdToGress, and deparserDeclIdToGress.
    for (size_t idx = 0; idx < blocks.size(); ++idx) {
        const auto *declType = blocks.at(idx);

        auto canonicalName = Bmv2V1ModelProgramInfo::ARCH_SPEC.getArchMember(idx)->blockName;
        programmableBlocks.emplace(canonicalName, declType);

        if (idx < 3) {
            declIdToGress[declType->declid] = BMV2_INGRESS;
        } else {
            declIdToGress[declType->declid] = BMV2_EGRESS;
        }
    }

    return new Bmv2V1ModelProgramInfo(*compilerResult.checkedTo<BMv2V1ModelCompilerResult>(),
                                      programmableBlocks, declIdToGress);
}

Bmv2TestBackend *Bmv2V1ModelTestgenTarget::getTestBackendImpl(
    const ProgramInfo &programInfo, const TestBackendConfiguration &testBackendConfiguration,
    SymbolicExecutor &symbex) const {
    return new Bmv2TestBackend(*programInfo.checkedTo<Bmv2V1ModelProgramInfo>(),
                               testBackendConfiguration, symbex);
}

Bmv2V1ModelCmdStepper *Bmv2V1ModelTestgenTarget::getCmdStepperImpl(
    ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo) const {
    return new Bmv2V1ModelCmdStepper(state, solver, programInfo);
}

Bmv2V1ModelExprStepper *Bmv2V1ModelTestgenTarget::getExprStepperImpl(
    ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo) const {
    return new Bmv2V1ModelExprStepper(state, solver, programInfo);
}

}  // namespace P4Tools::P4Testgen::Bmv2
