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

#include "simplifySelectList.h"

namespace P4 {

void SubstituteStructures::explode(
    const IR::Expression* expression, const IR::Type* type,
    IR::Vector<IR::Expression>* output) {
    if (type->is<IR::Type_Struct>()) {
        auto st = type->to<IR::Type_Struct>();
        for (auto f : st->fields) {
            auto e = new IR::Member(expression, f->name);
            auto t = typeMap->getTypeType(f->type, true);
            explode(e, t, output);
        }
    } else {
        BUG_CHECK(!type->is<IR::Type_StructLike>() && !type->is<IR::Type_Stack>(),
                  "%1%: unexpected type", type);
        output->push_back(expression);
    }
}

const IR::Node* SubstituteStructures::postorder(IR::PathExpression* expression) {
    if (findContext<IR::SelectExpression>() == nullptr)
        return expression;
    if (getParent<IR::Member>() != nullptr)
        return expression;
    auto type = typeMap->getType(getOriginal(), true);
    if (!type->is<IR::Type_Struct>())
        return expression;
    auto result = new IR::ListExpression(expression->srcInfo, {});
    explode(expression, type, &result->components);
    LOG3("Replacing " << expression << " with " << result);
    return result;
}

void UnnestSelectList::flatten(const IR::Expression* expression,
                               IR::Vector<IR::Expression>* vec) {
    if (expression->is<IR::ListExpression>()) {
        nesting += '[';
        for (auto e : expression->to<IR::ListExpression>()->components)
            flatten(e, vec);
        nesting += ']';
    } else {
        vec->push_back(expression);
        nesting += '_';
    }
}

void UnnestSelectList::flatten(const IR::Expression* expression,
                               unsigned* nestingIndex,
                               IR::Vector<IR::Expression>* vec) {
    char c = nesting.get(*nestingIndex);
    if (expression->is<IR::ListExpression>()) {
        BUG_CHECK(c == '[', "%1%: expected [, got %2%", *nestingIndex, c);
        (*nestingIndex)++;
        for (auto e : expression->to<IR::ListExpression>()->components)
            flatten(e, nestingIndex, vec);
        char c = nesting.get(*nestingIndex);
        BUG_CHECK(c == ']', "%1%: expected ], got %2%", *nestingIndex, c);
        (*nestingIndex)++;
    } else if (expression->is<IR::DefaultExpression>()) {
        unsigned depth = 0;
        do {
            if (c == '[') {
                depth++;
            } else if (c == ']') {
                depth--;
            } else {
                vec->push_back(new IR::DefaultExpression(expression->srcInfo));
            }
            (*nestingIndex)++;
            c = nesting.get(*nestingIndex);
        } while (depth > 0);
    } else {
        BUG_CHECK(c == '_', "%1%: expected _, got %2%", *nestingIndex, c);
        vec->push_back(expression);
        (*nestingIndex)++;
    }
}

const IR::Node* UnnestSelectList::preorder(IR::SelectExpression* expression) {
    IR::Vector<IR::Expression> vec;
    nesting = "";
    flatten(expression->select, &vec);
    if (nesting.findlast(']') == nesting.c_str())
        // no nested lists found
        return expression;
    expression->select = new IR::ListExpression(expression->select->srcInfo, vec);
    LOG3("Flattened select list to " << expression->select << ": " << nesting);
    for (auto it = expression->selectCases.begin(); it != expression->selectCases.end(); ++it) {
        auto sc = *it;
        auto keyset = sc->keyset;
        if (keyset->is<IR::ListExpression>()) {
            IR::Vector<IR::Expression> vec;
            unsigned index = 0;
            flatten(sc->keyset, &index, &vec);
            keyset = new IR::ListExpression(keyset->srcInfo, vec);
            LOG3("Flattened select case to " << keyset);
            *it = new IR::SelectCase(sc->srcInfo, keyset, sc->state);
        }
        // else leave keyset unchanged
    }
    return expression;
}

}  // namespace P4
