#include "backends/p4tools/testgen/targets/bmv2/program_info.h"

#include <list>
#include <map>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/ir.h"
#include "ir/ir.h"
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
                    IRUtils::getConstant(ExecutionState::getPacketSizeVarType(), 32));
}

const ordered_map<cstring, const IR::Type_Declaration*>*
BMv2_V1ModelProgramInfo::getProgrammableBlocks() {
    return &programmableBlocks;
}

int BMv2_V1ModelProgramInfo::getGress(const IR::P4Parser* parser) const {
    return declIdToGress.at(parser->declid);
}

std::vector<Continuation::Command> BMv2_V1ModelProgramInfo::processDeclaration(
    const IR::Type_Declaration* declType, size_t pipeIdx) const {
    auto result = std::vector<Continuation::Command>();
    // Get the architecture specification for this target.
    const auto* archSpec = TestgenTarget::getArchSpec();
    const IR::ParameterList* params = nullptr;

    // Collect parameters.
    if (const auto* p4parser = declType->to<IR::P4Parser>()) {
        params = p4parser->getApplyParameters();
    } else if (const auto* p4control = declType->to<IR::P4Control>()) {
        params = p4control->getApplyParameters();
    } else {
        TESTGEN_UNIMPLEMENTED("Constructed type %s of type %s not supported.", declType,
                              declType->node_type_name());
    }
    std::vector<const IR::Statement*> copyOuts;
    // Retrieve the current canonical pipe in the architecture spec using the pipe index.
    const auto* archMember = archSpec->getArchMember(pipeIdx);
    for (size_t paramIdx = 0; paramIdx < params->size(); ++paramIdx) {
        const auto* param = params->getParameter(paramIdx);
        const auto* paramType = param->type;
        // We need to resolve type names.
        if (const auto* tn = paramType->to<IR::Type_Name>()) {
            paramType = resolveProgramType(tn);
        }
        // Retrieve the identifier of the global architecture map using the parameter index.
        auto archRef = archMember->blockParams.at(paramIdx);
        // If the archRef is a nullptr or empty, we do not have a mapping for this parameter.
        if (archRef.isNullOrEmpty()) {
            continue;
        }
        const auto* archPath = new IR::PathExpression(paramType, new IR::Path(archRef));
        const auto* paramRef = new IR::PathExpression(paramType, new IR::Path(param->name));
        const auto* paramDir = new IR::StringLiteral(directionToString(param->direction));
        // This mimicks the copy-in from the architecture environment.
        const auto* copyInCall = new IR::MethodCallStatement(IRUtils::generateInternalMethodCall(
            "copy_in", {archPath, paramRef, paramDir, new IR::BoolLiteral(false)}));
        result.emplace_back(copyInCall);
        // This mimicks the copy-out from the architecture environment.
        const auto* copyOutCall = new IR::MethodCallStatement(
            IRUtils::generateInternalMethodCall("copy_out", {archPath, paramRef, paramDir}));
        copyOuts.emplace_back(copyOutCall);
    }
    // Insert the actual pipeline.
    result.emplace_back(declType);
    // Add the copy out assignments after the pipe has completed executing.
    result.insert(result.end(), copyOuts.begin(), copyOuts.end());
    // After some specific pipelines (deparsers), we have to append the remaining packet
    // payload. We use an extern call for this.
    if ((archMember->blockName == "Deparser")) {
        const auto* stmt = new IR::MethodCallStatement(
            IRUtils::generateInternalMethodCall("prepend_emit_buffer", {}));
        result.emplace_back(stmt);
        // Also check whether we need to drop the packet.
        auto* dropStmt =
            new IR::MethodCallStatement(IRUtils::generateInternalMethodCall("drop_and_exit", {}));
        const auto* dropCheck = new IR::IfStatement(dropIsActive(), dropStmt, nullptr);
        result.emplace_back(dropCheck);
        const auto* recirculateCheck = new IR::MethodCallStatement(
            IRUtils::generateInternalMethodCall("check_recirculate", {}));
        result.emplace_back(recirculateCheck);
    }
    return result;
}

const IR::Member* BMv2_V1ModelProgramInfo::getTargetInputPortVar() const {
    return new IR::Member(IRUtils::getBitType(TestgenTarget::getPortNumWidth_bits()),
                          new IR::PathExpression("*standard_metadata"), "ingress_port");
}

const IR::Member* BMv2_V1ModelProgramInfo::getTargetOutputPortVar() const {
    return new IR::Member(IRUtils::getBitType(TestgenTarget::getPortNumWidth_bits()),
                          new IR::PathExpression("*standard_metadata"), "egress_spec");
}

const IR::Expression* BMv2_V1ModelProgramInfo::dropIsActive() const {
    const auto* egressPortVar = getTargetOutputPortVar();
    return new IR::Equ(IRUtils::getConstant(egressPortVar->type, 511), egressPortVar);
}

const IR::Expression* BMv2_V1ModelProgramInfo::createTargetUninitialized(const IR::Type* type,
                                                                         bool forceTaint) const {
    if (forceTaint) {
        return IRUtils::getTaintExpression(type);
    }
    return IRUtils::getDefaultValue(type);
}

const IR::Type_Bits* BMv2_V1ModelProgramInfo::getParserErrorType() const { return &parserErrBits; }

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
