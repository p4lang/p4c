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
// this function takes different subclass of Expression and translate it into
// string in desired format. For example, for PathExpression, it returns
// PathExpression->path->name For Member, it returns
// toStr(Member->expr).Member->member
cstring toStr(const IR::Expression *const);

// this function takes different subclass of Type and translate it into string
// in desired format. For example, for Type_Boolean, it returns bool For
// Type_Bits, it returns bit_<bit_width>
cstring toStr(const IR::Type *const);

// this function takes different subclass of PropertyValue and translate it into
// string in desired format. For example, for ExpressionValue, it returns
// toStr(ExpressionValue->expression)
cstring toStr(const IR::PropertyValue *const);

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

cstring toStr(const IR::Expression *const exp) {
    if (auto e = exp->to<IR::Constant>())
        return toStr(e);
    else if (auto e = exp->to<IR::BoolLiteral>())
        return toStr(e);
    else if (auto e = exp->to<IR::Member>())
        return toStr(e);
    else if (auto e = exp->to<IR::PathExpression>())
        return toStr(e);
    else if (auto e = exp->to<IR::TypeNameExpression>())
        return toStr(e);
    else if (auto e = exp->to<IR::MethodCallExpression>())
        return toStr(e);
    else if (auto e = exp->to<IR::Cast>())
        return toStr(e->expr);
    else if (auto e = exp->to<IR::ArrayIndex>()) {
        if (auto cst = e->right->to<IR::Constant>()) {
            return toStr(e->left) + "_" + toDecimal(cst);
        } else {
            ::error("%1% is not a constant", e->right);
        }
    } else
        BUG("%1% not implemented", exp);
    return "";
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
