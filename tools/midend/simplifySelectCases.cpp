/*
Copyright 2016 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "simplifySelectCases.h"
#include "frontends/p4/enumInstance.h"

namespace P4 {

void DoSimplifySelectCases::checkSimpleConstant(const IR::Expression* expr) const {
    CHECK_NULL(expr);
    if (expr->is<IR::DefaultExpression>())
        return;
    if (expr->is<IR::Constant>())
        return;
    if (expr->is<IR::BoolLiteral>())
        return;
    if (expr->is<IR::Mask>() || expr->is<IR::Range>()) {
        auto bin = expr->to<IR::Operation_Binary>();
        checkSimpleConstant(bin->left);
        checkSimpleConstant(bin->right);
        return;
    }
    if (expr->is<IR::ListExpression>()) {
        auto list = expr->to<IR::ListExpression>();
        for (auto e : list->components)
            checkSimpleConstant(e);
        return;
    }
    auto ei = EnumInstance::resolve(expr, typeMap);
    if (ei != nullptr)
        return;

    // we allow value_set name to be used in place of select case;
    if (expr->is<IR::PathExpression>()) {
        auto type = typeMap->getType(expr);
        if (type->is<IR::Type_Set>()) {
            return;
        }
    }
    ::error("%1%: must be a compile-time constant", expr);
}

const IR::Node* DoSimplifySelectCases::preorder(IR::SelectExpression* expression) {
    IR::Vector<IR::SelectCase> cases;

    bool seenDefault = false;
    bool changes = false;
    for (auto c : expression->selectCases) {
        if (seenDefault) {
            ::warning(ErrorType::WARN_PARSER_TRANSITION, "%1%: unreachable", c);
            changes = true;
            continue;
        }
        cases.push_back(c);
        if (c->keyset->is<IR::DefaultExpression>())
            seenDefault = true;
        if (requireConstants)
            checkSimpleConstant(c->keyset);
    }
    if (changes) {
        if (cases.size() == 1) {
            // just one default label
            ::warning(ErrorType::WARN_PARSER_TRANSITION,
                      "%1%: transition does not depend on select argument", expression->select);
            return cases.at(0)->state;
        }
        expression->selectCases = std::move(cases);
    }
    return expression;
}

}  // namespace P4
