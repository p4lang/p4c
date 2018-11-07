/*
Copyright 2018 VMware, Inc.

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

#include "structInitializers.h"

namespace P4 {

/// Given an expression and a destination type, convert ListExpressions
/// that occur within expression to StructInitializerExpression if the
/// destination type matches.
const IR::Expression*
convert(const IR::Expression* expression, const IR::Type* type) {
    bool modified = false;
    if (auto st = type->to<IR::Type_StructLike>()) {
        auto si = new IR::IndexedVector<IR::NamedExpression>();
        if (auto le = expression->to<IR::ListExpression>()) {
            size_t index = 0;
            for (auto f : st->fields) {
                auto expr = le->components.at(index);
                auto conv = convert(expr, f->type);
                auto ne = new IR::NamedExpression(conv->srcInfo, f->name, conv);
                si->push_back(ne);
                index++;
            }
            auto result = new IR::StructInitializerExpression(
                expression->srcInfo, st->name, *si, st->is<IR::Type_Header>());
            return result;
        } else if (auto sli = expression->to<IR::StructInitializerExpression>()) {
            for (auto f : st->fields) {
                auto ne = sli->components.getDeclaration<IR::NamedExpression>(f->name.name);
                BUG_CHECK(ne != nullptr, "%1%: no initializer for %2%", expression, f);
                auto convNe = convert(ne->expression, f->type);
                if (convNe != ne->expression)
                    modified = true;
                ne = new IR::NamedExpression(ne->srcInfo, f->name, convNe);
                si->push_back(ne);
            }
            if (modified) {
                auto result = new IR::StructInitializerExpression(
                    expression->srcInfo, st->name, *si, st->is<IR::Type_Header>());
                return result;
            }
        }
    } else if (auto tup = type->to<IR::Type_Tuple>()) {
        auto le = expression->to<IR::ListExpression>();
        if (le == nullptr)
            return expression;

        auto vec = new IR::Vector<IR::Expression>();
        for (size_t i = 0; i < le->size(); i++) {
            auto expr = le->components.at(i);
            auto type = tup->components.at(i);
            auto conv = convert(expr, type);
            vec->push_back(conv);
            modified |= (conv != expr);
        }
        if (modified) {
            auto result = new IR::ListExpression(expression->srcInfo, *vec);
            return result;
        }
    }
    return expression;
}

const IR::Node* CreateStructInitializers::postorder(IR::AssignmentStatement* statement) {
    auto type = typeMap->getType(statement->left);
    auto init = convert(statement->right, type);
    if (init != statement->right)
        statement->right = init;
    return statement;
}

const IR::Node* CreateStructInitializers::postorder(IR::MethodCallExpression* expression) {
    auto mi = MethodInstance::resolve(expression, refMap, typeMap);
    auto result = expression;
    auto convertedArgs = new IR::Vector<IR::Argument>();
    bool modified = false;
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        auto arg = mi->substitution.lookup(p);
        if (p->direction == IR::Direction::In ||
            p->direction == IR::Direction::None) {
            auto paramType = typeMap->getType(p, true);
            auto init = convert(arg->expression, paramType);
            CHECK_NULL(init);
            if (init != arg->expression) {
                modified = true;
                convertedArgs->push_back(new IR::Argument(arg->srcInfo, arg->name, init));
                continue;
            }
        }
        convertedArgs->push_back(arg);
    }
    if (modified) {
        LOG2("Converted some function arguments to struct initializers" << convertedArgs);
        return new IR::MethodCallExpression(
            result->srcInfo, result->method, result->typeArguments, convertedArgs);
    }
    return expression;
}


const IR::Node* CreateStructInitializers::postorder(IR::Operation_Relation* expression) {
    auto ltype = typeMap->getType(expression->left);
    auto rtype = typeMap->getType(expression->right);
    if (ltype->is<IR::Type_StructLike>() && rtype->is<IR::Type_Tuple>())
        expression->right = convert(expression->right, ltype);
    if (rtype->is<IR::Type_StructLike>() && ltype->is<IR::Type_Tuple>())
        expression->left = convert(expression->left, rtype);
    return expression;
}

}  // namespace P4
