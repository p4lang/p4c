/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_REMOVELEFTSLICES_H_
#define MIDEND_REMOVELEFTSLICES_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
 * This pass removes Slices on the lhs of an assignment
 *
 * \code{.cpp}
 * a[m:l] = e;  ->  a = (a & ~mask) | (((cast)e << l) & mask);
 * \endcode
 *
 * @pre none
 * @post no field slice operator in the lhs of assignment statement
 */
class DoRemoveLeftSlices : public Transform {
    P4::TypeMap *typeMap;

 public:
    explicit DoRemoveLeftSlices(P4::TypeMap *typeMap) : typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("DoRemoveLeftSlices");
    }
    const IR::Node *postorder(IR::AssignmentStatement *stat) override;
};

class RemoveLeftSlices : public PassManager {
 public:
    explicit RemoveLeftSlices(TypeMap *typeMap) {
        passes.push_back(new TypeChecking(nullptr, typeMap));
        passes.push_back(new DoRemoveLeftSlices(typeMap));
        setName("RemoveLeftSlices");
    }
};

}  // namespace P4

#endif /* MIDEND_REMOVELEFTSLICES_H_ */
