/*
Copyright 2017 VMware, Inc.

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

#include "setHeaders.h"

namespace P4 {

void DoSetHeaders::generateSetValid(
    const IR::Type* destType, const IR::Type* srcType,
    const IR::Expression* dest, IR::Vector<IR::StatOrDecl>* insert) {
    auto tt = srcType->to<IR::Type_Tuple>();
    if (tt == nullptr)
        return;

    // The source is either a tuple, or a list expression
    if (destType->is<IR::Type_Struct>()) {
        auto it = tt->components.begin();
        auto sl = destType->to<IR::Type_Struct>();
        for (auto f : sl->fields) {
            auto ftype = typeMap->getType(f, true);
            auto stype = *it;
            auto member = new IR::Member(dest, f->name);
            generateSetValid(ftype, stype, member, insert);
            ++it;
        }
    } else if (destType->is<IR::Type_Header>()) {
        LOG3("Inserting setValid for " << dest);
        auto method = new IR::Member(dest->srcInfo, dest, IR::Type_Header::setValid);
        auto mc = new IR::MethodCallExpression(
            dest->srcInfo, method, new IR::Vector<IR::Expression>());
        auto stat = new IR::MethodCallStatement(mc->srcInfo, mc);
        insert->push_back(stat);
    }
}

const IR::Node* DoSetHeaders::postorder(IR::AssignmentStatement* statement) {
    auto vec = new IR::Vector<IR::StatOrDecl>();
    auto ltype = typeMap->getType(statement->left, true);
    auto rtype = typeMap->getType(statement->right, true);
    generateSetValid(ltype, rtype, statement->left, vec);
    if (vec->empty())
        return statement;
    vec->push_back(statement);
    return vec;
}

}  // namespace P4
