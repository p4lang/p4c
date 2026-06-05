// Copyright 2017 VMware, Inc.
// SPDX-FileCopyrightText: 2017 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "dontcareArgs.h"

#include "frontends/p4/methodInstance.h"

namespace P4 {

Visitor::profile_t DontcareArgs::init_apply(const IR::Node *node) {
    auto rv = Transform::init_apply(node);
    node->apply(nameGen);

    return rv;
}

const IR::Node *DontcareArgs::postorder(IR::MethodCallExpression *expression) {
    bool changes = false;
    auto vec = new IR::Vector<IR::Argument>();

    auto mi = MethodInstance::resolve(expression, this, typeMap);
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        auto a = mi->substitution.lookup(p);
        if (a->expression->is<IR::DefaultExpression>()) {
            cstring name = nameGen.newName("arg");
            auto ptype = p->type;
            if (ptype->is<IR::Type_Dontcare>()) {
                ::P4::error(ErrorType::ERR_TYPE_ERROR, "Could not infer type for %1%", a);
                return expression;
            }
            auto decl = new IR::Declaration_Variable(IR::ID(name), ptype, nullptr);
            toAdd.push_back(decl);
            changes = true;
            vec->push_back(
                new IR::Argument(a->srcInfo, a->name, new IR::PathExpression(IR::ID(name))));
        } else {
            vec->push_back(a);
        }
    }
    if (changes) expression->arguments = vec;
    return expression;
}

}  // namespace P4
