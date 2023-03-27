#include "backends/p4tools/modules/testgen/targets/bmv2/target.h"

#include <stddef.h>

#include <map>
#include <string>
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
#include "backends/p4tools/modules/testgen/lib/namespace_context.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/constants.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/expr_stepper.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/program_info.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

/* =============================================================================================
 *  BMv2_V1ModelTestgenTarget implementation
 * ============================================================================================= */

BMv2_V1ModelTestgenTarget::BMv2_V1ModelTestgenTarget() : TestgenTarget("bmv2", "v1model") {}

void BMv2_V1ModelTestgenTarget::make() {
    static BMv2_V1ModelTestgenTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new BMv2_V1ModelTestgenTarget();
    }
}

const BMv2_V1ModelProgramInfo *BMv2_V1ModelTestgenTarget::initProgram_impl(
    const IR::P4Program *program, const IR::Declaration_Instance *mainDecl) const {
    // The blocks in the main declaration are just the arguments in the constructor call.
    // Convert mainDecl->arguments into a vector of blocks, represented as constructor-call
    // expressions.
    const auto *ns = NamespaceContext::Empty->push(program);
    std::vector<const IR::Type_Declaration *> blocks;
    argumentsToTypeDeclarations(ns, mainDecl->arguments, blocks);

    // We should have six arguments.
    BUG_CHECK(blocks.size() == 6, "%1%: The BMV2 architecture requires 6 pipes. Received %2%.",
              mainDecl, blocks.size());

    ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;
    std::map<int, int> declIdToGress;

    // Add to parserDeclIdToGress, mauDeclIdToGress, and deparserDeclIdToGress.
    for (size_t idx = 0; idx < blocks.size(); ++idx) {
        const auto *declType = blocks.at(idx);

        auto canonicalName = archSpec.getArchMember(idx)->blockName;
        programmableBlocks.emplace(canonicalName, declType);

        if (idx < 3) {
            declIdToGress[declType->declid] = BMV2_INGRESS;
        } else {
            declIdToGress[declType->declid] = BMV2_EGRESS;
        }
    }

    return new BMv2_V1ModelProgramInfo(program, programmableBlocks, declIdToGress);
}

Bmv2TestBackend *BMv2_V1ModelTestgenTarget::getTestBackend_impl(
    const ProgramInfo &programInfo, SymbolicExecutor &symbex, const std::filesystem::path &testPath,
    boost::optional<uint32_t> seed) const {
    return new Bmv2TestBackend(programInfo, symbex, testPath, seed);
}

int BMv2_V1ModelTestgenTarget::getPortNumWidth_bits_impl() const { return 9; }

BMv2_V1ModelCmdStepper *BMv2_V1ModelTestgenTarget::getCmdStepper_impl(
    ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo) const {
    return new BMv2_V1ModelCmdStepper(state, solver, programInfo);
}

BMv2_V1ModelExprStepper *BMv2_V1ModelTestgenTarget::getExprStepper_impl(
    ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo) const {
    return new BMv2_V1ModelExprStepper(state, solver, programInfo);
}

const ArchSpec BMv2_V1ModelTestgenTarget::archSpec =
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

const ArchSpec *BMv2_V1ModelTestgenTarget::getArchSpecImpl() const { return &archSpec; }

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools
