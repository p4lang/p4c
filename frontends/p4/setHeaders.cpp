// Copyright 2017 VMware, Inc.
// SPDX-FileCopyrightText: 2017 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "setHeaders.h"

namespace P4 {

void DoSetHeaders::generateSetValid(const IR::Expression *dest, const IR::Expression *src,
                                    const IR::Type *destType, IR::Vector<IR::StatOrDecl> &insert) {
    auto structType = destType->to<IR::Type_StructLike>();
    if (structType == nullptr) return;

    auto srcType = typeMap->getType(src, true);
    auto list = src->to<IR::ListExpression>();
    auto si = src->to<IR::StructExpression>();
    if (list == nullptr && si == nullptr) return;

    if (structType->is<IR::Type_Header>()) {
        LOG3("Inserting setValid for " << dest);
        auto srcInfo = dest->srcInfo;
        if (!srcInfo) srcInfo = src->srcInfo;
        auto method = new IR::Member(srcInfo, dest, IR::Type_Header::setValid);
        auto mc = new IR::MethodCallExpression(srcInfo, method, new IR::Vector<IR::Argument>());
        auto stat = new IR::MethodCallStatement(srcInfo, mc);
        insert.push_back(stat);
        return;
    }

    // Recurse on fields of structType
    if (list != nullptr) {
        auto tt = srcType->to<IR::Type_BaseList>();
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

const IR::Node *DoSetHeaders::postorder(IR::AssignmentStatement *statement) {
    IR::IndexedVector<IR::StatOrDecl> vec;
    auto destType = typeMap->getType(statement->left, true);
    generateSetValid(statement->left, statement->right, destType, vec);
    if (vec.empty()) return statement;
    vec.push_back(statement);
    return new IR::BlockStatement(statement->srcInfo, vec);
}

}  // namespace P4
