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

#ifndef BF_P4C_MIDEND_BLOCKMAP_H_
#define BF_P4C_MIDEND_BLOCKMAP_H_

#include "frontends/p4/evaluator/evaluator.h"
#include "ir/ir.h"

/* FIXME -- do we really need this pass?  Expression::type fields are set by TypeInferencing */

class FillFromBlockMap : public Transform {
    P4::ReferenceMap *refMap = nullptr;
    P4::TypeMap *typeMap = nullptr;

    const IR::Expression *preorder(IR::Expression *exp) override {
        if (exp->type == IR::Type::Unknown::get())
            if (auto type = typeMap->getType(getOriginal())) exp->type = type;
        return exp;
    }

    const IR::Type *preorder(IR::Type_Name *type) override {
        if (auto decl = refMap->getDeclaration(type->path)) {
            if (auto tdecl = decl->getNode()->to<IR::Type_Declaration>())
                return transform_child(tdecl)->to<IR::Type>();
            else
                BUG("Type_Name %1% maps to %2% rather than a type decl", type, decl);
        } else {
            BUG("Type_Name %1% doesn't map to a declaration", type, decl);
        }
    }

    const IR::StructExpression *preorder(IR::StructExpression *si) override {
        // Don't visit typeName, but visit everything else.
        preorder(static_cast<IR::Expression *>(si));
        visit(si->components, "components");
        prune();
        return si;
    }

    const IR::TypeNameExpression *preorder(IR::TypeNameExpression *tn) override {
        // Don't visit typeName, but visit everything else.
        preorder(static_cast<IR::Expression *>(tn));
        prune();
        return tn;
    }

    const IR::Type_Specialized *preorder(IR::Type_Specialized *ts) override {
        // Don't visit baseType, but visit everything else.
        Transform::preorder(static_cast<IR::Type *>(ts));
        visit(ts->arguments, "arguments");
        prune();
        return ts;
    }

    const IR::ConstructorCallExpression *preorder(IR::ConstructorCallExpression *ts) override {
        // Don't visit constructedType, but visit everything else.
        preorder(static_cast<IR::Expression *>(ts));
        visit(ts->arguments, "arguments");
        prune();
        return ts;
    }

 public:
    FillFromBlockMap(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {
        dontForwardChildrenBeforePreorder = true;
    }
};

#endif /* BF_P4C_MIDEND_BLOCKMAP_H_ */
