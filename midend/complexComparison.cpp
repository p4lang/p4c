/*
Copyright 2017 VMware, Inc.

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

#include "complexComparison.h"

namespace P4 {

const IR::Expression* RemoveComplexComparisons::explode(
    Util::SourceInfo srcInfo,
    const IR::Type* leftType, const IR::Expression* left,
    const IR::Type* rightType, const IR::Expression* right) {

    // we allow several cases
    // header == header
    // header == list (tuple)
    // list == header
    // list == list
    // struct == struct
    // array == array
    // struct == list
    // list == struct

    auto rightTuple = rightType->to<IR::Type_Tuple>();
    auto leftTuple = leftType->to<IR::Type_Tuple>();
    if (leftTuple && !rightTuple)
        // put the tuple on the right if it is the only one,
        // so we handle fewer cases
        return explode(srcInfo, rightType, right, leftType, left);

    if (auto ht = leftType->to<IR::Type_Header>()) {
        auto lmethod = new IR::Member(left, IR::Type_Header::isValid);
        const IR::Expression* lvalid;
        if (left->is<IR::StructInitializerExpression>()) {
            // A header defined this way is always valid
            lvalid = new IR::BoolLiteral(true);
        } else {
            lvalid = new IR::MethodCallExpression(srcInfo, lmethod);
        }
        auto linvalid = new IR::LNot(srcInfo, lvalid);

        const IR::Expression* rvalid;
        if (!rightTuple) {
            if (right->is<IR::StructInitializerExpression>()) {
                rvalid = new IR::BoolLiteral(true);
            } else {
                auto rmethod = new IR::Member(right, IR::Type_Header::isValid);
                rvalid = new IR::MethodCallExpression(srcInfo, rmethod);
            }
        } else {
            rvalid = new IR::BoolLiteral(true);
        }

        auto rinvalid = new IR::LNot(srcInfo, rvalid);
        auto invalid = new IR::LAnd(srcInfo, linvalid, rinvalid);
        auto valid = new IR::LAnd(srcInfo, lvalid, rvalid);
        size_t index = 0;
        for (auto f : ht->fields) {
            auto ftype = f->type;
            auto fleft = new IR::Member(srcInfo, left, f->name);
            const IR::Expression* fright;
            const IR::Type* rightType;
            if (!rightTuple) {
                if (auto si = right->to<IR::StructInitializerExpression>()) {
                    auto nf = si->components.getDeclaration<IR::NamedExpression>(f->name);
                    CHECK_NULL(nf);
                    fright = nf->expression;
                } else {
                    fright = new IR::Member(srcInfo, right, f->name);
                }
                rightType = f->type;
            } else {
                fright = right->to<IR::ListExpression>()->components.at(index);
                rightType = rightTuple->components.at(index);
            }
            auto rec = explode(srcInfo, ftype, fleft, rightType, fright);
            valid = new IR::LAnd(srcInfo, valid, rec);
            index++;
        }
        auto result = new IR::LOr(srcInfo, invalid, valid);
        return result;
    } else if (auto st = leftType->to<IR::Type_StructLike>()) {
        // Works for structs and unions
        const IR::Expression* result = new IR::BoolLiteral(true);
        size_t index = 0;
        for (auto f : st->fields) {
            auto ftype = f->type;
            auto fleft = new IR::Member(srcInfo, left, f->name);
            const IR::Expression* fright;
            const IR::Type* rightType;
            if (!rightTuple) {
                if (auto si = right->to<IR::StructInitializerExpression>()) {
                    auto nf = si->components.getDeclaration<IR::NamedExpression>(f->name);
                    CHECK_NULL(nf);
                    fright = nf->expression;
                } else {
                    fright = new IR::Member(srcInfo, right, f->name);
                }
                rightType = f->type;
            } else {
                fright = right->to<IR::ListExpression>()->components.at(index);
                rightType = rightTuple->components.at(index);
            }
            index++;
            auto rec = explode(srcInfo, ftype, fleft, rightType, fright);
            result = new IR::LAnd(srcInfo, result, rec);
        }
        return result;
    } else if (auto at = leftType->to<IR::Type_Stack>()) {
        auto size = at->getSize();
        const IR::Expression* result = new IR::BoolLiteral(true);
        BUG_CHECK(rightType->is<IR::Type_Stack>(),
                  "%1%: comparing stack with %1%", left, rightType);
        for (unsigned i=0; i < size; i++) {
            auto index = new IR::Constant(i);
            auto lelem = new IR::ArrayIndex(srcInfo, left, index);
            auto relem = new IR::ArrayIndex(srcInfo, right, index);
            auto rec = explode(srcInfo, at->elementType, lelem, at->elementType, relem);
            result = new IR::LAnd(srcInfo, result, rec);
        }
        return result;
    } else if (leftTuple) {
        BUG_CHECK(rightTuple, "%1% vs %2%: unexpected comparison", left, right);
        const IR::Expression* result = new IR::BoolLiteral(true);
        auto leftList = left->to<IR::ListExpression>();
        for (size_t index = 0; index < leftList->components.size(); index++) {
            auto fleft = leftList->components.at(index);
            auto leftType = leftTuple->components.at(index);
            auto fright = right->to<IR::ListExpression>()->components.at(index);
            auto rightType = rightTuple->components.at(index);
            auto rec = explode(srcInfo, leftType, fleft, rightType, fright);
            result = new IR::LAnd(srcInfo, result, rec);
        }
        return result;
    } else {
        return new IR::Equ(srcInfo, left, right);
    }
}

const IR::Node* RemoveComplexComparisons::postorder(IR::Operation_Binary* expression) {
    if (!expression->is<IR::Neq>() && !expression->is<IR::Equ>())
        return expression;
    auto ltype = typeMap->getType(expression->left, true);
    auto rtype = typeMap->getType(expression->right, true);
    if (!ltype->is<IR::Type_StructLike>() && !ltype->is<IR::Type_Stack>() &&
        !ltype->is<IR::Type_Tuple>())
        return expression;
    auto result = explode(expression->srcInfo, ltype, expression->left, rtype, expression->right);
    if (expression->is<IR::Neq>())
        result = new IR::LNot(expression->srcInfo, result);
    return result;
}

}  // namespace P4
