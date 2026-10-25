/*
 * Copyright 2018 VMware, Inc.
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_CHECKNAMEDARGS_H_
#define FRONTENDS_P4_CHECKNAMEDARGS_H_

#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

/// A method call must have either all or none of the arguments named.
/// We also check that no argument appears twice.
/// We also check that no optional parameter has a default value.
/// We also check that controls, parsers, and functions cannot have optional parameters
class CheckNamedArgs : public Inspector {
    bool checkOptionalParameters(const IR::ParameterList *params);

 public:
    CheckNamedArgs() { setName("CheckNamedArgs"); }

    bool checkArguments(const IR::Vector<IR::Argument> *arguments);
    bool preorder(const IR::MethodCallExpression *call) override {
        return checkArguments(call->arguments);
    }
    bool preorder(const IR::Declaration_Instance *call) override {
        return checkArguments(call->arguments);
    }
    bool preorder(const IR::Parameter *parameter) override;
    bool preorder(const IR::P4Control *control) override {
        return checkOptionalParameters(control->getConstructorParameters());
    }
    bool preorder(const IR::P4Parser *parser) override {
        return checkOptionalParameters(parser->getConstructorParameters());
    }
    bool preorder(const IR::Function *function) override {
        return checkOptionalParameters(function->getParameters());
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_CHECKNAMEDARGS_H_ */
