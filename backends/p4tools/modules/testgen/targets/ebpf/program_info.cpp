#include "backends/p4tools/modules/testgen/targets/ebpf/program_info.h"

#include <list>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen//lib/exceptions.h"
#include "backends/p4tools/modules/testgen/core/compiler_target.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/packet_vars.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/concolic.h"
#include "backends/p4tools/modules/testgen/targets/ebpf/constants.h"

namespace P4Tools::P4Testgen::EBPF {

const IR::Type_Bits EBPFProgramInfo::PARSER_ERR_BITS = IR::Type_Bits(32, false);

EBPFProgramInfo::EBPFProgramInfo(const TestgenCompilerResult &compilerResult,
                                 ordered_map<cstring, const IR::Type_Declaration *> inputBlocks)
    : ProgramInfo(compilerResult), programmableBlocks(std::move(inputBlocks)) {
    concolicMethodImpls.add(*EBPFConcolic::getEBPFConcolicMethodImpls());

    // Just concatenate everything together.
    // Iterate through the (ordered) pipes of the target architecture.
    const auto &archSpec = getArchSpec();
    BUG_CHECK(archSpec.getArchVectorSize() == programmableBlocks.size(),
              "The eBPF architecture requires %1% pipes (provided %2% pipes).",
              archSpec.getArchVectorSize(), programmableBlocks.size());

    /// Compute the series of nodes corresponding to the in-order execution of top-level
    /// pipeline-component instantiations. For a standard ebpf_model, this produces
    /// the parser and the filter.
    /// This sequence also includes nodes that handle transitions between the individual component
    /// instantiations.
    int blockIdx = 0;
    for (const auto &declTuple : programmableBlocks) {
        blockMap.emplace(declTuple.second->getName(), declTuple.first);
        // Iterate through the (ordered) pipes of the target architecture.
        auto subResult = processDeclaration(declTuple.second, blockIdx);
        pipelineSequence.insert(pipelineSequence.end(), subResult.begin(), subResult.end());
        ++blockIdx;
    }
    // The input packet should be larger than 0.
    targetConstraints =
        new IR::Grt(IR::Type::Boolean::get(), ExecutionState::getInputPacketSizeVar(),
                    IR::getConstant(&PacketVars::PACKET_SIZE_VAR_TYPE, 0));
}

const ArchSpec &EBPFProgramInfo::getArchSpec() const { return ARCH_SPEC; }

const ordered_map<cstring, const IR::Type_Declaration *> *EBPFProgramInfo::getProgrammableBlocks()
    const {
    return &programmableBlocks;
}

std::vector<Continuation::Command> EBPFProgramInfo::processDeclaration(
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

    // After some specific pipelines (filter), we check whether the packet has been dropped.
    // eBPF can not modify the packet, so we do not append any emit buffer here.
    if ((archMember->blockName == "filter")) {
        auto *dropStmt =
            new IR::MethodCallStatement(Utils::generateInternalMethodCall("drop_and_exit", {}));
        const auto *dropCheck = new IR::IfStatement(dropIsActive(), dropStmt, nullptr);
        cmds.emplace_back(dropCheck);
    }
    return cmds;
}

const IR::StateVariable &EBPFProgramInfo::getTargetInputPortVar() const {
    return *new IR::StateVariable(new IR::Member(IR::getBitType(EBPFConstants::PORT_BIT_WIDTH),
                                                 new IR::PathExpression("*"), "input_port"));
}

const IR::StateVariable &EBPFProgramInfo::getTargetOutputPortVar() const {
    return *new IR::StateVariable(new IR::Member(IR::getBitType(EBPFConstants::PORT_BIT_WIDTH),
                                                 new IR::PathExpression("*"), "output_port"));
}

const IR::Expression *EBPFProgramInfo::dropIsActive() const {
    return new IR::LNot(new IR::PathExpression(IR::Type_Boolean::get(), new IR::Path("*accept")));
}

const IR::Type_Bits *EBPFProgramInfo::getParserErrorType() const { return &PARSER_ERR_BITS; }

const ArchSpec EBPFProgramInfo::ARCH_SPEC =
    ArchSpec("ebpfFilter", {// parser parse<H>(packet_in packet, out H headers);
                            {"parse", {nullptr, "*hdr"}},
                            // control filter<H>(inout H headers, out bool accept);
                            {"filter", {"*hdr", "*accept"}}});

}  // namespace P4Tools::P4Testgen::EBPF
