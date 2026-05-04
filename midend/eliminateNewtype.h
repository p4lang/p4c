/*
 * Copyright 2018 VMware, Inc.
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_ELIMINATENEWTYPE_H_
#define MIDEND_ELIMINATENEWTYPE_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Replaces newtype by the type it was defined to represent.
 * This can be done when all the information required by the
 * control-plane has been generated.  We actually just replace
 * the newtype by a typedef.
 */
class DoReplaceNewtype final : public Transform {
    const TypeMap *typeMap;

 public:
    explicit DoReplaceNewtype(const TypeMap *typeMap) : typeMap(typeMap) {
        setName("DoReplaceNewtype");
    }
    const IR::Node *postorder(IR::Type_Newtype *type) override {
        return new IR::Type_Typedef(type->srcInfo, type->name, type->type);
    }
    const IR::Node *postorder(IR::Cast *expression) override;
};

class EliminateNewtype final : public PassManager {
 public:
    explicit EliminateNewtype(TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoReplaceNewtype(typeMap));
        passes.push_back(new ClearTypeMap(typeMap));
        setName("EliminateNewtype");
    }
};

}  // namespace P4

#endif /* MIDEND_ELIMINATENEWTYPE_H_ */
