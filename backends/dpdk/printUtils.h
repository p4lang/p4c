/*
 * Copyright 2021 Intel Corporation
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_DPDK_PRINTUTILS_H_
#define BACKENDS_DPDK_PRINTUTILS_H_

#include <iostream>

#include "ir/dbprint.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4::DPDK {
/// This function translates nodes of different subclass of Expression, Type
/// and PropertyValue in desired format. For example, for PathExpression, it returns
/// PathExpression->path->name For Member, it returns toStr(Member->expr).Member->member etc.
cstring toStr(const IR::Node *const);

class ConvertToString : public Inspector {
 public:
    std::ostringstream out;
    bool preorder(const IR::Expression *e);
    bool preorder(const IR::Type *t);
    bool preorder(const IR::PropertyValue *p);
    bool preorder(const IR::Constant *c);
    bool preorder(const IR::BoolLiteral *b);
    bool preorder(const IR::Member *m);
    bool preorder(const IR::PathExpression *p);
    bool preorder(const IR::TypeNameExpression *p);
    bool preorder(const IR::MethodCallExpression *m);
    bool preorder(const IR::Cast *e);
    bool preorder(const IR::ArrayIndex *e);
    bool preorder(const IR::Type_Specialized *type);
    bool preorder(const IR::Type_Name *type);
    bool preorder(const IR::Type_Boolean *type);
    bool preorder(const IR::Type_Bits *type);
    bool preorder(const IR::ExpressionValue *property);
};
}  // namespace P4::DPDK

#endif /* BACKENDS_DPDK_PRINTUTILS_H_ */
