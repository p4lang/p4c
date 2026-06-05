/*
 * Copyright 2018 VMware, Inc.
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_ELIMINATESERENUMS_H_
#define MIDEND_ELIMINATESERENUMS_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Replaces serializable enum constants with their values.
 */
class DoEliminateSerEnums final : public Transform {
    const TypeMap *typeMap;

 public:
    explicit DoEliminateSerEnums(const TypeMap *typeMap) : typeMap(typeMap) {
        setName("DoEliminateSerEnums");
        visitDagOnce = false;
    }
    const IR::Node *preorder(IR::Type_SerEnum *type) override;
    const IR::Node *postorder(IR::Type_Name *type) override;
    const IR::Node *postorder(IR::Member *expression) override;
};

class EliminateSerEnums final : public PassManager {
 public:
    explicit EliminateSerEnums(TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoEliminateSerEnums(typeMap));
        passes.push_back(new ClearTypeMap(typeMap));
        setName("EliminateSerEnums");
    }
};

}  // namespace P4

#endif /* MIDEND_ELIMINATESERENUMS_H_ */
