/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/**
 * \defgroup SimplifyIfStatement P4::SimplifyIfStatement
 * \ingroup midend
 * \brief Set of passes that simplify if statements.
 */
#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_SIMPLIFYIFSTATEMENT_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_SIMPLIFYIFSTATEMENT_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/**
 * \ingroup SimplifyIfStatement
 * \brief Pass that eliminates call expressions in if statement’s condition.
 *
 * Eliminating CallExpr in IfStatement condition
 *
 *     if (extern.methodcall() == 1w0) {
 *        ...
 *     }
 *
 * transform it to
 *
 *     bit<1> tmp = extern.methodcall();
 *     if (tmp == 1w0) {
 *         ...
 *     }
 *
 */

class ElimCallExprInIfCond : public Transform {
    ReferenceMap *refMap;

 public:
    ElimCallExprInIfCond(ReferenceMap *refMap, TypeMap *) : refMap(refMap) {}
    const IR::Node *preorder(IR::IfStatement *statement) override;
    const IR::Node *postorder(IR::MethodCallExpression *methodCall) override;

    IR::IndexedVector<IR::StatOrDecl> stack;
};

/**
 * \ingroup SimplifyIfStatement
 * \brief Top level PassManager that governs simplification of if statements.
 *
 */
class SimplifyIfStatement : public PassManager {
 public:
    SimplifyIfStatement(ReferenceMap *refMap, TypeMap *typeMap) {
        passes.push_back(new ElimCallExprInIfCond(refMap, typeMap));
        passes.push_back(new SimplifyControlFlow(typeMap));
    }
};

}  // namespace P4

#endif /* BACKENDS_TOFINO_BF_P4C_MIDEND_SIMPLIFYIFSTATEMENT_H_ */
