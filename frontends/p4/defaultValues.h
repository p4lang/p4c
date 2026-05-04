/*
 * Copyright 2023 VMWare, Inc.
 * SPDX-FileCopyrightText: 2023 VMWare, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_DEFAULTVALUES_H_
#define FRONTENDS_P4_DEFAULTVALUES_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Expand the 'default values' initializer ...
 */
class DoDefaultValues final : public Transform {
    TypeMap *typeMap;

    const IR::Expression *defaultValue(const IR::Expression *expression, const IR::Type *type);

 public:
    explicit DoDefaultValues(TypeMap *typeMap) : typeMap(typeMap) { CHECK_NULL(typeMap); }
    const IR::Node *postorder(IR::Dots *dots) override;
    const IR::Node *postorder(IR::StructExpression *expression) override;
    const IR::Node *postorder(IR::ListExpression *expression) override;
    const IR::Node *postorder(IR::ArrayExpression *expression) override;
};

class DefaultValues : public PassManager {
 public:
    explicit DefaultValues(TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (typeMap != nullptr) {
            if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap, true);
            passes.push_back(typeChecking);
        }
        passes.push_back(new DoDefaultValues(typeMap));
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_DEFAULTVALUES_H_ */
