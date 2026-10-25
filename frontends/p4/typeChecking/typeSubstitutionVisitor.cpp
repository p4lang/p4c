// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "typeSubstitutionVisitor.h"

namespace P4 {

bool TypeOccursVisitor::preorder(const IR::Type_Var *typeVariable) {
    if (*typeVariable == *(toFind->asType())) occurs = true;
    return occurs;
}

bool TypeOccursVisitor::preorder(const IR::Type_InfInt *typeVariable) {
    if (*typeVariable == *(toFind->asType())) occurs = true;
    return occurs;
}

const IR::Node *TypeVariableSubstitutionVisitor::preorder(IR::TypeParameters *tps) {
    for (auto it = tps->parameters.begin(); it != tps->parameters.end();) {
        const IR::Type *type = bindings->lookup(*it);
        if (type != nullptr && !replace) {
            // remove variables that are substituted
            LOG3("Removing from generic parameters " << *it);
            it = tps->parameters.erase(it);
        } else {
            if (type != nullptr)
                BUG_CHECK(type->is<IR::Type_Var>(),
                          "cannot replace a type parameter %1% with %2%:", *it, type);
            ++it;
        }
    }
    return tps;
}

const IR::Node *TypeVariableSubstitutionVisitor::replacement(const IR::ITypeVar *original,
                                                             const IR::Node *node) {
    LOG3("Visiting " << dbp(original));
    const IR::ITypeVar *current = original;
    const IR::Type *replacement = nullptr;
    while (true) {
        // This will end because there should be no circular chain of variables pointing
        // to each other.
        const IR::Type *type = bindings->lookup(current);
        if (type == nullptr) break;
        replacement = type;
        if (!type->is<IR::ITypeVar>()) break;
        current = type->to<IR::ITypeVar>();
    }
    if (replacement == nullptr) return node;
    LOG2("Replacing " << getOriginal() << " with " << replacement);
    return replacement;
}

}  // namespace P4
