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
    const IR::Type* type, const IR::Expression* left, const IR::Expression* right) {
    if (auto ht = type->to<IR::Type_Header>()) {
        auto lmethod = new IR::Member(left, IR::Type_Header::isValid);
        auto lvalid = new IR::MethodCallExpression(srcInfo, lmethod);
        auto rmethod = new IR::Member(right, IR::Type_Header::isValid);
        auto rvalid = new IR::MethodCallExpression(srcInfo, rmethod);

        auto linvalid = new IR::LNot(srcInfo, lvalid);
        auto rinvalid = new IR::LNot(srcInfo, rvalid);
        auto invalid = new IR::LAnd(srcInfo, linvalid, rinvalid);

        auto valid = new IR::LAnd(srcInfo, lvalid, rvalid);
        for (auto f : ht->fields) {
            auto ftype = f->type;
            auto fleft = new IR::Member(srcInfo, left, f->name);
            auto fright = new IR::Member(srcInfo, right, f->name);
            auto rec = explode(srcInfo, ftype, fleft, fright);
            valid = new IR::LAnd(srcInfo, valid, rec);
        }
        auto result = new IR::LOr(srcInfo, invalid, valid);
        return result;
    } else if (auto st = type->to<IR::Type_StructLike>()) {
        // Works for structs and unions
        const IR::Expression* result = new IR::BoolLiteral(true);
        for (auto f : st->fields) {
            auto ftype = f->type;
            auto fleft = new IR::Member(srcInfo, left, f->name);
            auto fright = new IR::Member(srcInfo, right, f->name);
            auto rec = explode(srcInfo, ftype, fleft, fright);
            result = new IR::LAnd(srcInfo, result, rec);
        }
        return result;
    } else if (auto at = type->to<IR::Type_Stack>()) {
        auto size = at->getSize();
        const IR::Expression* result = new IR::BoolLiteral(true);
        for (unsigned i=0; i < size; i++) {
            auto index = new IR::Constant(i);
            auto lelem = new IR::ArrayIndex(srcInfo, left, index);
            auto relem = new IR::ArrayIndex(srcInfo, right, index);
            auto rec = explode(srcInfo, at->elementType, lelem, relem);
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
    if (!ltype->is<IR::Type_StructLike>() && !ltype->is<IR::Type_Stack>())
        return expression;
    auto result = explode(expression->srcInfo, ltype, expression->left, expression->right);
    if (expression->is<IR::Neq>())
        result = new IR::LNot(expression->srcInfo, result);
    return result;
}

}  // namespace P4
