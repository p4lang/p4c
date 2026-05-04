/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_TYPECHECKING_SYNTACTICEQUIVALENCE_H_
#define FRONTENDS_P4_TYPECHECKING_SYNTACTICEQUIVALENCE_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

// Check if two expressions are syntactically equivalent
class SameExpression {
    const DeclarationLookup *refMap;
    const TypeMap *typeMap;

 public:
    explicit SameExpression(const DeclarationLookup *refMap, const TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
    }
    bool sameType(const IR::Type *left, const IR::Type *right) const;
    bool sameExpression(const IR::Expression *left, const IR::Expression *right) const;
    bool sameExpressions(const IR::Vector<IR::Expression> *left,
                         const IR::Vector<IR::Expression> *right) const;
    bool sameExpressions(const IR::Vector<IR::Argument> *left,
                         const IR::Vector<IR::Argument> *right) const;
};

}  // namespace P4

#endif /* FRONTENDS_P4_TYPECHECKING_SYNTACTICEQUIVALENCE_H_ */
