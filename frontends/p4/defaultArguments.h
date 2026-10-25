/*
 * Copyright 2018 VMware, Inc.
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_DEFAULTARGUMENTS_H_
#define FRONTENDS_P4_DEFAULTARGUMENTS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Add arguments for parameters that have default values but no corresponding argument.
 * For example, given:
 * void f(in bit<32> a = 0);
 * convert
 * f();
 * to
 * f(a = 0);
 */
class DoDefaultArguments : public Transform, public ResolutionContext {
    TypeMap *typeMap;

 public:
    explicit DoDefaultArguments(TypeMap *typeMap) : typeMap(typeMap) {
        setName("DoDefaultArguments");
        CHECK_NULL(typeMap);
    }
    const IR::Node *postorder(IR::MethodCallExpression *expression) override;
    const IR::Node *postorder(IR::Declaration_Instance *inst) override;
    const IR::Node *postorder(IR::ConstructorCallExpression *ccc) override;
    const IR::Node *preorder(IR::ActionList *al) override {
        // don't modify the action lists in tables
        prune();
        return al;
    }
};

class DefaultArguments : public PassManager {
 public:
    explicit DefaultArguments(TypeMap *typeMap) {
        setName("DefaultArguments");
        passes.push_back(new TypeChecking(nullptr, typeMap));
        passes.push_back(new DoDefaultArguments(typeMap));
        passes.push_back(new ClearTypeMap(typeMap));
        // this may insert casts into the new arguments
        passes.push_back(new TypeInference(typeMap, false));
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_DEFAULTARGUMENTS_H_ */
