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

#include <iostream>

#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace DPDK {
// this function translates nodes of different subclass of Expression, Type
// and PropertyValue in desired format. For example, for PathExpression, it returns
// PathExpression->path->name For Member, it returns toStr(Member->expr).Member->member etc.
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
}  // namespace DPDK

#endif /* BACKENDS_DPDK_PRINTUTILS_H_ */
