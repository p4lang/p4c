// Copyright 2017 VMware, Inc.
// SPDX-FileCopyrightText: 2017 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "deprecated.h"

namespace P4 {

void Deprecated::warnIfDeprecated(const IR::IAnnotated *annotated, const IR::Node *errorNode) {
    if (annotated == nullptr) return;
    const auto *anno = annotated->getAnnotation(IR::Annotation::deprecatedAnnotation);
    if (anno == nullptr) return;

    std::string message;
    for (const auto *a : anno->getExpr()) {
        if (const auto *str = a->to<IR::StringLiteral>()) message += str->value;
    }
    ::P4::warning(ErrorType::WARN_DEPRECATED, "%1%: Using deprecated feature %2%. %3%", errorNode,
                  annotated->getNode(), message);
}

bool Deprecated::preorder(const IR::PathExpression *expression) {
    auto decl = getDeclaration(expression->path, true);
    warnIfDeprecated(decl->to<IR::IAnnotated>(), expression);
    return false;
}

bool Deprecated::preorder(const IR::Type_Name *name) {
    auto decl = getDeclaration(name->path, true);
    warnIfDeprecated(decl->to<IR::IAnnotated>(), name);
    return false;
}

}  // namespace P4
