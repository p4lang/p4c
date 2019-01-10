/*
Copyright 2018 VMware, Inc.

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

#include "checkNamedArgs.h"

namespace P4 {

bool CheckNamedArgs::checkArguments(const IR::Vector<IR::Argument>* arguments) {
    bool first = true;
    bool hasName = false;
    std::map<cstring, const IR::Argument*> found;

    for (auto arg : *arguments) {
        cstring argName = arg->name.name;
        bool argHasName = !argName.isNullOrEmpty();
        if (first) {
            hasName = argHasName;
            first = false;
        } else {
            if (argHasName != hasName)
                ::error(ErrorType::ERR_INVALID,
                        "either all or none of the arguments of a call must be named", arg);
            if (argHasName) {
                auto it = found.find(argName);
                if (it != found.end())
                    ::error("%1% and %2%: same argument name", it->second, arg);
            }
        }
        if (argHasName)
            found.emplace(argName, arg);
    }
    return true;
}

bool CheckNamedArgs::preorder(const IR::Parameter* parameter) {
    if (parameter->defaultValue != nullptr) {
        if (parameter->isOptional())
            ::error("%1%: optional parameters cannot have default values", parameter);
        if (parameter->hasOut())
            ::error("%1%: out parameters cannot have default values", parameter);
    }
    return true;
}

}  // namespace P4
