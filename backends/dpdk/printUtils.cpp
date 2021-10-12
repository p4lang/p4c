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
cstring toStr(const IR::Constant *const c) {
    std::ostringstream out;
    out << "0x" << std::hex << c->value;
    return out.str();
}

cstring toDecimal(const IR::Constant*const c) {
    std::ostringstream out;
    out << c->value;
    return out.str();
}

cstring toStr(const IR::BoolLiteral *const b) {
    std::ostringstream out;
    out << b->value;
    return out.str();
}

cstring toStr(const IR::Member *const m) {
    std::ostringstream out;
    out << m->member.originalName;
    return toStr(m->expr) + "." + out.str();
}

cstring toStr(const IR::PathExpression *const p) { return p->path->name; }

cstring toStr(const IR::TypeNameExpression *const p) {
    return p->typeName->path->name;
}

cstring toStr(const IR::MethodCallExpression *const m) {
    if (auto path = m->method->to<IR::PathExpression>()) {
        return path->path->name.toString();
    } else {
        ::error("%1% is not a PathExpression", m->toString());
    }
    return "";
}

bool ConvertExprToString::preorder(const IR::Constant *e) {
    str = toStr(e);
    return false;
}

bool ConvertExprToString::preorder(const IR::BoolLiteral *e) {
    str = toStr(e);
    return false;
}

bool ConvertExprToString::preorder(const IR::Member *e){
    str = toStr(e);
    return false;
}
bool ConvertExprToString::preorder(const IR::PathExpression *e) {
    str = toStr(e);
    return false;
}
bool ConvertExprToString::preorder(const IR::TypeNameExpression *e) {
    str = toStr(e);
    return false;
}
bool ConvertExprToString::preorder(const IR::MethodCallExpression *e) {
    str = toStr(e);
    return false;
}

bool ConvertExprToString::preorder(const IR::Cast *e) {
    str = toStr(e->expr);
    return false;
}

bool ConvertExprToString::preorder(const IR::ArrayIndex *e) {
    if (auto cst = e->right->to<IR::Constant>()) {
        str = toStr(e->left) + "_" + toDecimal(cst);
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

cstring toStr(const IR::Type *const type) {
    if (type->is<IR::Type_Boolean>())
        return "bool";
    else if (auto b = type->to<IR::Type_Bits>()) {
        std::ostringstream out;
        out << "bit_" << b->width_bits();
        return out.str();
    } else if (auto n = type->to<IR::Type_Name>()) {
        return n->path->name;
    } else if (auto n = type->to<IR::Type_Specialized>()) {
        return n->baseType->path->name.name;
    } else {
        std::cerr << type->node_type_name() << std::endl;
        BUG("not implemented type");
    }
}
cstring toStr(const IR::PropertyValue *const property) {
    if (auto expr_value = property->to<IR::ExpressionValue>()) {
        return toStr(expr_value->expression);
    } else {
        std::cerr << property->node_type_name() << std::endl;
        BUG("not implemneted property value");
    }
}

} // namespace DPDK
