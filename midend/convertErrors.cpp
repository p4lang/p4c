/* Copyright 2021 Intel Corporation

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

#include "convertErrors.h"

namespace P4 {

const IR::Node* DoConvertErrors::preorder(IR::Type_Error* type) {
    bool convert = policy->convert(type);
    if (!convert)
        return type;
    unsigned long long count = type->members.size();
    unsigned long long width = policy->errorSize(count);
    LOG2("Converting error " << type->name << " to " << "bit<" << width << ">");
    BUG_CHECK(count <= (1ULL << width),
              "%1%: not enough bits to represent %2%", width, type);
    errorRepr = new ErrorRepresentation(type->srcInfo, width);
    for (auto d : type->members)
        errorRepr->add(d->name.name);
    return new IR::Type_Bits(width, false);
}

const IR::Node* DoConvertErrors::postorder(IR::Type_Name* type) {
    if (type->toString() == IR::Type_Error::error) {
        if (findContext<IR::TypeNameExpression>() != nullptr)
            // This will be resolved by the caller.
            return type;
        else
            return new IR::Type_Bits(16, false);
    }
    return type;
}

/// process error expression, e.g., X.a
const IR::Node* DoConvertErrors::postorder(IR::Member* expression) {
    if (expression->expr->toString() == "error") {
        auto value = errorRepr->get(expression->member.name);
        auto cst = new IR::Constant(expression->srcInfo, errorRepr->type, value);
        return cst;
    }
    return expression;
}

}  // namespace P4
