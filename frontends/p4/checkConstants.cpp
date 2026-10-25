// Copyright 2017 VMware, Inc.
// SPDX-FileCopyrightText: 2017 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "checkConstants.h"

#include "methodInstance.h"

namespace P4 {

void DoCheckConstants::postorder(const IR::MethodCallExpression *expression) {
    auto mi = MethodInstance::resolve(expression, this, typeMap);
    if (auto bi = mi->to<BuiltInMethod>()) {
        if (bi->name == IR::Type_Array::push_front || bi->name == IR::Type_Array::pop_front) {
            BUG_CHECK(expression->arguments->size() == 1, "Expected 1 argument for %1%",
                      expression);
            auto arg0 = expression->arguments->at(0)->expression;
            if (!arg0->is<IR::Constant>())
                ::P4::error(ErrorType::ERR_INVALID, "%1%: argument must be a constant", arg0);
        }
    }
}

void DoCheckConstants::postorder(const IR::KeyElement *key) {
    if (key->expression->is<IR::Literal>())
        warn(ErrorType::WARN_MISMATCH, "%1%: Constant key field", key->expression);
}

void DoCheckConstants::postorder(const IR::P4Table *table) {
    // This will print an error if the property exists and is not an integer
    (void)table->getSizeProperty();
}

}  // namespace P4
