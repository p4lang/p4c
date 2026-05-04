/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_USELESSCASTS_H_
#define FRONTENDS_P4_USELESSCASTS_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
Removes casts where the input expression has the exact same type
as the cast type
*/
class RemoveUselessCasts : public Transform {
    const P4::TypeMap *typeMap;

 public:
    explicit RemoveUselessCasts(const P4::TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("RemoveUselessCasts");
    }
    const IR::Node *postorder(IR::Cast *cast) override;
};

class UselessCasts : public PassManager {
 public:
    explicit UselessCasts(TypeMap *typeMap) {
        passes.push_back(new TypeChecking(nullptr, typeMap));
        passes.push_back(new RemoveUselessCasts(typeMap));
        setName("UselessCasts");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_USELESSCASTS_H_ */
