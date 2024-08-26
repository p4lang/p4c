#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_NIC_EXPRESSIONS_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_NIC_EXPRESSIONS_H_

#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/core/target.h"

namespace P4::P4Tools::P4Smith {

class NicExpressionGenerator : public ExpressionGenerator {
 public:
    explicit NicExpressionGenerator(SmithTarget &parent) : ExpressionGenerator(parent) {}

    [[nodiscard]] std::vector<int> availableBitWidths() const override {
        return {4, 8, 16, 32, 64};
    }
};

}  // namespace P4::P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_NIC_EXPRESSIONS_H_ */
