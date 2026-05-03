// Copyright 2018 VMware, Inc.
// SPDX-FileCopyrightText: 2018 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "orderArguments.h"

#include "frontends/p4/methodInstance.h"

namespace P4 {

static IR::Vector<IR::Argument> *reorder(const ParameterSubstitution &substitution) {
    auto reordered = new IR::Vector<IR::Argument>();

    bool foundOptional = false;
    for (auto p : *substitution.getParametersInOrder()) {
        auto arg = substitution.lookup(p);
        if (arg == nullptr) {
            // This argument may be missing either because it is
            // optional or because it is an argument for an action
            // which has some control-plane arguments.
            foundOptional = true;
        } else {
            BUG_CHECK(!foundOptional, "Not all optional parameters are at the end");
            reordered->push_back(arg);
        }
    }
    return reordered;
}

const IR::Node *DoOrderArguments::postorder(IR::MethodCallExpression *expression) {
    auto mi = MethodInstance::resolve(expression, this, typeMap);
    expression->arguments = reorder(mi->substitution);
    return expression;
}

const IR::Node *DoOrderArguments::postorder(IR::ConstructorCallExpression *expression) {
    ConstructorCall *ccd = ConstructorCall::resolve(expression, this, typeMap);
    expression->arguments = reorder(ccd->substitution);
    return expression;
}

const IR::Node *DoOrderArguments::postorder(IR::Declaration_Instance *instance) {
    auto inst = Instantiation::resolve(instance, this, typeMap);
    instance->arguments = reorder(inst->substitution);
    return instance;
}

}  // namespace P4
