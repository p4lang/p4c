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

namespace P4 {

const IR::Expression *DoDefaultValues::defaultValue(const IR::Expression *expression,
                                                    const IR::Type *type) {
    Util::SourceInfo srcInfo = expression->srcInfo;
    if (auto anyType = type->to<IR::Type_Any>()) {
        type = typeMap->getSubstitution(anyType->to<IR::Type_Any>());
        if (!type) {
            ::error(ErrorType::ERR_TYPE_ERROR, "%1%: could not find default value", expression);
            return expression;
        }
    }

    if (auto tb = type->to<IR::Type_Bits>()) {
        return new IR::Constant(srcInfo, tb, 0);
    } else if (type->is<IR::Type_InfInt>()) {
        return new IR::Constant(srcInfo, 0);
    } else if (type->is<IR::Type_Boolean>()) {
        return new IR::BoolLiteral(srcInfo, false);
    } else if (auto te = type->to<IR::Type_Enum>()) {
        return new IR::Member(srcInfo, new IR::TypeNameExpression(te->name),
                              te->members.at(0)->getName());
    } else if (auto te = type->to<IR::Type_SerEnum>()) {
        return new IR::Cast(srcInfo, type->getP4Type(), new IR::Constant(srcInfo, te->type, 0));
    } else if (auto te = type->to<IR::Type_Error>()) {
        return new IR::Member(srcInfo, new IR::TypeNameExpression(te->name), "NoError");
    } else if (type->is<IR::Type_String>()) {
        return new IR::StringLiteral(srcInfo, cstring(""));
    } else if (type->is<IR::Type_Varbits>()) {
        ::error(ErrorType::ERR_UNSUPPORTED, "%1% default values for varbit types", expression);
        return nullptr;
    } else if (auto ht = type->to<IR::Type_Header>()) {
        return new IR::InvalidHeader(ht->getP4Type());
    } else if (auto hu = type->to<IR::Type_HeaderUnion>()) {
        return new IR::InvalidHeaderUnion(hu->getP4Type());
    } else if (auto st = type->to<IR::Type_StructLike>()) {
        auto vec = new IR::IndexedVector<IR::NamedExpression>();
        for (auto field : st->fields) {
            auto value = defaultValue(expression, field->type);
            if (!value) return nullptr;
            vec->push_back(new IR::NamedExpression(field->name, value));
        }
        auto resultType = st->getP4Type();
        return new IR::StructExpression(srcInfo, resultType, resultType, *vec);
    } else if (auto tf = type->to<IR::Type_Fragment>()) {
        return defaultValue(expression, tf->type);
    } else if (auto tt = type->to<IR::Type_BaseList>()) {
        auto vec = new IR::Vector<IR::Expression>();
        for (auto field : tt->components) {
            auto value = defaultValue(expression, field);
            if (!value) return nullptr;
            vec->push_back(value);
        }
        return new IR::ListExpression(srcInfo, *vec);
    } else if (auto ts = type->to<IR::Type_Stack>()) {
        auto vec = new IR::Vector<IR::Expression>();
        auto elementType = ts->elementType;
        for (size_t i = 0; i < ts->getSize(); i++) {
            const IR::Expression *invalid;
            if (elementType->is<IR::Type_Header>()) {
                invalid = new IR::InvalidHeader(elementType->getP4Type());
            } else {
                BUG_CHECK(elementType->is<IR::Type_HeaderUnion>(),
                          "%1%: expected a header or header union stack", elementType);
                invalid = new IR::InvalidHeaderUnion(srcInfo, elementType->getP4Type());
            }
            vec->push_back(invalid);
        }
        auto resultType = ts->getP4Type();
        return new IR::HeaderStackExpression(srcInfo, resultType, *vec, resultType);
    } else {
        ::error(ErrorType::ERR_INVALID, "%1%: No default value for type %2%", expression, type);
        return nullptr;
    }
}

const IR::Node *DoDefaultValues::postorder(IR::Dots *expression) {
    auto parent = getContext()->node;
    if (parent->is<IR::ListExpression>() || parent->is<IR::HeaderStackExpression>())
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

const IR::Node *DoDefaultValues::postorder(IR::HeaderStackExpression *expression) {
    if (expression->containsDots()) {
        auto dots = expression->components.at(expression->size() - 1);
        auto vec = IR::Vector<IR::Expression>();
        for (size_t i = 0; i < expression->size() - 1; i++)
            vec.push_back(expression->components.at(i));  // skip '...'

        auto expressionType = typeMap->getType(getOriginal(), true);
        auto stackType = expressionType->to<IR::Type_Stack>();
        BUG_CHECK(stackType, "%1%: expected a stack type", expressionType);
        auto dotsType = typeMap->getType(dots, true);
        auto result = defaultValue(expression, dotsType);
        if (result == nullptr) return expression;
        auto se = result->to<IR::HeaderStackExpression>();
        CHECK_NULL(se);
        vec.append(se->components);
        return new IR::HeaderStackExpression(expression->srcInfo, stackType->getP4Type(), vec,
                                             stackType->getP4Type());
    }
    return expression;
}

}  // namespace P4
