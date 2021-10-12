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

#ifndef BACKENDS_DPDK_PRINTUTILS_H_
#define BACKENDS_DPDK_PRINTUTILS_H_

#include "ir/ir.h"
#include "ir/dbprint.h"
#include <iostream>

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

class ConvertExprToString : public Inspector {
public:
    cstring str;
    bool preorder(const IR::Constant *c);
    bool preorder(const IR::BoolLiteral *b);
    bool preorder(const IR::Member *m);
    bool preorder(const IR::PathExpression *p);
    bool preorder(const IR::TypeNameExpression *p);
    bool preorder(const IR::MethodCallExpression *m);
    bool preorder(const IR::Cast *e);
    bool preorder(const IR::ArrayIndex *e);
};

class ConvertTypeToString : public Inspector {
public:
    cstring str;
    bool preorder(const IR::Type_Specialized *type);
    bool preorder(const IR::Type_Name *type);
    bool preorder(const IR::Type_Boolean *type);
    bool preorder(const IR::Type_Bits *type);
};

class ConvertPropertyValToString : public Inspector {
public:
    cstring str;
    bool preorder(const IR::ExpressionValue *property);
};
}// namespace DPDK

#endif
// BACKENDS_DPDK_PRINTUTILS_H_
