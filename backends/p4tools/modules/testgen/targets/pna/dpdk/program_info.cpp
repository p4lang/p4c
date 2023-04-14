#include "backends/p4tools/modules/testgen/targets/pna/dpdk/program_info.h"

#include <list>
#include <utility>
#include <variant>
#include <vector>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/targets/pna/concolic.h"
#include "backends/p4tools/modules/testgen/targets/pna/constants.h"

namespace P4Tools::P4Testgen::Pna {

PnaDpdkProgramInfo::PnaDpdkProgramInfo(
    const IR::P4Program *program, ordered_map<cstring, const IR::Type_Declaration *> inputBlocks)
    : SharedPnaProgramInfo(program, std::move(inputBlocks)) {
    concolicMethodImpls.add(*PnaDpdkConcolic::getPnaDpdkConcolicMethodImpls());

    // Just concatenate everything together.
    // Iterate through the (ordered) pipes of the target architecture.
    const auto *archSpec = TestgenTarget::getArchSpec();
    const auto *programmableBlocks = getProgrammableBlocks();

    BUG_CHECK(archSpec->getArchVectorSize() == programmableBlocks->size(),
              "The PNA architecture requires %1% pipes (provided %2% pipes).",
              archSpec->getArchVectorSize(), programmableBlocks->size());

    /// Compute the series of nodes corresponding to the in-order execution of top-level
    /// pipeline-component instantiations. For a standard pna, this produces
    /// the main parser, the pre control, the main control, and finally
    /// the deparser. This sequence also includes nodes that handle transitions between the
    /// individual component instantiations.
    int pipeIdx = 0;

    for (const auto &declTuple : *programmableBlocks) {
        // Iterate through the (ordered) pipes of the target architecture.
        auto subResult = processDeclaration(declTuple.second, pipeIdx);
        pipelineSequence.insert(pipelineSequence.end(), subResult.begin(), subResult.end());
        ++pipeIdx;
    }
}

std::vector<Continuation::Command> PnaDpdkProgramInfo::processDeclaration(
    const IR::Type_Declaration *typeDecl, size_t blockIdx) const {
    // Get the architecture specification for this target.
    const auto *archSpec = TestgenTarget::getArchSpec();

    // Collect parameters.
    const auto *applyBlock = typeDecl->to<IR::IApply>();
    if (applyBlock == nullptr) {
        TESTGEN_UNIMPLEMENTED("Constructed type %s of type %s not supported.", typeDecl,
                              typeDecl->node_type_name());
    }
    const auto *params = applyBlock->getApplyParameters();
    // Retrieve the current canonical pipe in the architecture spec using the pipe index.
    const auto *archMember = archSpec->getArchMember(blockIdx);

    std::vector<Continuation::Command> cmds;
    // Generate all the necessary copy-in/outs.
    std::vector<Continuation::Command> copyOuts;
    for (size_t paramIdx = 0; paramIdx < params->size(); ++paramIdx) {
        const auto *param = params->getParameter(paramIdx);
        produceCopyInOutCall(param, paramIdx, archMember, &cmds, &copyOuts);
    }
    // Insert the actual pipeline.
    cmds.emplace_back(typeDecl);
    // Add the copy out assignments after the pipe has completed executing.
    cmds.insert(cmds.end(), copyOuts.begin(), copyOuts.end());

    auto *dropStmt =
        new IR::MethodCallStatement(Utils::generateInternalMethodCall("drop_and_exit", {}));

    if ((archMember->blockName == "MainParserT")) {
        // After the parser has set the parser error, assign it to the two metadatas that contain an
        // error.
        const auto *preIstdParserErrAssign = new IR::AssignmentStatement(
            new IR::Member(getParserErrorType(), new IR::PathExpression("*pre_istd"),
                           "parser_error"),
            &PnaConstants::PARSER_ERROR);
        cmds.emplace_back(preIstdParserErrAssign);
        const auto *mainIstdParserErrAssign = new IR::AssignmentStatement(
            new IR::Member(getParserErrorType(), new IR::PathExpression("*main_istd"),
                           "parser_error"),
            &PnaConstants::PARSER_ERROR);
        cmds.emplace_back(mainIstdParserErrAssign);
    }
    // After some specific pipelines (deparsers), we have to append the remaining packet
    // payload. We use an extern call for this.
    if ((archMember->blockName == "MainDeparserT")) {
        const auto *stmt = new IR::MethodCallStatement(
            Utils::generateInternalMethodCall("prepend_emit_buffer", {}));
        cmds.emplace_back(stmt);
        // Also check whether we need to drop the packet.
        const auto *dropCheck = new IR::IfStatement(dropIsActive(), dropStmt, nullptr);
        cmds.emplace_back(dropCheck);
    }
    return cmds;
}

}  // namespace P4Tools::P4Testgen::Pna
