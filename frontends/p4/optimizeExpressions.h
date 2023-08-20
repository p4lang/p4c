#ifndef FRONTENDS_P4_OPTIMIZEEXPRESSIONS_H_
#define FRONTENDS_P4_OPTIMIZEEXPRESSIONS_H_

#include "frontends/common/constantFolding.h"
#include "frontends/p4/strengthReduction.h"

namespace P4 {

/// Applies expression optimizations to the input node.
/// Currently, performs constant folding and strength reduction.
inline const IR::Expression *optimizeExpression(const IR::Expression *node) {
    auto pass = PassRepeated({
        new P4::StrengthReduction(nullptr, nullptr, nullptr),
        new P4::ConstantFolding(nullptr, nullptr, false),
    });
    node = node->apply(pass);
    BUG_CHECK(::errorCount() == 0, "Encountered errors while trying to optimize expressions.");
    return node;
}

}  // namespace P4

#endif /* FRONTENDS_P4_OPTIMIZEEXPRESSIONS_H_ */
