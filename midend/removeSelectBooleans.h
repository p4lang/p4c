/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_REMOVESELECTBOOLEANS_H_
#define MIDEND_REMOVESELECTBOOLEANS_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/**
Casts boolean expressions in a select or a select case keyset
to the type bit<1>.

We assume that this pass is run after SimplifySelectLists, so there
are only scalar types in the select expression.
*/
class DoRemoveSelectBooleans : public Transform {
    const P4::TypeMap *typeMap;

    const IR::Expression *addToplevelCasts(const IR::Expression *expression);

 public:
    explicit DoRemoveSelectBooleans(const P4::TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoRemoveSelectBooleans");
    }

    const IR::Node *postorder(IR::SelectExpression *expression) override;
    const IR::Node *postorder(IR::SelectCase *selectCase) override;
};

class RemoveSelectBooleans : public PassManager {
 public:
    explicit RemoveSelectBooleans(TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoRemoveSelectBooleans(typeMap));
        passes.push_back(new ClearTypeMap(typeMap));  // some types have changed
        setName("RemoveSelectBooleans");
    }
};

}  // namespace P4

#endif /* MIDEND_REMOVESELECTBOOLEANS_H_ */
