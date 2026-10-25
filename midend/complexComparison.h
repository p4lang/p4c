/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_COMPLEXCOMPARISON_H_
#define MIDEND_COMPLEXCOMPARISON_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Implements a pass that removes comparisons on complex values
 * by converting them to comparisons on all their fields.
 */
class RemoveComplexComparisons : public Transform {
 protected:
    TypeMap *typeMap;

    /// Expands left == right into sub-field comparisons
    const IR::Expression *explode(Util::SourceInfo srcInfo, const IR::Type *leftType,
                                  const IR::Expression *left, const IR::Type *rightType,
                                  const IR::Expression *right);

 public:
    explicit RemoveComplexComparisons(TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("RemoveComplexComparisons");
    }
    const IR::Node *postorder(IR::Operation_Binary *expression) override;
};

class SimplifyComparisons final : public PassManager {
 public:
    explicit SimplifyComparisons(TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new RemoveComplexComparisons(typeMap));
        setName("SimplifyComparisons");
    }
};

}  // namespace P4

#endif /* MIDEND_COMPLEXCOMPARISON_H_ */
