/*
 * Copyright 2019 VMware, Inc.
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_REMOVEMISS_H_
#define MIDEND_REMOVEMISS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/**
 *  This visitor converts table.apply().miss into !table.apply().hit.
 *  In an if statement it actually inverts the branches.
 */
class DoRemoveMiss : public Transform, public ResolutionContext {
    TypeMap *typeMap;

 public:
    explicit DoRemoveMiss(TypeMap *typeMap) : typeMap(typeMap) {
        visitDagOnce = false;
        CHECK_NULL(typeMap);
        setName("DoRemoveMiss");
    }
    const IR::Node *preorder(IR::Member *expression) override;
    const IR::Node *preorder(IR::IfStatement *statement) override;
};

class RemoveMiss : public PassManager {
 public:
    explicit RemoveMiss(TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoRemoveMiss(typeMap));
        setName("RemoveMiss");
    }
};

}  // namespace P4

#endif /* MIDEND_REMOVEMISS_H_ */
