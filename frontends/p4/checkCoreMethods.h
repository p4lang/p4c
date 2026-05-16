/*
 * Copyright 2021 VMware, Inc.
 * SPDX-FileCopyrightText: 2021 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_CHECKCOREMETHODS_H_
#define FRONTENDS_P4_CHECKCOREMETHODS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/// Check types for arguments of core.p4 methods
class DoCheckCoreMethods : public Inspector, public ResolutionContext {
    TypeMap *typeMap;

    void checkEmitType(const IR::Expression *emit, const IR::Type *type) const;
    void checkCorelibMethods(const ExternMethod *em) const;

 public:
    explicit DoCheckCoreMethods(TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoCheckCoreMethods");
    }

    void postorder(const IR::MethodCallExpression *expr) override;
};

class CheckCoreMethods : public PassManager {
 public:
    explicit CheckCoreMethods(TypeMap *typeMap) {
        passes.push_back(new TypeChecking(nullptr, typeMap));
        passes.push_back(new DoCheckCoreMethods(typeMap));
        setName("CheckCoreMethods");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_CHECKCOREMETHODS_H_ */
