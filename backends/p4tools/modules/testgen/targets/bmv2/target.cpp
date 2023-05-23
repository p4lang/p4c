#include "backends/p4tools/modules/testgen/targets/bmv2/target.h"

#include <cstddef>
#include <map>
#include <vector>

#include "backends/p4tools/common/core/solver.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
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

const Bmv2V1ModelProgramInfo *Bmv2V1ModelTestgenTarget::initProgramImpl(
    const IR::P4Program *program, const IR::Declaration_Instance *mainDecl) const {
    // The blocks in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of blocks, represented as constructor-call
    // expressions.
    std::vector<const IR::Type_Declaration *> blocks;
    argumentsToTypeDeclarations(program, mainDecl->arguments, blocks);

    // We should have six arguments.
    BUG_CHECK(blocks.size() == 6, "%1%: The BMV2 architecture requires 6 pipes. Received %2%.",
              mainDecl, blocks.size());

    ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;
    std::map<int, int> declIdToGress;

    // Add to parserDeclIdToGress, mauDeclIdToGress, and deparserDeclIdToGress.
    for (size_t idx = 0; idx < blocks.size(); ++idx) {
        const auto *declType = blocks.at(idx);

        auto canonicalName = ARCH_SPEC.getArchMember(idx)->blockName;
        programmableBlocks.emplace(canonicalName, declType);

        if (idx < 3) {
            declIdToGress[declType->declid] = BMV2_INGRESS;
        } else {
            declIdToGress[declType->declid] = BMV2_EGRESS;
        }
    }

    return new Bmv2V1ModelProgramInfo(program, programmableBlocks, declIdToGress);
}

Bmv2TestBackend *Bmv2V1ModelTestgenTarget::getTestBackendImpl(
    const ProgramInfo &programInfo, SymbolicExecutor &symbex,
    const std::filesystem::path &testPath) const {
    return new Bmv2TestBackend(programInfo, symbex, testPath);
}

int Bmv2V1ModelTestgenTarget::getPortNumWidthBitsImpl() const { return 9; }

Bmv2V1ModelCmdStepper *Bmv2V1ModelTestgenTarget::getCmdStepperImpl(
    ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo) const {
    return new Bmv2V1ModelCmdStepper(state, solver, programInfo);
}

Bmv2V1ModelExprStepper *Bmv2V1ModelTestgenTarget::getExprStepperImpl(
    ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo) const {
    return new Bmv2V1ModelExprStepper(state, solver, programInfo);
}

const ArchSpec Bmv2V1ModelTestgenTarget::ARCH_SPEC =
    ArchSpec("V1Switch", {// parser Parser<H, M>(packet_in b,
                          //                     out H parsedHdr,
                          //                     inout M meta,
                          //                     inout standard_metadata_t standard_metadata);
                          {"Parser", {nullptr, "*hdr", "*meta", "*standard_metadata"}},
                          // control VerifyChecksum<H, M>(inout H hdr,
                          //                              inout M meta);
                          {"VerifyChecksum", {"*hdr", "*meta"}},
                          // control Ingress<H, M>(inout H hdr,
                          //                       inout M meta,
                          //                       inout standard_metadata_t standard_metadata);
                          {"Ingress", {"*hdr", "*meta", "*standard_metadata"}},
                          // control Egress<H, M>(inout H hdr,
                          //            inout M meta,
                          //            inout standard_metadata_t standard_metadata);
                          {"Egress", {"*hdr", "*meta", "*standard_metadata"}},
                          // control ComputeChecksum<H, M>(inout H hdr,
                          //                       inout M meta);
                          {"ComputeChecksum", {"*hdr", "*meta"}},
                          // control Deparser<H>(packet_out b, in H hdr);
                          {"Deparser", {nullptr, "*hdr"}}});

const ArchSpec *Bmv2V1ModelTestgenTarget::getArchSpecImpl() const { return &ARCH_SPEC; }

}  // namespace P4Tools::P4Testgen::Bmv2
