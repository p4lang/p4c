/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "simplify_nested_if.h"

#include <cmath>
#include <functional>
#include <numeric>

#include "lib/bitvec.h"

namespace P4 {

const IR::Node *DoSimplifyNestedIf::preorder(IR::P4Control *control) {
    if (policy != nullptr && !policy->convert(control)) prune();  // skip this one
    return control;
}

std::vector<int> *DoSimplifyNestedIf::checkTypeAndGetVec(const IR::Statement *stmt) {
    const IR::MethodCallStatement *methodStmt = nullptr;
    if (auto bstmt = stmt->to<IR::BlockStatement>()) {
        for (auto b : bstmt->components) {
            methodStmt = b->to<IR::MethodCallStatement>();
        }
    } else {
        methodStmt = stmt->to<IR::MethodCallStatement>();
    }
    if (!methodStmt) return nullptr;
    auto methodCall = methodStmt->methodCall->to<IR::MethodCallExpression>();
    auto method = methodCall->method->to<IR::Member>();
    if (!method) return nullptr;
    auto expr = method->expr->to<IR::PathExpression>();
    if (!expr) return nullptr;
    const IR::Type_Extern *typeEx = nullptr;
    if (auto type = expr->type->to<IR::Type_SpecializedCanonical>()) {
        typeEx = type->baseType->to<IR::Type_Extern>();
    } else {
        typeEx = expr->type->to<IR::Type_Extern>();
    }
    if (!typeEx) return nullptr;
    if (typeEx->name == "Mirror") {
        return mirrorType;
    } else if (typeEx->name == "Resubmit") {
        return resubmitType;
    } else if (typeEx->name == "Digest") {
        return digestType;
    }
    return nullptr;
}

void DoSimplifyNestedIf::addInArray(std::vector<int> *typeVec, const IR::Expression *condition,
                                    const IR::IfStatement *ifstmt) {
    if (auto neq = condition->to<IR::LNot>()) {
        condition = neq->expr;
    }
    Pattern::Match<IR::Expression> expr;
    Pattern::Match<IR::Constant> constant;
    if (auto cond = condition->to<IR::Operation_Binary>()) {
        if ((expr == constant).match(cond)) {
            typeVec->at(constant->asInt()) = 1;
        } else {
            error(
                "If statement is too complex to handle.%1% \n Supported if condition :"
                " if (digest/mirror/resubmit_type == constant)",
                ifstmt->srcInfo);
        }
    }
    return;
}

void DoSimplifyNestedIf::setExtraStmts(std::vector<int> *typeVec, const IR::Statement *stmt,
                                       const IR::Expression *condition) {
    if (auto neq = condition->to<IR::LNot>()) {
        condition = neq->expr;
    }
    if (auto cond = condition->to<IR::Operation_Binary>()) {
        auto size = cond->right->to<IR::Constant>()->type->width_bits();
        int idx = 0;
        for (auto a : *typeVec) {
            if (a == 0) {
                extraStmts[stmt].push_back(
                    new IR::Equ(cond->srcInfo, cond->left,
                                new IR::Constant(IR::Type_Bits::get(size), idx, 10)));
            }
            idx++;
        }
    }
}

const IR::Node *DoSimplifyNestedIf::preorder(IR::IfStatement *stmt) {
    prune();
    if (stmt->ifTrue) {
        stack_.push_back(stmt->condition);
        visit(stmt->ifTrue);
        if (stmt->ifTrue) {
            if (auto typeVec = checkTypeAndGetVec(stmt->ifTrue)) {
                addInArray(typeVec, stmt->condition, stmt);
            }
            predicates[stmt->ifTrue] = stack_;
        }
        stack_.pop_back();
    }

    if (stmt->ifFalse) {
        if (auto negCond = stmt->condition->to<IR::LNot>()) {
            stack_.push_back(negCond->expr);
        } else {
            stack_.push_back(new IR::LNot(stmt->condition));
        }
        visit(stmt->ifFalse);
        if (stmt->ifFalse && !stmt->ifFalse->is<IR::IfStatement>()) {
            // This is for a bare 'else'
            if (auto typeVec = checkTypeAndGetVec(stmt->ifFalse)) {
                setExtraStmts(typeVec, stmt->ifFalse, stmt->condition);
            } else {
                predicates[stmt->ifFalse] = stack_;
            }
        }
        stack_.pop_back();
    }

    if (stack_.size() == 0) {
        IR::IndexedVector<IR::StatOrDecl> vec;
        auto fold = [](const IR::Expression *a, const IR::Expression *b) {
            return new IR::LAnd(a, b);
        };
        for (auto p : predicates) {
            auto newcond = std::accumulate(std::next(p.second.rbegin()), p.second.rend(),
                                           p.second.back(), fold);
            vec.push_back(new IR::IfStatement(newcond, p.first, nullptr));
        }
        for (auto e : extraStmts) {
            for (auto &a : e.second) {
                vec.push_back(new IR::IfStatement(a, e.first, nullptr));
            }
        }
        predicates.clear();
        extraStmts.clear();
        return new IR::BlockStatement(stmt->srcInfo, vec);
    }
    return nullptr;
}

void DoSimplifyComplexCondition::do_equ(bitvec &val, const IR::Equ *eq) {
    if (auto cst = eq->left->to<IR::Constant>()) {
        if (!policy->check(eq->right)) return;
        unique_dest = eq->right;
        auto idx = cst->asInt();
        val.setbit(idx);
    } else if (auto cst = eq->right->to<IR::Constant>()) {
        if (!policy->check(eq->left)) return;
        unique_dest = eq->left;
        auto idx = cst->asInt();
        val.setbit(idx);
    }
}

void DoSimplifyComplexCondition::do_neq(bitvec &val, const IR::Neq *neq) {
    if (auto cst = neq->left->to<IR::Constant>()) {
        auto idx = cst->asInt();
        int width = cst->type->width_bits();
        bitvec negated;
        for (auto v = 0; v < (1 << width); v++) {
            if (idx == v) continue;
            negated.setbit(v);
        }
        val &= negated;
    } else if (auto cst = neq->right->to<IR::Constant>()) {
        auto idx = cst->asInt();
        int width = cst->type->width_bits();
        bitvec negated;
        for (auto v = 0; v < (1 << width); v++) {
            if (idx == v) continue;
            negated.setbit(v);
        }
        val &= negated;
    }
}

const IR::Node *DoSimplifyComplexCondition::preorder(IR::P4Control *control) {
    if (skip != nullptr && !skip->convert(control)) prune();  // skip this one
    return control;
}

const IR::Node *DoSimplifyComplexCondition::preorder(IR::LAnd *expr) {
    prune();
    auto ctxt = findOrigCtxt<IR::IfStatement>();
    if (!ctxt) return expr;

    if (auto e = expr->left->to<IR::LAnd>()) {
        stack_.push(e);
        visit(expr->left);
        stack_.pop();
    }

    if (auto e = expr->right->to<IR::LAnd>()) {
        stack_.push(e);
        visit(expr->right);
        stack_.pop();
    }

    auto lowered_isValid = [](const IR::Equ *eq) {
        // See DoCopyHeaders::postorder(IR::MethodCallExpression* mc)
        auto mem = eq->left->to<IR::Member>();
        return mem && mem->member == "$valid";
    };

    if (auto e = expr->left->to<IR::Equ>())
        if (!lowered_isValid(e)) do_equ(constants, e);
    if (auto e = expr->right->to<IR::Equ>())
        if (!lowered_isValid(e)) do_equ(constants, e);
    if (auto e = expr->left->to<IR::Neq>()) do_neq(constants, e);
    if (auto e = expr->right->to<IR::Neq>()) do_neq(constants, e);

    if (stack_.size() == 0) {
        if (!unique_dest) return expr;
        auto idx = constants.ffs(0);
        auto dest_width = unique_dest->type->width_bits();
        auto ret = new IR::Equ(unique_dest->type, unique_dest,
                               new IR::Constant(IR::Type_Bits::get(dest_width), idx, 10));
        constants.clear();
        unique_dest = nullptr;
        policy->reset();
        return ret;
    }
    return expr;
}

}  // namespace P4
