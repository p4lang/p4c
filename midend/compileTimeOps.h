/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIDEND_COMPILETIMEOPS_H_
#define MIDEND_COMPILETIMEOPS_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Check that operations which are only defined at compile-time
 * do not exist in the program.
 *
 * @pre This should be run after inlining, constant folding and strength reduction.
 * @post There are no IR::Mod and IR::Div operations in the program.
 */
class CompileTimeOperations : public Inspector {
 public:
    CompileTimeOperations() { setName("CompileTimeOperations"); }
    void err(const IR::Node *expression) {
        ::P4::error(ErrorType::ERR_INVALID,
                    "%1%: could not evaluate expression at compilation time", expression);
    }
    void postorder(const IR::Mod *expression) override { err(expression); }
    void postorder(const IR::Div *expression) override { err(expression); }
};

}  // namespace P4

#endif /* MIDEND_COMPILETIMEOPS_H_ */
