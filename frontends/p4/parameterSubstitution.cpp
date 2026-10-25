// Copyright 2018 VMware, Inc.
// SPDX-FileCopyrightText: 2018 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "parameterSubstitution.h"

#include "lib/log.h"

namespace P4 {

void ParameterSubstitution::add(const IR::Parameter *parameter, const IR::Argument *value) {
    CHECK_NULL(value);
    LOG2("Mapping " << dbp(parameter) << " to " << dbp(value));
    cstring name = parameter->name.name;
    auto par = get(parametersByName, name);
    BUG_CHECK(par == nullptr, "Two parameters with the same name %1% in a substitution", name);
    parameterValues.emplace(name, value);
    parametersByName.emplace(name, parameter);
    parameters.push_back(parameter);
}

void ParameterSubstitution::populate(const IR::ParameterList *params,
                                     const IR::Vector<IR::Argument> *args) {
    BUG_CHECK(paramList == nullptr, "Already populated");
    paramList = params;
    BUG_CHECK(params->size() >= args->size(),
              "Incompatible number of arguments for parameter list: %1% and %2%", params, args);

    auto pe = params->getEnumerator();
    for (auto a : *args) {
        const IR::Parameter *p;
        if (a->name) {
            p = params->getParameter(a->name);
            if (p == nullptr) {
                ::P4::error(ErrorType::ERR_NOT_FOUND, "No parameter named %1%", a->name);
                continue;
            }
        } else {
            bool success = pe->moveNext();
            BUG_CHECK(success, "Enumerator finished too soon");
            p = pe->getCurrent();
        }
        add(p, a);
    }
}

}  // namespace P4
