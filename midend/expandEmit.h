/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_EXPANDEMIT_H_
#define MIDEND_EXPANDEMIT_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Convert an emit of a struct recursively into a sequence of emits of
 * all fields of the struct.  Convert an emit of a header stack into a
 * sequence of emits of all elements of the stack.
 * header H1 {}
 * header H2 {}
 * struct S {
 *    H1 h1;
 *    H2[2] h2;
 * }
 * S s;
 * packet.emit(s);
 *
 * becomes
 *
 * emit(s.h1);
 * emit(s.h2[0]);
 * emit(s.h2[1]);
 */
class DoExpandEmit : public Transform, public ResolutionContext {
    TypeMap *typeMap;

 public:
    explicit DoExpandEmit(TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoExpandEmit");
    }
    // return true if the expansion produced something "new"
    bool expandArg(const IR::Type *type, const IR::Argument *argument,
                   std::vector<const IR::Argument *> *result,
                   std::vector<const IR::Type *> *resultTypes);
    const IR::Node *postorder(IR::MethodCallStatement *statement) override;
};

class ExpandEmit : public PassManager {
 public:
    explicit ExpandEmit(TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        setName("ExpandEmit");
        if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoExpandEmit(typeMap));
    }
};

}  // namespace P4

#endif /* MIDEND_EXPANDEMIT_H_ */
