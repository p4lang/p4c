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

#include <string>

#include <boost/multiprecision/number.hpp>

#include "ir/id.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/log.h"

namespace DPDK {

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
        ::error(ErrorType::ERR_INVALID, "%1% is not a PathExpression", e->toString());
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
        ::error(ErrorType::ERR_INVALID, "%1% is not a constant", e->right);
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
}  // namespace DPDK
