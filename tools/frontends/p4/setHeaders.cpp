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
    const IR::Expression* dest, const IR::Expression* src,
    const IR::Type* destType,
    IR::Vector<IR::StatOrDecl>* insert) {

    auto structType = destType->to<IR::Type_StructLike>();
    if (structType == nullptr)
        return;

    auto srcType = typeMap->getType(src, true);
    auto list = src->to<IR::ListExpression>();
    auto si = src->to<IR::StructInitializerExpression>();
    if (list == nullptr && si == nullptr)
        return;

    if (structType->is<IR::Type_Header>()) {
        LOG3("Inserting setValid for " << dest);
        auto method = new IR::Member(dest->srcInfo, dest, IR::Type_Header::setValid);
        auto mc = new IR::MethodCallExpression(
            dest->srcInfo, method, new IR::Vector<IR::Argument>());
        auto stat = new IR::MethodCallStatement(mc->srcInfo, mc);
        insert->push_back(stat);
        return;
    }

    // Recurse on fields of structType
    if (list != nullptr) {
        auto tt = srcType->to<IR::Type_Tuple>();
        CHECK_NULL(tt);
        auto it = list->components.begin();
        for (auto f : structType->fields) {
            auto member = new IR::Member(dest, f->name);
            auto ft = typeMap->getType(f);
            generateSetValid(member, *it, ft, insert);
            ++it;
        }
        return;
    }

    CHECK_NULL(si);
    for (auto f : structType->fields) {
        auto member = new IR::Member(dest, f->name);
        auto srcMember = si->components.getDeclaration<IR::NamedExpression>(f->name);
        auto ft = typeMap->getType(f);
        CHECK_NULL(srcMember);
        generateSetValid(member, srcMember->expression, ft, insert);
    }
}

const IR::Node* DoSetHeaders::postorder(IR::AssignmentStatement* statement) {
    auto vec = new IR::Vector<IR::StatOrDecl>();
    auto destType = typeMap->getType(statement->left, true);
    generateSetValid(statement->left, statement->right, destType, vec);
    if (vec->empty())
        return statement;
    vec->push_back(statement);
    return vec;
}

}  // namespace P4
