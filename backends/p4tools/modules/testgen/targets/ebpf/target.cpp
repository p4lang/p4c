#include "backends/p4tools/modules/testgen/targets/ebpf/target.h"

#include <cstddef>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "ir/ir.h"
#include "ir/solver.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/compiler_target.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/program_info.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/test_backend.h"

namespace P4Tools::P4Testgen::EBPF {

/* =============================================================================================
 *  EBPFTestgenTarget implementation
 ============================================================================================= */

EBPFTestgenTarget::EBPFTestgenTarget() : TestgenTarget("ebpf", "ebpf") {}

void EBPFTestgenTarget::make() {
    static EBPFTestgenTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new EBPFTestgenTarget();
    }
}

const EBPFProgramInfo *EBPFTestgenTarget::produceProgramInfoImpl(
    const CompilerResult &compilerResult, const IR::Declaration_Instance *mainDecl) const {
    // The blocks in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of blocks, represented as constructor-call
    // expressions.
    const auto blocks =
        argumentsToTypeDeclarations(&compilerResult.getProgram(), mainDecl->arguments);

    // We should have six arguments.
    BUG_CHECK(blocks.size() == 2, "%1%: The EBPF architecture requires 2 blocks. Received %2%.",
              mainDecl, blocks.size());

    ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;
    for (size_t idx = 0; idx < blocks.size(); ++idx) {
        const auto *declType = blocks.at(idx);
        auto canonicalName = EBPFProgramInfo::ARCH_SPEC.getArchMember(idx)->blockName;
        programmableBlocks.emplace(canonicalName, declType);
    }

    // TODO: We bound the max packet size. However, eBPF should be able to support jumbo sized
    // packets. There might be a bug in the framework
    auto &testgenOptions = TestgenOptions::get();
    if (testgenOptions.maxPktSize > 12000) {
        ::warning("Max packet size %1% larger than 12000 bits. Bounding size to 12000 bits.",
                  testgenOptions.maxPktSize);
        testgenOptions.maxPktSize = 12000;
    }

    return new EBPFProgramInfo(*compilerResult.checkedTo<TestgenCompilerResult>(),
                               programmableBlocks);
}

EBPFTestBackend *EBPFTestgenTarget::getTestBackendImpl(
    const ProgramInfo &programInfo, const TestBackendConfiguration &testBackendConfiguration,
    SymbolicExecutor &symbex) const {
    return new EBPFTestBackend(programInfo, testBackendConfiguration, symbex);
}

EBPFCmdStepper *EBPFTestgenTarget::getCmdStepperImpl(ExecutionState &state, AbstractSolver &solver,
                                                     const ProgramInfo &programInfo) const {
    return new EBPFCmdStepper(state, solver, programInfo);
}

EBPFExprStepper *EBPFTestgenTarget::getExprStepperImpl(ExecutionState &state,
                                                       AbstractSolver &solver,
                                                       const ProgramInfo &programInfo) const {
    return new EBPFExprStepper(state, solver, programInfo);
}

}  // namespace P4Tools::P4Testgen::EBPF
