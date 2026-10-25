/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_14_TYPECHECK_H_
#define FRONTENDS_P4_14_TYPECHECK_H_

#include "ir/ir.h"
#include "ir/pass_manager.h"

namespace P4 {

/* This is the P4 v1.0/v1.1 typechecker/type inference algorithm */
class TypeCheck : public PassManager {
    std::map<const IR::Node *, const IR::Type *> actionArgUseTypes;
    int iterCounter = 0;
    class AssignInitialTypes;
    class InferExpressionsBottomUp;
    class InferExpressionsTopDown;
    class InferActionArgsBottomUp;
    class InferActionArgsTopDown;
    class AssignActionArgTypes;
    class MakeImplicitCastsExplicit;

 public:
    TypeCheck();
    const IR::Node *apply_visitor(const IR::Node *, const char *) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_14_TYPECHECK_H_ */
