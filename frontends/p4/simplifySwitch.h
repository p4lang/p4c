/*
 * Copyright 2021 VMware, Inc.
 * SPDX-FileCopyrightText: 2021 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_SIMPLIFYSWITCH_H_
#define FRONTENDS_P4_SIMPLIFYSWITCH_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/** @brief Simplify select and switch statements that have constant arguments.
 */
class DoSimplifySwitch : public Transform {
    TypeMap *typeMap;

    bool matches(const IR::Expression *left, const IR::Expression *right) const;

 public:
    explicit DoSimplifySwitch(TypeMap *typeMap) : typeMap(typeMap) {
        setName("DoSimplifySwitch");
        CHECK_NULL(typeMap);
    }

    const IR::Node *postorder(IR::SwitchStatement *stat) override;
};

class SimplifySwitch : public PassManager {
 public:
    explicit SimplifySwitch(TypeMap *typeMap) {
        passes.push_back(new TypeChecking(nullptr, typeMap));
        passes.push_back(new DoSimplifySwitch(typeMap));
        setName("SimplifySwitch");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_SIMPLIFYSWITCH_H_ */
