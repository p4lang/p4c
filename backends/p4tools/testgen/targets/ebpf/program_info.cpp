#include "backends/p4tools/testgen/targets/ebpf/program_info.h"

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
    // The input packet should be larger than 0.
    targetConstraints =
        new IR::Grt(IR::Type::Boolean::get(), ExecutionState::getInputPacketSizeVar(),
                    IR::IRUtils::getConstant(ExecutionState::getPacketSizeVarType(), 0));
}

const ordered_map<cstring, const IR::Type_Declaration*>* EBPFProgramInfo::getProgrammableBlocks()
    const {
    return &programmableBlocks;
}

std::vector<Continuation::Command> EBPFProgramInfo::processDeclaration(
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

    // After some specific pipelines (filter), we check whether the packet has been dropped.
    // eBPF can not modify the packet, so we do not append any emit buffer here.
    if ((archMember->blockName == "filter")) {
        auto* dropStmt =
            new IR::MethodCallStatement(Utils::generateInternalMethodCall("drop_and_exit", {}));
        const auto* dropCheck = new IR::IfStatement(dropIsActive(), dropStmt, nullptr);
        cmds.emplace_back(dropCheck);
    }
    return cmds;
}

const IR::Member* EBPFProgramInfo::getTargetInputPortVar() const {
    return new IR::Member(IR::IRUtils::getBitType(TestgenTarget::getPortNumWidth_bits()),
                          new IR::PathExpression("*"), "input_port");
}

const IR::Member* EBPFProgramInfo::getTargetOutputPortVar() const {
    return new IR::Member(IR::IRUtils::getBitType(TestgenTarget::getPortNumWidth_bits()),
                          new IR::PathExpression("*"), "output_port");
}

const IR::Expression* EBPFProgramInfo::dropIsActive() const {
    return new IR::LNot(
        new IR::Member(IR::Type_Boolean::get(), new IR::PathExpression("*accept"), "*"));
}

const IR::Expression* EBPFProgramInfo::createTargetUninitialized(const IR::Type* type,
                                                                 bool forceTaint) const {
    if (forceTaint) {
        return Utils::getTaintExpression(type);
    }
    return IR::IRUtils::getDefaultValue(type);
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
