// Copyright 2018 VMware, Inc.
// SPDX-FileCopyrightText: 2018 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "checkNamedArgs.h"

#include "lib/hash.h"

namespace P4 {

bool CheckNamedArgs::checkArguments(const IR::Vector<IR::Argument> *arguments) {
    bool first = true;
    bool hasName = false;
    absl::flat_hash_map<cstring, const IR::Argument *, Util::Hash> found;

    for (auto arg : *arguments) {
        cstring argName = arg->name.name;
        bool argHasName = !argName.isNullOrEmpty();
        if (first) {
            hasName = argHasName;
            first = false;
        } else {
            if (argHasName != hasName)
                ::P4::error(ErrorType::ERR_INVALID,
                            "%1%: either all or none of the arguments of a call must be named",
                            arg);
            if (argHasName) {
                auto it = found.find(argName);
                if (it != found.end())
                    ::P4::error(ErrorType::ERR_DUPLICATE, "%1% and %2%: same argument name",
                                it->second, arg);
            }
        }
        if (argHasName) found.emplace(argName, arg);
    }
    return true;
}

bool CheckNamedArgs::checkOptionalParameters(const IR::ParameterList *parameters) {
    for (auto parameter : parameters->parameters) {
        if (parameter->isOptional())
            ::P4::error(ErrorType::ERR_INVALID, "%1%: optional parameter not allowed here",
                        parameter);
    }
    return true;
}

bool CheckNamedArgs::preorder(const IR::Parameter *parameter) {
    if (parameter->defaultValue != nullptr) {
        if (parameter->isOptional())
            ::P4::error(ErrorType::ERR_INVALID,
                        "%1%: optional parameters cannot have default values", parameter);
        if (parameter->hasOut())
            ::P4::error(ErrorType::ERR_INVALID, "%1%: out parameters cannot have default values",
                        parameter);
    }
    return true;
}

}  // namespace P4
