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

#include "syntacticEquivalence.h"

namespace P4 {

bool SameExpression::sameType(const IR::Type* left, const IR::Type* right) const {
    auto lt = typeMap->getType(left, true);
    auto rt = typeMap->getType(right, true);
    return TypeMap::equivalent(lt, rt);
}

bool SameExpression::sameExpressions(const IR::Vector<IR::Expression>* left,
                                     const IR::Vector<IR::Expression>* right) const {
    if (left->size() != right->size())
        return false;
    for (unsigned i = 0; i < left->size(); i++)
        if (!sameExpression(left->at(i), right->at(i)))
            return false;
    return true;
}

bool SameExpression::sameExpressions(const IR::Vector<IR::Argument>* left,
                                     const IR::Vector<IR::Argument>* right) const {
    if (left->size() != right->size())
        return false;
    for (unsigned i = 0; i < left->size(); i++)
        if (!sameExpression(left->at(i)->expression, right->at(i)->expression))
            return false;
    return true;
}

bool SameExpression::sameExpression(const IR::Expression* left, const IR::Expression* right) const {
    CHECK_NULL(left); CHECK_NULL(right);
    if (left->node_type_name() != right->node_type_name())
        return false;
    if (left->is<IR::Operation_Unary>()) {
        auto lu = left->to<IR::Operation_Unary>();
        auto ru = right->to<IR::Operation_Unary>();
        if (left->is<IR::Member>()) {
            auto lm = left->to<IR::Member>();
            auto rm = right->to<IR::Member>();
            if (lm->member != rm->member)
                return false;
        } else if (left->is<IR::Cast>()) {
            auto lc = left->to<IR::Cast>();
            auto rc = right->to<IR::Cast>();
            if (!sameType(lc->type, rc->type))
                return false;
        }
        return sameExpression(lu->expr, ru->expr);
    } else if (left->is<IR::Operation_Binary>()) {
        auto lb = left->to<IR::Operation_Binary>();
        auto rb = right->to<IR::Operation_Binary>();
        return sameExpression(lb->left, rb->left) && sameExpression(lb->right, rb->right);
    } else if (left->is<IR::Operation_Ternary>()) {
        auto lt = left->to<IR::Operation_Ternary>();
        auto rt = right->to<IR::Operation_Ternary>();
        return sameExpression(lt->e0, rt->e0) &&
                sameExpression(lt->e1, rt->e1) &&
                sameExpression(lt->e2, rt->e2);
    } else if (left->is<IR::Constant>()) {
        return left->to<IR::Constant>()->value == right->to<IR::Constant>()->value;
    } else if (left->is<IR::Literal>()) {
        return *left == *right;
    } else if (left->is<IR::PathExpression>()) {
        auto ld = refMap->getDeclaration(left->to<IR::PathExpression>()->path, true);
        auto rd = refMap->getDeclaration(right->to<IR::PathExpression>()->path, true);
        return ld == rd;
    } else if (left->is<IR::TypeNameExpression>()) {
        auto ld = refMap->getDeclaration(left->to<IR::TypeNameExpression>()->typeName->path, true);
        auto rd = refMap->getDeclaration(right->to<IR::TypeNameExpression>()->typeName->path, true);
        return ld == rd;
    } else if (left->is<IR::ListExpression>()) {
        auto ll = left->to<IR::ListExpression>();
        auto rl = right->to<IR::ListExpression>();
        return sameExpressions(&ll->components, &rl->components);
    } else if (left->is<IR::MethodCallExpression>()) {
        auto lm = left->to<IR::MethodCallExpression>();
        auto rm = right->to<IR::MethodCallExpression>();
        if (!sameExpression(lm->method, rm->method))
            return false;
        if (lm->typeArguments->size() != rm->typeArguments->size())
            return false;
        for (unsigned i = 0; i < lm->typeArguments->size(); i++)
            if (!sameType(lm->typeArguments->at(i), rm->typeArguments->at(i)))
                return false;
        return sameExpressions(lm->arguments, rm->arguments);
    } else if (left->is<IR::ConstructorCallExpression>()) {
        auto lc = left->to<IR::ConstructorCallExpression>();
        auto rc = right->to<IR::ConstructorCallExpression>();
        if (!sameType(lc->constructedType, rc->constructedType))
            return false;
        return sameExpressions(lc->arguments, rc->arguments);
    } else {
        BUG("%1%: Unexpected expression", left);
    }
}


}  // namespace P4
