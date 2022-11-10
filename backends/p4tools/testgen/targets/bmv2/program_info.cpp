#include "backends/p4tools/testgen/targets/bmv2/program_info.h"

#include <list>
#include <map>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

#include "backends/p4tools/testgen//lib/exceptions.h"
#include "backends/p4tools/testgen/core/target.h"
#include "backends/p4tools/testgen/lib/concolic.h"
#include "backends/p4tools/testgen/targets/bmv2/concolic.h"

namespace P4Tools {

namespace P4Testgen {

namespace Bmv2 {

const IR::Type_Bits BMv2_V1ModelProgramInfo::parserErrBits = IR::Type_Bits(32, false);

BMv2_V1ModelProgramInfo::BMv2_V1ModelProgramInfo(
    const IR::P4Program* program, ordered_map<cstring, const IR::Type_Declaration*> inputBlocks,
    const std::map<int, int> declIdToGress)
    : ProgramInfo(program),
      programmableBlocks(std::move(inputBlocks)),
      declIdToGress(declIdToGress) {
    concolicMethodImpls.add(*Bmv2Concolic::getBmv2ConcolicMethodImpls());

    // Just concatenate everything together.
    // Iterate through the (ordered) pipes of the target architecture.
    const auto* archSpec = TestgenTarget::getArchSpec();
    BUG_CHECK(archSpec->getArchVectorSize() == programmableBlocks.size(),
              "The BMV2 architecture requires %1% pipes (provided %2% pipes).",
              archSpec->getArchVectorSize(), programmableBlocks.size());

    /// Compute the series of nodes corresponding to the in-order execution of top-level
    /// pipeline-component instantiations. For a standard v1model, this produces
    /// the parser, the checksum verifier, the MAU pipeline, the checksum calculator, and finally
    /// the deparser. This sequence also includes nodes that handle transitions between the
    /// individual component instantiations.
    int pipeIdx = 0;
    for (const auto& declTuple : programmableBlocks) {
        // Iterate through the (ordered) pipes of the target architecture.
        auto subResult = processDeclaration(declTuple.second, pipeIdx);
        pipelineSequence.insert(pipelineSequence.end(), subResult.begin(), subResult.end());
        ++pipeIdx;
    }
    // Sending a too short packet in BMV2 produces nonsense, so we require the packet size to be
    // larger than 32 bits
    targetConstraints =
        new IR::Grt(IR::Type::Boolean::get(), ExecutionState::getInputPacketSizeVar(),
                    IR::getConstant(ExecutionState::getPacketSizeVarType(), 32));
}

const ordered_map<cstring, const IR::Type_Declaration*>*
BMv2_V1ModelProgramInfo::getProgrammableBlocks() const {
    return &programmableBlocks;
}

int BMv2_V1ModelProgramInfo::getGress(const IR::Type_Declaration* decl) const {
    return declIdToGress.at(decl->declid);
}

std::vector<Continuation::Command> BMv2_V1ModelProgramInfo::processDeclaration(
    const IR::Type_Declaration* typeDecl, size_t blockIdx) const {
    // Get the architecture specification for this target.
    const auto* archSpec = TestgenTarget::getArchSpec();

    // Collect parameters.
    const auto* applyBlock = typeDecl->to<IR::IApply>();
    if (applyBlock == nullptr) {
        TESTGEN_UNIMPLEMENTED("Constructed type %s of type %s not supported.", typeDecl,
                              typeDecl->node_type_name());
    }
    const auto* params = applyBlock->getApplyParameters();
    // Retrieve the current canonical pipe in the architecture spec using the pipe index.
    const auto* archMember = archSpec->getArchMember(blockIdx);

    std::vector<Continuation::Command> cmds;
    // Generate all the necessary copy-in/outs.
    std::vector<Continuation::Command> copyOuts;
    for (size_t paramIdx = 0; paramIdx < params->size(); ++paramIdx) {
        const auto* param = params->getParameter(paramIdx);
        produceCopyInOutCall(param, paramIdx, archMember, &cmds, &copyOuts);
    }
    // Insert the actual pipeline.
    cmds.emplace_back(typeDecl);
    // Add the copy out assignments after the pipe has completed executing.
    cmds.insert(cmds.end(), copyOuts.begin(), copyOuts.end());

    // Update some metadata variables for egress processing after we are done with Ingress
    // processing. For example, the egress port.
    if ((archMember->blockName == "Ingress")) {
        auto* egressPortVar =
            new IR::Member(IR::getBitType(TestgenTarget::getPortNumWidth_bits()),
                           new IR::PathExpression("*standard_metadata"), "egress_port");
        auto* portStmt = new IR::AssignmentStatement(egressPortVar, getTargetOutputPortVar());
        cmds.emplace_back(portStmt);
    }
    // After some specific pipelines (deparsers), we have to append the remaining packet
    // payload. We use an extern call for this.
    if ((archMember->blockName == "Deparser")) {
        const auto* stmt = new IR::MethodCallStatement(
            Utils::generateInternalMethodCall("prepend_emit_buffer", {}));
        cmds.emplace_back(stmt);
        // Also check whether we need to drop the packet.
        auto* dropStmt =
            new IR::MethodCallStatement(Utils::generateInternalMethodCall("drop_and_exit", {}));
        const auto* dropCheck = new IR::IfStatement(dropIsActive(), dropStmt, nullptr);
        cmds.emplace_back(dropCheck);
        const auto* recirculateCheck =
            new IR::MethodCallStatement(Utils::generateInternalMethodCall("check_recirculate", {}));
        cmds.emplace_back(recirculateCheck);
    }
    return cmds;
}

const IR::Member* BMv2_V1ModelProgramInfo::getTargetInputPortVar() const {
    return new IR::Member(IR::getBitType(TestgenTarget::getPortNumWidth_bits()),
                          new IR::PathExpression("*standard_metadata"), "ingress_port");
}

const IR::Member* BMv2_V1ModelProgramInfo::getTargetOutputPortVar() const {
    return new IR::Member(IR::getBitType(TestgenTarget::getPortNumWidth_bits()),
                          new IR::PathExpression("*standard_metadata"), "egress_spec");
}

const IR::Expression* BMv2_V1ModelProgramInfo::dropIsActive() const {
    const auto* egressPortVar = getTargetOutputPortVar();
    return new IR::Equ(IR::getConstant(egressPortVar->type, 511), egressPortVar);
}

const IR::Expression* BMv2_V1ModelProgramInfo::createTargetUninitialized(const IR::Type* type,
                                                                         bool forceTaint) const {
    if (forceTaint) {
        return Utils::getTaintExpression(type);
    }
    return IR::getDefaultValue(type);
}

const IR::Type_Bits* BMv2_V1ModelProgramInfo::getParserErrorType() const { return &parserErrBits; }

const IR::PathExpression* BMv2_V1ModelProgramInfo::getBlockParam(cstring blockLabel,
                                                                 size_t paramIndex) const {
    // Retrieve the block and get the parameter type.
    // TODO: This should be necessary, we should be able to this using only the arch spec.
    // TODO: Make this more general and lift it into program_info core.
    const auto* programmableBlocks = getProgrammableBlocks();
    const auto* typeDecl = programmableBlocks->at(blockLabel);
    const auto* applyBlock = typeDecl->to<IR::IApply>();
    CHECK_NULL(applyBlock);
    const auto* params = applyBlock->getApplyParameters();
    const auto* param = params->getParameter(paramIndex);
    const auto* paramType = param->type;
    // For convenience, resolve type names.
    if (const auto* tn = paramType->to<IR::Type_Name>()) {
        paramType = resolveProgramType(tn);
    }

    const auto* archSpec = TestgenTarget::getArchSpec();
    auto archIndex = archSpec->getBlockIndex(blockLabel);
    auto archRef = archSpec->getParamName(archIndex, paramIndex);
    return new IR::PathExpression(paramType, new IR::Path(archRef));
}

const IR::Member* BMv2_V1ModelProgramInfo::getParserParamVar(const IR::P4Parser* parser,
                                                             const IR::Type* type,
                                                             size_t paramIndex,
                                                             cstring paramLabel) const {
    // If the optional parser parameter is not present, write directly to the
    // global parser metadata state. Otherwise, we retrieve the parameter name.
    auto parserApplyParams = parser->getApplyParameters()->parameters;
    cstring structLabel = nullptr;
    if (parserApplyParams.size() > paramIndex) {
        const auto* paramString = parser->getApplyParameters()->parameters.at(paramIndex);
        structLabel = paramString->name;
    } else {
        const auto* archSpec = TestgenTarget::getArchSpec();
        structLabel = archSpec->getParamName("Parser", paramIndex);
    }
    return new IR::Member(type, new IR::PathExpression(structLabel), paramLabel);
}

}  // namespace Bmv2

}  // namespace P4Testgen

}  // namespace P4Tools
