/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "simplifyIfStatement.h"

#include "bf-p4c/midend/path_linearizer.h"
#include "ir/ir.h"
#include "ir/pattern.h"
#include "ir/visitor.h"

namespace P4 {

const IR::Node *ElimCallExprInIfCond::preorder(IR::IfStatement *stmt) {
    prune();
    BUG_CHECK(stack.empty(), "if statement in if condition?");
    visit(stmt->condition, "condition");
    const IR::Node *rv = stmt;
    if (!stack.empty()) {
        auto block = new IR::BlockStatement(stack);
        stack.clear();
        block->push_back(stmt);
        rv = block;
    }
    visit(stmt->ifTrue, "ifTrue");
    visit(stmt->ifFalse, "ifFalse");
    return rv;
}

const IR::Node *ElimCallExprInIfCond::postorder(IR::MethodCallExpression *methodCall) {
    const Context *ctxt = nullptr;
    auto ifstmt = findContext<IR::IfStatement>(ctxt);
    if (!ifstmt) return methodCall;
    if (strcmp(ctxt->child_name, "condition") != 0) return methodCall;
    BFN::PathLinearizer linearizer;
    if (auto member = methodCall->method->to<IR::Member>()) {
        if (member->member.name == "isValid" || member->member.name == "apply") return methodCall;
        member->expr->apply(linearizer);
    } else if (methodCall->method->to<IR::PathExpression>()) {
        // may need to modify to support function();
        return methodCall;
    } else {
        BUG("Unexpected method call", methodCall);
    }
    auto &path = *linearizer.linearPath;
    auto tempVar = refMap->newName(path.to_cstring().string_view());
    auto decl = new IR::Declaration_Variable(tempVar, methodCall->type, methodCall->clone());
    stack.push_back(decl);
    return new IR::PathExpression(tempVar);
}

}  // namespace P4
