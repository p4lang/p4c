// Copyright 2017 VMware, Inc.
// SPDX-FileCopyrightText: 2017 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "removeSelectBooleans.h"

namespace P4 {

// cast all boolean values to bit values, but only at the top-level
const IR::Expression *DoRemoveSelectBooleans::addToplevelCasts(const IR::Expression *expression) {
    if (expression->is<IR::ListExpression>()) {
        IR::Vector<IR::Expression> vec;
        bool changes = false;
        auto list = expression->to<IR::ListExpression>();
        for (auto e : list->components) {
            auto type = typeMap->getType(e, true);
            if (type->is<IR::Type_Boolean>()) {
                changes = true;
                auto cast = new IR::Cast(IR::Type_Bits::get(1), e);
                vec.push_back(cast);
            } else {
                vec.push_back(e);
            }
        }
        if (changes) return new IR::ListExpression(expression->srcInfo, vec);
        return expression;
    } else {
        auto type = typeMap->getType(expression, true);
        if (type->is<IR::Type_Boolean>())
            expression = new IR::Cast(IR::Type_Bits::get(1), expression);
        return expression;
    }
}

const IR::Node *DoRemoveSelectBooleans::postorder(IR::SelectExpression *expression) {
    auto e = addToplevelCasts(expression->select);
    expression->select = e->to<IR::ListExpression>();
    return expression;
}

const IR::Node *DoRemoveSelectBooleans::postorder(IR::SelectCase *selectCase) {
    // here we rely on the fact that Range and Mask expressions cannot
    // be applied to booleans
    selectCase->keyset = addToplevelCasts(selectCase->keyset);
    return selectCase;
}

}  // namespace P4
