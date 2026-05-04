// Copyright 2021 Intel Corporation
// SPDX-FileCopyrightText: 2021 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "printUtils.h"

namespace P4::DPDK {

bool ConvertToString::preorder(const IR::Expression *e) {
    BUG("%1% not implemented", e);
    return false;
}

bool ConvertToString::preorder(const IR::Type *t) {
    BUG("Not implemented type %1%", t->node_type_name());
    return false;
}

bool ConvertToString::preorder(const IR::PropertyValue *p) {
    BUG("Not implemented property value %1%", p->node_type_name());
    return false;
}

bool ConvertToString::preorder(const IR::Constant *e) {
    out << "0x" << std::hex << std::uppercase << e->value;
    return false;
}

bool ConvertToString::preorder(const IR::BoolLiteral *e) {
    out << e->value;
    return false;
}

bool ConvertToString::preorder(const IR::Member *e) {
    out << toStr(e->expr) << "." << e->member.toString();
    return false;
}

bool ConvertToString::preorder(const IR::PathExpression *e) {
    out << e->path->name;
    return false;
}

bool ConvertToString::preorder(const IR::TypeNameExpression *e) {
    if (auto tn = e->typeName->to<IR::Type_Name>()) {
        out << tn->path->name;
    } else {
        BUG("%1%: unexpected typeNameExpression", e);
    }
    return false;
}

bool ConvertToString::preorder(const IR::MethodCallExpression *e) {
    out << "";
    if (auto path = e->method->to<IR::PathExpression>()) {
        out << path->path->name.name;
    } else {
        ::P4::error(ErrorType::ERR_INVALID, "%1% is not a PathExpression", e->toString());
    }
    return false;
}

bool ConvertToString::preorder(const IR::Cast *e) {
    out << toStr(e->expr);
    return false;
}

bool ConvertToString::preorder(const IR::ArrayIndex *e) {
    if (auto cst = e->right->to<IR::Constant>()) {
        out << toStr(e->left) << "_" << cst->value;
    } else {
        ::P4::error(ErrorType::ERR_INVALID, "%1% is not a constant", e->right);
    }
    return false;
}

bool ConvertToString::preorder(const IR::Type_Boolean * /*type*/) {
    out << "bool";
    return false;
}

bool ConvertToString::preorder(const IR::Type_Bits *type) {
    out << "bit_" << type->width_bits();
    return false;
}

bool ConvertToString::preorder(const IR::Type_Name *type) {
    out << type->path->name;
    return false;
}

bool ConvertToString::preorder(const IR::Type_Specialized *type) {
    out << type->baseType->path->name.name;
    return false;
}

bool ConvertToString::preorder(const IR::ExpressionValue *property) {
    out << toStr(property->expression);
    return false;
}

cstring toStr(const IR::Node *const n) {
    auto nodeToString = new ConvertToString;
    n->apply(*nodeToString);
    if (nodeToString->out.str() != "") {
        return nodeToString->out.str();
    } else {
        std::cerr << n->node_type_name() << std::endl;
        BUG("not implemented type");
    }
}
}  // namespace P4::DPDK
