/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_CHECKCONSTANTS_H_
#define FRONTENDS_P4_CHECKCONSTANTS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/// Makes sure that some methods that expect constant
/// arguments have constant arguments (e.g., push_front).
/// Checks that table sizes are constant integers.
class DoCheckConstants : public Inspector, public ResolutionContext {
    TypeMap *typeMap;

 public:
    explicit DoCheckConstants(TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoCheckConstants");
    }

    void postorder(const IR::MethodCallExpression *expr) override;
    void postorder(const IR::KeyElement *expr) override;
    void postorder(const IR::P4Table *table) override;
};

class CheckConstants : public PassManager {
 public:
    explicit CheckConstants(TypeMap *typeMap) {
        passes.push_back(new TypeChecking(nullptr, typeMap));
        passes.push_back(new DoCheckConstants(typeMap));
        setName("CheckConstants");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_CHECKCONSTANTS_H_ */
