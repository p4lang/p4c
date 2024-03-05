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

#include "backends/p4tools/modules/testgen/core/compiler_target.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/targets/pna/concolic.h"
#include "backends/p4tools/modules/testgen/targets/pna/constants.h"

namespace P4Tools::P4Testgen::Pna {

PnaDpdkProgramInfo::PnaDpdkProgramInfo(
    const TestgenCompilerResult &compilerResult,
    ordered_map<cstring, const IR::Type_Declaration *> inputBlocks)
    : SharedPnaProgramInfo(compilerResult, std::move(inputBlocks)) {
    concolicMethodImpls.add(*PnaDpdkConcolic::getPnaDpdkConcolicMethodImpls());

    // Just concatenate everything together.
    // Iterate through the (ordered) pipes of the target architecture.
    const auto &archSpec = getArchSpec();
    const auto *programmableBlocks = getProgrammableBlocks();

    BUG_CHECK(archSpec.getArchVectorSize() == programmableBlocks->size(),
              "The PNA architecture requires %1% pipes (provided %2% pipes).",
              archSpec.getArchVectorSize(), programmableBlocks->size());

    /// Compute the series of nodes corresponding to the in-order execution of top-level
    /// pipeline-component instantiations. For a standard pna, this produces
    /// the main parser, the pre control, the main control, and finally
    /// the deparser. This sequence also includes nodes that handle transitions between the
    /// individual component instantiations.
    int pipeIdx = 0;

    for (const auto &declTuple : *programmableBlocks) {
        blockMap.emplace(declTuple.second->getName(), declTuple.first);
        // Iterate through the (ordered) pipes of the target architecture.
        auto subResult = processDeclaration(declTuple.second, pipeIdx);
        pipelineSequence.insert(pipelineSequence.end(), subResult.begin(), subResult.end());
        ++pipeIdx;
    }
}

const ArchSpec &PnaDpdkProgramInfo::getArchSpec() const { return ARCH_SPEC; }

std::vector<Continuation::Command> PnaDpdkProgramInfo::processDeclaration(
    const IR::Type_Declaration *typeDecl, size_t blockIdx) const {
    // Collect parameters.
    const auto *applyBlock = typeDecl->to<IR::IApply>();
    if (applyBlock == nullptr) {
        TESTGEN_UNIMPLEMENTED("Constructed type %s of type %s not supported.", typeDecl,
                              typeDecl->node_type_name());
    }
    // Retrieve the current canonical pipe in the architecture spec using the pipe index.
    const auto *archMember = getArchSpec().getArchMember(blockIdx);

    std::vector<Continuation::Command> cmds;
    // Copy-in.
    const auto *copyInCall = new IR::MethodCallStatement(Utils::generateInternalMethodCall(
        "copy_in", {new IR::StringLiteral(typeDecl->name)}, IR::Type_Void::get(),
        new IR::ParameterList(
            {new IR::Parameter("blockRef", IR::Direction::In, IR::Type_Unknown::get())})));
    cmds.emplace_back(copyInCall);
    // Insert the actual pipeline.
    cmds.emplace_back(typeDecl);
    // Copy-out.
    const auto *copyOutCall = new IR::MethodCallStatement(Utils::generateInternalMethodCall(
        "copy_out", {new IR::StringLiteral(typeDecl->name)}, IR::Type_Void::get(),
        new IR::ParameterList(
            {new IR::Parameter("blockRef", IR::Direction::In, IR::Type_Unknown::get())})));
    cmds.emplace_back(copyOutCall);

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

const ArchSpec PnaDpdkProgramInfo::ARCH_SPEC = ArchSpec(
    "PNA_NIC", {
                   // parser MainParserT<MH, MM>(
                   //     packet_in pkt,
                   //     //in    PM pre_user_meta,
                   //     out   MH main_hdr,
                   //     inout MM main_user_meta,
                   //     in    pna_main_parser_input_metadata_t istd);
                   {"MainParserT", {nullptr, "*main_hdr", "*main_user_meta", "*parser_istd"}},
                   // control PreControlT<PH, PM>(
                   //     in    PH pre_hdr,
                   //     inout PM pre_user_meta,
                   //     in    pna_pre_input_metadata_t  istd,
                   //     inout pna_pre_output_metadata_t ostd);
                   {"PreControlT", {"*main_hdr", "*main_user_meta", "*pre_istd", "*pre_ostd"}},
                   // control MainControlT<MH, MM>(
                   //     //in    PM pre_user_meta,
                   //     inout MH main_hdr,
                   //     inout MM main_user_meta,
                   //     in    pna_main_input_metadata_t  istd,
                   //     inout pna_main_output_metadata_t ostd);
                   {"MainControlT", {"*main_hdr", "*main_user_meta", "*main_istd", "*ostd"}},
                   // control MainDeparserT<MH, MM>(
                   //     packet_out pkt,
                   //     in    MH main_hdr,
                   //     in    MM main_user_meta,
                   //     in    pna_main_output_metadata_t ostd);
                   {"MainDeparserT", {nullptr, "*main_hdr", "*main_user_meta", "*ostd"}},
               });

}  // namespace P4Tools::P4Testgen::Pna
