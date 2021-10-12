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

#include "printUtils.h"

namespace DPDK {

bool ConvertExprToString::preorder(const IR::Constant *e) {
    std::ostringstream out;
    out << "0x" << std::hex << e->value;
    str = out.str();
    return false;
}

bool ConvertExprToString::preorder(const IR::BoolLiteral *e) {
    std::ostringstream out;
    out << e->value;
    str = out.str();
    return false;
}

bool ConvertExprToString::preorder(const IR::Member *e){
    std::ostringstream out;
    out << e->member.originalName;
    str = toStr(e->expr) + "." + out.str();
    return false;
}

bool ConvertExprToString::preorder(const IR::PathExpression *e) {
    str = e->path->name;
    return false;
}

bool ConvertExprToString::preorder(const IR::TypeNameExpression *e) {
    str = e->typeName->path->name;
    return false;
}

bool ConvertExprToString::preorder(const IR::MethodCallExpression *e) {
    str = "";
    if (auto path = e->method->to<IR::PathExpression>()) {
        str = path->path->name.toString();
    } else {
        ::error("%1% is not a PathExpression", e->toString());
    }
    return false;
}

bool ConvertExprToString::preorder(const IR::Cast *e) {
    str = toStr(e->expr);
    return false;
}

bool ConvertExprToString::preorder(const IR::ArrayIndex *e) {
    if (auto cst = e->right->to<IR::Constant>()) {
        std::ostringstream out;
        out << cst->value;
        str = toStr(e->left) + "_" + out.str();
    } else {
        ::error("%1% is not a constant", e->right);
    }
    return false;
}

cstring toStr(const IR::Expression *const exp) {
    auto exprToString = new ConvertExprToString;
    exp->apply(*exprToString);
    return exprToString->str;
}

bool ConvertTypeToString::preorder(const IR::Type_Boolean * /*type*/) {
    str = "bool";
    return false;
}

bool ConvertTypeToString::preorder(const IR::Type_Bits *type) {
    std::ostringstream out;
    out << "bit_" << type->width_bits();
    str = out.str();
    return false;
}

bool ConvertTypeToString::preorder(const IR::Type_Name *type) {
    str = type->path->name;
    return false;
}

bool ConvertTypeToString::preorder(const IR::Type_Specialized *type) {
    str = type->baseType->path->name.name;
    return false;
}

cstring toStr(const IR::Type *const type) {
    auto typeToString = new ConvertTypeToString;
    type->apply(*typeToString);
    return typeToString->str;
}

bool ConvertPropertyValToString::preorder(const IR::ExpressionValue *property) {
    str = toStr(property->expression);
    return false;
}

cstring toStr(const IR::PropertyValue *const property) {
    auto propToString = new ConvertPropertyValToString;
    property->apply(*propToString);
    return propToString->str;
}
} // namespace DPDK
