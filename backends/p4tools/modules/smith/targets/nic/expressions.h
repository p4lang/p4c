#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_NIC_EXPRESSIONS_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_NIC_EXPRESSIONS_H_

#include <ostream>
#include <string>

#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/parser.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/common/table.h"
#include "backends/p4tools/modules/smith/core/target.h"
#include "ir/ir.h"

namespace P4Tools::P4Smith {

class NicExpressionGenerator : public ExpressionGenerator {
 public:
    explicit NicExpressionGenerator(SmithTarget &parent) : ExpressionGenerator(parent) {}
    static constexpr int BIT_WIDTHS[5] = {4, 8, 16, 32, 64};
    static const IR::Type_Bits *genBitType(bool isSigned);
    static const IR::Type *pickRndBaseType(const std::vector<int64_t> &type_probs);
    const IR::Type *pickRndType(TyperefProbs type_probs) override;
};

}  // namespace P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_NIC_EXPRESSIONS_H_ */
