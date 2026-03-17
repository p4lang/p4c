/*
Copyright 2023 VMWare, Inc.

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

#include "defaultValues.h"

#include "ir/irutils.h"

namespace P4 {

const IR::Expression *DoDefaultValues::defaultValue(const IR::Expression *expression,
                                                    const IR::Type *type) {
    if (const auto *anyType = type->to<IR::Type_Any>()) {
        type = typeMap->getSubstitution(anyType->to<IR::Type_Any>());
        if (type == nullptr) {
            ::P4::error(ErrorType::ERR_TYPE_ERROR, "%1%: could not find default value", expression);
            return expression;
        }
    }
    return IR::getDefaultValue(type, expression->srcInfo, false);
}

const IR::Node *DoDefaultValues::postorder(IR::Dots *expression) {
    auto parent = getContext()->node;
    if (parent->is<IR::ListExpression>() || parent->is<IR::ArrayExpression>())
        // Handled by the parent in postorder
        return expression;
    auto type = typeMap->getType(getOriginal(), true);
    auto result = defaultValue(expression, type);
    if (!result) return expression;
    typeMap->setType(result, type);
    return result;
}

const IR::Node *DoDefaultValues::postorder(IR::StructExpression *expression) {
    if (expression->containsDots()) {
        auto namedDots = expression->components.at(expression->size() - 1);
        auto vec = IR::IndexedVector<IR::NamedExpression>();
        for (size_t i = 0; i < expression->size() - 1; i++)
            vec.push_back(expression->components.at(i));  // skip '...'

        auto dotsType = typeMap->getType(namedDots->expression, true);
        auto result = defaultValue(expression, dotsType);
        if (result == nullptr) return expression;
        auto se = result->to<IR::StructExpression>();
        CHECK_NULL(se);
        vec.append(se->components);
        auto st = expression->structType;
        return new IR::StructExpression(expression->srcInfo, st, st, vec);
    }
    return expression;
}

const IR::Node *DoDefaultValues::postorder(IR::ListExpression *expression) {
    if (expression->containsDots()) {
        auto dots = expression->components.at(expression->size() - 1);
        auto vec = IR::Vector<IR::Expression>();
        for (size_t i = 0; i < expression->size() - 1; i++)
            vec.push_back(expression->components.at(i));  // skip '...'

        auto dotsType = typeMap->getType(dots, true);
        auto result = defaultValue(expression, dotsType);
        if (result == nullptr) return expression;
        auto se = result->to<IR::ListExpression>();
        CHECK_NULL(se);
        vec.append(se->components);
        return new IR::ListExpression(expression->srcInfo, vec);
    }
    return expression;
}

const IR::Node *DoDefaultValues::postorder(IR::ArrayExpression *expression) {
    if (expression->containsDots()) {
        auto dots = expression->components.at(expression->size() - 1);
        auto vec = IR::Vector<IR::Expression>();
        for (size_t i = 0; i < expression->size() - 1; i++)
            vec.push_back(expression->components.at(i));  // skip '...'

        auto expressionType = typeMap->getType(getOriginal(), true);
        auto arrayType = expressionType->to<IR::Type_Array>();
        BUG_CHECK(arrayType, "%1%: expected an array type", expressionType);
        auto dotsType = typeMap->getType(dots, true);
        auto result = defaultValue(expression, dotsType);
        if (result == nullptr) return expression;
        auto ae = result->to<IR::ArrayExpression>();
        CHECK_NULL(ae);
        vec.append(ae->components);
        return new IR::ArrayExpression(expression->srcInfo, arrayType->getP4Type(), vec,
                                       arrayType->getP4Type());
    }
    return expression;
}

}  // namespace P4
