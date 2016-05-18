/*
Copyright 2013-present Barefoot Networks, Inc. 

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

#ifndef _IR_PARAMETERSUBSTITUTION_H_
#define _IR_PARAMETERSUBSTITUTION_H_

#include <iostream>

#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/enumerator.h"

namespace IR {

/* Maps Parameters to Expressions */
class ParameterSubstitution {
    std::map<cstring, const IR::Expression*>  parameterValues;
    // Map from parameter name to parameter.
    std::map<cstring, const IR::Parameter*>   parameters;

 public:
    void add(const IR::Parameter* parameter, const IR::Expression* value) {
        LOG1("Mapping " << parameter << " to " << value);
        cstring name = parameter->name.name;
        auto par = get(parameters, name);
        BUG_CHECK(par == nullptr,
                  "Two parameters with the same name %1% in a substitution", name);
        parameterValues.emplace(name, value);
        parameters.emplace(name, parameter);
    }

    const IR::Expression* lookupName(cstring name) const {
        return get(parameterValues, name);
    }

    bool containsName(cstring name) const
    { return parameterValues.find(name) != parameterValues.end(); }

    ParameterSubstitution* append(const ParameterSubstitution* other) const {
        ParameterSubstitution* result = new ParameterSubstitution(*this);
        for (auto param : other->parameters)
            result->add(param.second, other->lookupName(param.first));
        return result;
    }

    const IR::Expression* lookup(const IR::Parameter* param) const
    { return lookupName(param->name.name); }

    bool contains(const IR::Parameter* param) const {
        if (!containsName(param->name.name))
            return false;
        auto it = parameters.find(param->name.name);
        if (param != it->second)
            return false;
        return true;
    }

    const IR::Parameter* getParameter(cstring name) const
    { return get(parameters, name); }

    bool empty() const
    { return parameterValues.empty(); }

    void populate(const IR::ParameterList* params,
                  const IR::Vector<IR::Expression>* args) {
        BUG_CHECK(params->size() == args->size(),
                  "Incompatible number of arguments for parameter list");

        auto it = params->parameters->begin();
        for (auto a : *args) {
            add(*it, a);
            ++it;
        }
    }

    void dbprint(std::ostream& out) const {
        for (auto s : parameters)
            out << s.second << "=>" << lookupName(s.first) << std::endl;
    }
};

}  // namespace IR

#endif /* _IR_PARAMETERSUBSTITUTION_H_ */
