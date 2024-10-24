/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
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
