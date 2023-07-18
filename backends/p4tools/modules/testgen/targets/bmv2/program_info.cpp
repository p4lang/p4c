#include "backends/p4tools/modules/testgen/targets/bmv2/program_info.h"

#include <list>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/null.h"

#include "backends/p4tools/modules/testgen//lib/exceptions.h"
#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/packet_vars.h"
#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/concolic.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/constants.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/map_direct_externs.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/p4_asserts_parser.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/p4_refers_to_parser.h"

namespace P4Tools::P4Testgen::Bmv2 {

const IR::Type_Bits Bmv2V1ModelProgramInfo::PARSER_ERR_BITS = IR::Type_Bits(32, false);

Bmv2V1ModelProgramInfo::Bmv2V1ModelProgramInfo(
    const IR::P4Program *program, ordered_map<cstring, const IR::Type_Declaration *> inputBlocks,
    std::map<int, int> declIdToGress)
    : ProgramInfo(program),
      programmableBlocks(std::move(inputBlocks)),
      declIdToGress(std::move(declIdToGress)) {
    const auto &options = TestgenOptions::get();
    concolicMethodImpls.add(*Bmv2Concolic::getBmv2ConcolicMethodImpls());

    // Just concatenate everything together.
    // Iterate through the (ordered) pipes of the target architecture.
    const auto *archSpec = TestgenTarget::getArchSpec();
    BUG_CHECK(archSpec->getArchVectorSize() == programmableBlocks.size(),
              "The BMV2 architecture requires %1% pipes (provided %2% pipes).",
              archSpec->getArchVectorSize(), programmableBlocks.size());

    /// Compute the series of nodes corresponding to the in-order execution of top-level
    /// pipeline-component instantiations. For a standard v1model, this produces
    /// the parser, the checksum verifier, the MAU pipeline, the checksum calculator, and finally
    /// the deparser. This sequence also includes nodes that handle transitions between the
    /// individual component instantiations.
    int pipeIdx = 0;

    for (const auto &declTuple : programmableBlocks) {
        blockMap.emplace(declTuple.second->getName(), declTuple.first);
        // Iterate through the (ordered) pipes of the target architecture.
        auto subResult = processDeclaration(declTuple.second, pipeIdx);
        pipelineSequence.insert(pipelineSequence.end(), subResult.begin(), subResult.end());
        ++pipeIdx;
    }
    /// Sending a too short packet in BMV2 produces nonsense, so we require the packet size to be
    /// larger than 32 bits.This number needs to be raised to the size of the ethernet header for
    /// the PTF and PROTOBUF back ends.
    auto minPktSize = BMv2Constants::STF_MIN_PKT_SIZE;
    if (options.testBackend == "PTF") {
        minPktSize = BMv2Constants::ETH_HDR_SIZE;
    }
    const IR::Expression *constraint =
        new IR::Grt(IR::Type::Boolean::get(), ExecutionState::getInputPacketSizeVar(),
                    IR::getConstant(&PacketVars::PACKET_SIZE_VAR_TYPE, minPktSize));
    // Vector containing pairs of restrictions and nodes to which these restrictions apply.
    std::vector<std::vector<const IR::Expression *>> restrictionsVec;
    // Defines all "entry_restriction" and then converts restrictions from string to IR
    // expressions, and stores them in restrictionsVec to move targetConstraints further.
    program->apply(AssertsParser::AssertsParser(restrictionsVec));
    // Defines all "refers_to" and then converts restrictions from string to IR expressions,
    // and stores them in restrictionsVec to move targetConstraints further.
    program->apply(RefersToParser::RefersToParser(restrictionsVec));
    for (const auto &element : restrictionsVec) {
        for (const auto *restriction : element) {
            constraint = new IR::LAnd(constraint, restriction);
        }
    }
    // Try to map all instances of direct externs to the table they are attached to.
    // Save the map in @var directExternMap.
    auto directExternMapper = MapDirectExterns();
    program->apply(directExternMapper);
    auto mappedDirectExterns = directExternMapper.getdirectExternMap();
    directExternMap.insert(mappedDirectExterns.begin(), mappedDirectExterns.end());

    /// Finally, set the target constraints.
    targetConstraints = constraint;
}

const IR::P4Table *Bmv2V1ModelProgramInfo::getTableofDirectExtern(
    const IR::IDeclaration *directExternDecl) const {
    auto it = directExternMap.find(directExternDecl);
    if (it == directExternMap.end()) {
        BUG("No table associated with this direct extern %1%. The extern should have been removed.",
            directExternDecl);
    }
    return it->second;
}

const ordered_map<cstring, const IR::Type_Declaration *>
    *Bmv2V1ModelProgramInfo::getProgrammableBlocks() const {
    return &programmableBlocks;
}

int Bmv2V1ModelProgramInfo::getGress(const IR::Type_Declaration *decl) const {
    return declIdToGress.at(decl->declid);
}

std::vector<Continuation::Command> Bmv2V1ModelProgramInfo::processDeclaration(
    const IR::Type_Declaration *typeDecl, size_t blockIdx) const {
    // Get the architecture specification for this target.
    const auto *archSpec = TestgenTarget::getArchSpec();
    const auto &options = TestgenOptions::get();
    // Collect parameters.
    const auto *applyBlock = typeDecl->to<IR::IApply>();
    if (applyBlock == nullptr) {
        TESTGEN_UNIMPLEMENTED("Constructed type %s of type %s not supported.", typeDecl,
                              typeDecl->node_type_name());
    }
    // Retrieve the current canonical pipe in the architecture spec using the pipe index.
    const auto *archMember = archSpec->getArchMember(blockIdx);

    std::vector<Continuation::Command> cmds;
    // Copy-in.
    const auto *copyInCall = new IR::MethodCallStatement(
        Utils::generateInternalMethodCall("copy_in", {new IR::PathExpression(typeDecl->name)}));
    cmds.emplace_back(copyInCall);
    // Insert the actual pipeline.
    cmds.emplace_back(typeDecl);
    // Copy-out.
    const auto *copyOutCall = new IR::MethodCallStatement(
        Utils::generateInternalMethodCall("copy_out", {new IR::PathExpression(typeDecl->name)}));
    cmds.emplace_back(copyOutCall);
    auto *dropStmt =
        new IR::MethodCallStatement(Utils::generateInternalMethodCall("drop_and_exit", {}));

    // Update some metadata variables for egress processing after we are done with Ingress
    // processing. For example, the egress port.
    if ((archMember->blockName == "Ingress")) {
        auto *egressPortVar =
            new IR::Member(IR::getBitType(BMv2Constants::PORT_BIT_WIDTH),
                           new IR::PathExpression("*standard_metadata"), "egress_port");
        auto *portStmt = new IR::AssignmentStatement(egressPortVar, getTargetOutputPortVar());
        cmds.emplace_back(portStmt);

        /// If the the vector of permitted port ranges is not empty, set the restrictions on the
        /// possible output port.
        if (!options.permittedPortRanges.empty()) {
            const auto &outPortVar = getTargetOutputPortVar();
            /// The drop port is also always a valid port. Initialize the condition with it.
            const IR::Expression *cond = new IR::Equ(
                outPortVar, new IR::Constant(outPortVar->type, BMv2Constants::DROP_PORT));
            for (auto portRange : options.permittedPortRanges) {
                const auto *loVarOut = IR::getConstant(outPortVar->type, portRange.first);
                const auto *hiVarOut = IR::getConstant(outPortVar->type, portRange.second);
                cond = new IR::LOr(cond, new IR::LAnd(new IR::Leq(loVarOut, outPortVar),
                                                      new IR::Leq(outPortVar, hiVarOut)));
            }
            cmds.emplace_back(Continuation::Guard(cond));
        }
        // TODO: We have not implemented multi cast yet.
        // Drop the packet if the multicast group is set.
        const IR::Expression *mcastGroupVar = new IR::Member(
            IR::getBitType(16), new IR::PathExpression("*standard_metadata"), "mcast_grp");
        mcastGroupVar = new IR::Neq(mcastGroupVar, IR::getConstant(IR::getBitType(16), 0));
        auto *mcastStmt = new IR::IfStatement(mcastGroupVar, dropStmt, nullptr);
        cmds.emplace_back(mcastStmt);
    }
    // After some specific pipelines (deparsers), we have to append the remaining packet
    // payload. We use an extern call for this.
    if ((archMember->blockName == "Deparser")) {
        const auto *stmt = new IR::MethodCallStatement(
            Utils::generateInternalMethodCall("prepend_emit_buffer", {}));
        cmds.emplace_back(stmt);
        // Also check whether we need to drop the packet.
        const auto *dropCheck = new IR::IfStatement(dropIsActive(), dropStmt, nullptr);
        cmds.emplace_back(dropCheck);
        const auto *recirculateCheck = new IR::MethodCallStatement(
            Utils::generateInternalMethodCall("invoke_traffic_manager", {}));
        cmds.emplace_back(recirculateCheck);
    }
    return cmds;
}

const IR::StateVariable &Bmv2V1ModelProgramInfo::getTargetInputPortVar() const {
    return *new IR::StateVariable(new IR::Member(IR::getBitType(BMv2Constants::PORT_BIT_WIDTH),
                                                 new IR::PathExpression("*standard_metadata"),
                                                 "ingress_port"));
}

const IR::StateVariable &Bmv2V1ModelProgramInfo::getTargetOutputPortVar() const {
    return *new IR::StateVariable(new IR::Member(IR::getBitType(BMv2Constants::PORT_BIT_WIDTH),
                                                 new IR::PathExpression("*standard_metadata"),
                                                 "egress_spec"));
}

const IR::Expression *Bmv2V1ModelProgramInfo::dropIsActive() const {
    const auto &egressPortVar = getTargetOutputPortVar();
    return new IR::Equ(IR::getConstant(egressPortVar->type, BMv2Constants::DROP_PORT),
                       egressPortVar);
}

const IR::Type_Bits *Bmv2V1ModelProgramInfo::getParserErrorType() const { return &PARSER_ERR_BITS; }

const IR::PathExpression *Bmv2V1ModelProgramInfo::getBlockParam(cstring blockLabel,
                                                                size_t paramIndex) const {
    // Retrieve the block and get the parameter type.
    // TODO: This should be necessary, we should be able to this using only the arch spec.
    // TODO: Make this more general and lift it into program_info core.
    const auto *programmableBlocks = getProgrammableBlocks();
    const auto *typeDecl = programmableBlocks->at(blockLabel);
    const auto *applyBlock = typeDecl->to<IR::IApply>();
    CHECK_NULL(applyBlock);
    const auto *params = applyBlock->getApplyParameters();
    const auto *param = params->getParameter(paramIndex);
    const auto *paramType = param->type;
    // For convenience, resolve type names.
    if (const auto *tn = paramType->to<IR::Type_Name>()) {
        paramType = resolveProgramType(program, tn);
    }

    const auto *archSpec = TestgenTarget::getArchSpec();
    auto archIndex = archSpec->getBlockIndex(blockLabel);
    auto archRef = archSpec->getParamName(archIndex, paramIndex);
    return new IR::PathExpression(paramType, new IR::Path(archRef));
}

const IR::Member *Bmv2V1ModelProgramInfo::getParserParamVar(const IR::P4Parser *parser,
                                                            const IR::Type *type, size_t paramIndex,
                                                            cstring paramLabel) {
    // If the optional parser parameter is not present, write directly to the
    // global parser metadata state. Otherwise, we retrieve the parameter name.
    auto parserApplyParams = parser->getApplyParameters()->parameters;
    cstring structLabel = nullptr;
    if (parserApplyParams.size() > paramIndex) {
        const auto *paramString = parser->getApplyParameters()->parameters.at(paramIndex);
        structLabel = paramString->name;
    } else {
        const auto *archSpec = TestgenTarget::getArchSpec();
        structLabel = archSpec->getParamName("Parser", paramIndex);
    }
    return new IR::Member(type, new IR::PathExpression(structLabel), paramLabel);
}

}  // namespace P4Tools::P4Testgen::Bmv2
