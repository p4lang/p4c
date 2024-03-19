#include "backends/p4tools/modules/testgen/targets/pna/shared_program_info.h"

#include <utility>

#include "backends/p4tools/common/lib/arch_spec.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/cstring.h"
#include "lib/null.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/targets/pna/concolic.h"
#include "backends/p4tools/modules/testgen/targets/pna/constants.h"

namespace P4Tools::P4Testgen::Pna {

const IR::Type_Bits SharedPnaProgramInfo::PARSER_ERR_BITS = IR::Type_Bits(32, false);

SharedPnaProgramInfo::SharedPnaProgramInfo(
    const TestgenCompilerResult &compilerResult,
    ordered_map<cstring, const IR::Type_Declaration *> inputBlocks)
    : ProgramInfo(compilerResult), programmableBlocks(std::move(inputBlocks)) {
    concolicMethodImpls.add(*PnaDpdkConcolic::getPnaDpdkConcolicMethodImpls());
}

const ordered_map<cstring, const IR::Type_Declaration *> *
SharedPnaProgramInfo::getProgrammableBlocks() const {
    return &programmableBlocks;
}

const IR::StateVariable &SharedPnaProgramInfo::getTargetInputPortVar() const {
    return *new IR::StateVariable(new IR::Member(IR::getBitType(PnaConstants::PORT_BIT_WIDTH),
                                                 new IR::PathExpression("*parser_istd"),
                                                 "input_port"));
}

const IR::StateVariable &SharedPnaProgramInfo::getTargetOutputPortVar() const {
    return *new IR::StateVariable(&PnaConstants::OUTPUT_PORT_VAR);
}

const IR::Expression *SharedPnaProgramInfo::dropIsActive() const { return &PnaConstants::DROP_VAR; }

const IR::Type_Bits *SharedPnaProgramInfo::getParserErrorType() const { return &PARSER_ERR_BITS; }

const IR::PathExpression *SharedPnaProgramInfo::getBlockParam(cstring blockLabel,
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
        paramType = resolveProgramType(&getP4Program(), tn);
    }

    const auto &archSpec = getArchSpec();
    auto archIndex = archSpec.getBlockIndex(blockLabel);
    auto archRef = archSpec.getParamName(archIndex, paramIndex);
    return new IR::PathExpression(paramType, new IR::Path(archRef));
}

}  // namespace P4Tools::P4Testgen::Pna
