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

#ifndef BF_P4C_MIDEND_BLOCKMAP_H_
#define BF_P4C_MIDEND_BLOCKMAP_H_

#include "ir/ir.h"
#include "frontends/p4/evaluator/evaluator.h"

/* FIXME -- do we really need this pass?  Expression::type fields are set by TypeInferencing */

class FillFromBlockMap : public Transform {
    P4::ReferenceMap* refMap = nullptr;
    P4::TypeMap* typeMap = nullptr;

    const IR::Expression *preorder(IR::Expression *exp) override {
        if (exp->type == IR::Type::Unknown::get())
            if (auto type = typeMap->getType(getOriginal()))
                exp->type = type;
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

    const IR::StructExpression *preorder(IR::StructExpression * si) override {
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
    FillFromBlockMap(P4::ReferenceMap* refMap, P4::TypeMap* typeMap)
    : refMap(refMap), typeMap(typeMap) { dontForwardChildrenBeforePreorder = true; }
};

#endif /* BF_P4C_MIDEND_BLOCKMAP_H_ */
