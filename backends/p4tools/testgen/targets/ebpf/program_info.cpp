#include "backends/p4tools/testgen/targets/ebpf/program_info.h"

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
#include "backends/p4tools/testgen/targets/ebpf/concolic.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

const IR::Type_Bits EBPFProgramInfo::parserErrBits = IR::Type_Bits(32, false);

EBPFProgramInfo::EBPFProgramInfo(const IR::P4Program* program,
                                 ordered_map<cstring, const IR::Type_Declaration*> inputBlocks)
    : ProgramInfo(program), programmableBlocks(std::move(inputBlocks)) {
    concolicMethodImpls.add(*EBPFConcolic::getEBPFConcolicMethodImpls());

    // Just concatenate everything together.
    // Iterate through the (ordered) pipes of the target architecture.
    const auto* archSpec = TestgenTarget::getArchSpec();
    BUG_CHECK(archSpec->getArchVectorSize() == programmableBlocks.size(),
              "The eBPF architecture requires %1% pipes (provided %2% pipes).",
              archSpec->getArchVectorSize(), programmableBlocks.size());

    /// Compute the series of nodes corresponding to the in-order execution of top-level
    /// pipeline-component instantiations. For a standard ebpf_model, this produces
    /// the parser and the filter.
    /// This sequence also includes nodes that handle transitions between the individual component
    /// instantiations.
    int blockIdx = 0;
    for (const auto& declTuple : programmableBlocks) {
        // Iterate through the (ordered) pipes of the target architecture.
        auto subResult = processDeclaration(declTuple.second, blockIdx);
        pipelineSequence.insert(pipelineSequence.end(), subResult.begin(), subResult.end());
        ++blockIdx;
    }
    // Sending a too short packet eBPF produces nonsense, so we require the packet size to be
    // larger than 32 bits
    targetConstraints =
        new IR::Grt(IR::Type::Boolean::get(), ExecutionState::getInputPacketSizeVar(),
                    IRUtils::getConstant(ExecutionState::getPacketSizeVarType(), 32));
}

const ordered_map<cstring, const IR::Type_Declaration*>* EBPFProgramInfo::getProgrammableBlocks()
    const {
    return &programmableBlocks;
}

std::vector<Continuation::Command> EBPFProgramInfo::processDeclaration(
    const IR::Type_Declaration* declType, size_t blockIdx) const {
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
    const auto* archMember = archSpec->getArchMember(blockIdx);
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
    // After some specific pipelines (filter), we check whether the packet has been dropped.
    if ((archMember->blockName == "filter")) {
        // Also check whether we need to drop the packet.
        auto* dropStmt =
            new IR::MethodCallStatement(IRUtils::generateInternalMethodCall("drop_and_exit", {}));
        const auto* dropCheck = new IR::IfStatement(dropIsActive(), dropStmt, nullptr);
        result.emplace_back(dropCheck);
    }
    return result;
}

const IR::Member* EBPFProgramInfo::getTargetInputPortVar() const {
    return new IR::Member(IRUtils::getBitType(TestgenTarget::getPortNumWidth_bits()),
                          new IR::PathExpression("*standard_metadata"), "ingress_port");
}

const IR::Member* EBPFProgramInfo::getTargetOutputPortVar() const {
    return new IR::Member(IRUtils::getBitType(TestgenTarget::getPortNumWidth_bits()),
                          new IR::PathExpression("*standard_metadata"), "egress_spec");
}

const IR::Expression* EBPFProgramInfo::dropIsActive() const {
    return new IR::LNot(
        new IR::Member(IR::Type_Boolean::get(), new IR::PathExpression("*accept"), "*"));
}

const IR::Expression* EBPFProgramInfo::createTargetUninitialized(const IR::Type* type,
                                                                 bool forceTaint) const {
    if (forceTaint) {
        return IRUtils::getTaintExpression(type);
    }
    return IRUtils::getDefaultValue(type);
}

const IR::Type_Bits* EBPFProgramInfo::getParserErrorType() const { return &parserErrBits; }

const IR::Member* EBPFProgramInfo::getParserParamVar(const IR::P4Parser* parser,
                                                     const IR::Type* type, size_t paramIndex,
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
        structLabel = archSpec->getParamName("parse", paramIndex);
    }
    return new IR::Member(type, new IR::PathExpression(structLabel), paramLabel);
}

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools
