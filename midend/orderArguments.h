/*
 * Copyright 2018 VMware, Inc.
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_ORDERARGUMENTS_H_
#define MIDEND_ORDERARGUMENTS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Order the arguments of a call in the order that the parameters appear.
 * This only works if all optional parameters are at the end.
 */
class DoOrderArguments : public Transform, public ResolutionContext {
    TypeMap *typeMap;

 public:
    explicit DoOrderArguments(TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoOrderArguments");
    }

    const IR::Node *postorder(IR::MethodCallExpression *expression) override;
    const IR::Node *postorder(IR::ConstructorCallExpression *expression) override;
    const IR::Node *postorder(IR::Declaration_Instance *instance) override;
};

class OrderArguments : public PassManager {
 public:
    explicit OrderArguments(TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoOrderArguments(typeMap));
        setName("OrderArguments");
    }
};

}  // namespace P4

#endif /* MIDEND_ORDERARGUMENTS_H_ */
