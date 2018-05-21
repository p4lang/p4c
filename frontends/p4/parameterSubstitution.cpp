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

#include "parameterSubstitution.h"

namespace P4 {

void ParameterSubstitution::add(
    const IR::Parameter* parameter, const IR::Argument* value) {
    LOG2("Mapping " << dbp(parameter) << " to " << dbp(value));
    cstring name = parameter->name.name;
    auto par = get(parametersByName, name);
    BUG_CHECK(par == nullptr,
              "Two parameters with the same name %1% in a substitution", name);
    parameterValues.emplace(name, value);
    parametersByName.emplace(name, parameter);
    parameters.push_back(parameter);
}

void ParameterSubstitution::populate(const IR::ParameterList* params,
                                     const IR::Vector<IR::Argument>* args) {
    BUG_CHECK(paramList == nullptr, "Already populated");
    paramList = params;
    BUG_CHECK(params->size() >= args->size(),
              "Incompatible number of arguments for parameter list: %1% and %2%",
              params, args);

    auto pe = params->getEnumerator();
    for (auto a : *args) {
        const IR::Parameter* p;
        if (a->name) {
            p = params->getParameter(a->name);
            if (p == nullptr) {
                ::error("No parameter named %1%", a->name);
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
