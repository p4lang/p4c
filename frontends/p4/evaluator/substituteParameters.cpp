// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "substituteParameters.h"

namespace P4 {

bool SubstituteParameters::useParameterSourceInfo(const IR::Expression *replacement) const {
    if (!replacement->srcInfo.isValid()) return true;
    auto path = replacement->to<IR::PathExpression>();
    if (!path) return false;
    if (path->path->name.originalName == nullptr) return true;

    const IR::IDeclaration *decl = nullptr;
    if (refMap) {
        decl = refMap->getDeclaration(path->path, false);
    } else {
        // The temporary may not have been added to the program yet.
        // Look it up without reporting an error if it is missing.
        auto declarations = resolve(path->path->name, ResolutionType::Any);
        if (declarations.size() == 1) decl = declarations.front();
    }
    return decl && decl->getName().originalName == nullptr;
}

const IR::Node *SubstituteParameters::postorder(IR::This *t) {
    auto result = new IR::This(t->srcInfo);
    LOG1("Cloned " << dbp(t) << " into " << dbp(result));
    return result;
}

const IR::Node *SubstituteParameters::postorder(IR::PathExpression *expr) {
    auto decl =
        (refMap ? refMap->getDeclaration(expr->path, true) : getDeclaration(expr->path, true));
    auto param = decl->to<IR::Parameter>();
    if (param != nullptr && subst->contains(param)) {
        const auto *value = subst->lookup(param)->expression;
        LOG1("Replaced " << dbp(expr) << " with " << dbp(value));
        // Keep the argument's source location unless it is a compiler temporary.
        // For temporaries, use the location of the parameter reference.
        auto *cloned = value->clone();
        if (useParameterSourceInfo(value)) cloned->srcInfo = expr->srcInfo;
        return cloned;
    }

    // Path expressions always need to be cloned.
    IR::ID newid = expr->path->name;
    auto path = new IR::Path(newid, expr->path->absolute);
    auto result = new IR::PathExpression(expr->srcInfo, path);
    LOG1("Cloned " << dbp(expr) << " into " << dbp(result));
    return result;
}

const IR::Node *SubstituteParameters::postorder(IR::Type_Name *type) {
    auto decl =
        (refMap ? refMap->getDeclaration(type->path, true) : getDeclaration(type->path, true));
    auto var = decl->to<IR::Type_Var>();
    if (var != nullptr && bindings->containsKey(var)) {
        auto repl = bindings->lookup(var);
        LOG1("Replaced " << dbp(type) << " with " << dbp(repl));
        return repl;
    }

    IR::ID newid = type->path->name;
    auto path = new IR::Path(newid, type->path->absolute);
    auto result = new IR::Type_Name(type->srcInfo, path);
    LOG1("Cloned " << dbp(type) << " into " << dbp(result));
    return result;
}

}  // namespace P4
