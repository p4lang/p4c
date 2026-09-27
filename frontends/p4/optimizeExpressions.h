/*
 * SPDX-FileCopyrightText: 2022 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_OPTIMIZEEXPRESSIONS_H_
#define FRONTENDS_P4_OPTIMIZEEXPRESSIONS_H_

#include "frontends/common/constantFolding.h"
#include "frontends/p4/strengthReduction.h"

namespace P4 {

/// Applies expression optimizations to the input node.
/// Currently, performs constant folding and strength reduction.
inline IR::Ptr<IR::Expression> optimizeExpression(IR::Ptr<IR::Expression> node) {
    P4::StrengthReductionPolicy strengthReductionPolicy;
    P4::ConstantFoldingPolicy constantFoldingPolicy;
    P4::DoStrengthReduction strengthReduction(nullptr, &strengthReductionPolicy);
    P4::DoConstantFolding constantFolding(nullptr, false, &constantFoldingPolicy);
    auto pass = PassRepeated({&strengthReduction, &constantFolding});
    node = node->apply(pass);
    BUG_CHECK(::P4::errorCount() == 0, "Encountered errors while trying to optimize expressions.");
    return node;
}

}  // namespace P4

#endif /* FRONTENDS_P4_OPTIMIZEEXPRESSIONS_H_ */
