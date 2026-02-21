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
#include "lib/algorithm.h"

namespace P4 {

bool isCompileTimeConstant(const IR::Expression *expr, const TypeMap *typeMap) {
    if (expr->is<IR::Constant>()) return true;
    if (expr->is<IR::BoolLiteral>()) return true;

    if (auto list = expr->to<IR::ListExpression>()) {
        const auto &components = list->components;
        return std::all_of(components.begin(), components.end(), [&](const IR::Expression *e) {
            return isCompileTimeConstant(e, typeMap);
        });
    }

    if (EnumInstance::resolve(expr, typeMap)) return true;

    return false;
}

void DoSimplifySelectCases::checkSimpleConstant(const IR::Expression *expr) const {
    CHECK_NULL(expr);
    if (expr->is<IR::DefaultExpression>()) return;
    if (expr->is<IR::Constant>()) return;
    if (expr->is<IR::BoolLiteral>()) return;
    if (expr->is<IR::Mask>() || expr->is<IR::Range>()) {
        auto bin = expr->to<IR::Operation_Binary>();
        checkSimpleConstant(bin->left);
        checkSimpleConstant(bin->right);
        return;
    }
    if (expr->is<IR::ListExpression>()) {
        auto list = expr->to<IR::ListExpression>();
        for (auto e : list->components) checkSimpleConstant(e);
        return;
    }
    auto ei = EnumInstance::resolve(expr, typeMap);
    if (ei != nullptr) return;

    // we allow value_set name to be used in place of select case;
    if (expr->is<IR::PathExpression>()) {
        auto type = typeMap->getType(expr);
        if (type->is<IR::Type_Set>()) {
            return;
        }
    }
    ::P4::error(ErrorType::ERR_INVALID, "%1%: must be a compile-time constant", expr);
}

const IR::Node *DoSimplifySelectCases::preorder(IR::SelectExpression *expression) {
    IR::Vector<IR::SelectCase> cases;

    bool seenDefault = false;
    bool changes = false;
    for (auto c : expression->selectCases) {
        if (seenDefault) {
            warn(ErrorType::WARN_PARSER_TRANSITION, "%1%: unreachable", c);
            changes = true;
            continue;
        }
        cases.push_back(c);
        if (c->keyset->is<IR::DefaultExpression>()) seenDefault = true;
        if (requireConstants) checkSimpleConstant(c->keyset);
    }

    bool allConst = std::all_of(cases.begin(), cases.end(), [&](const IR::SelectCase *c) {
        if (!typeMap->isCompileTimeConstant(c->keyset)) return false;

        return isCompileTimeConstant(c->keyset, typeMap) || c->keyset->is<IR::DefaultExpression>();
    });
    // Remove all duplicated select cases by keyset.
    if (allConst) {
        IR::Vector<IR::SelectCase> tmp;
        for (auto c : cases) {  // exclude last
            auto pred = [&](const IR::SelectCase *other) {
                return c->keyset->equiv(*other->keyset);
            };
            if (!contains_if(tmp, pred)) tmp.push_back(c);
        }
        if (cases.size() != tmp.size()) {
            cases = tmp;
            changes = true;
        }
    }
    // Remove all duplicated select cases equal to default state.
    if (seenDefault && allConst) {
        auto excludeDefault = std::prev(cases.end());
        auto state = getDeclaration(cases.back()->state->path)->to<IR::ParserState>();
        auto it = std::remove_if(cases.begin(), excludeDefault, [&](const IR::SelectCase *c) {
            return state == getDeclaration(c->state->path)->to<IR::ParserState>();
        });
        changes |= it != excludeDefault;
        cases.erase(it, excludeDefault);
    }

    if (changes) {
        if (cases.size() == 1) {
            // just one default label
            warn(ErrorType::WARN_PARSER_TRANSITION,
                 "'%1%': transition does not depend on select argument", expression->select);
            return cases.at(0)->state;
        }
        expression->selectCases = std::move(cases);
    }
    return expression;
}

}  // namespace P4
