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

#ifndef FRONTENDS_P4_PARAMETERSUBSTITUTION_H_
#define FRONTENDS_P4_PARAMETERSUBSTITUTION_H_

#include <iostream>

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/enumerator.h"

namespace P4 {

/* Maps Parameters to Expressions via their name.  Note that
   parameter identity is not important, but the parameter name is. */
class ParameterSubstitution : public IHasDbPrint {
 protected:
    // Parameter names are unique for a procedure, so each name
    // should show up only once.
    std::map<cstring, const IR::Expression*> parameterValues;
    // Map from parameter name to parameter.
    std::map<cstring, const IR::Parameter*>  parametersByName;
    std::vector<const IR::Parameter*>        parameters;

    bool containsName(cstring name) const
    { return parameterValues.find(name) != parameterValues.end(); }

 public:
    ParameterSubstitution() = default;
    ParameterSubstitution(const ParameterSubstitution& other) = default;

    void add(const IR::Parameter* parameter, const IR::Expression* value) {
        LOG1("Mapping " << dbp(parameter) << " to " << dbp(value));
        cstring name = parameter->name.name;
        auto par = get(parametersByName, name);
        BUG_CHECK(par == nullptr,
                  "Two parameters with the same name %1% in a substitution", name);
        parameterValues.emplace(name, value);
        parametersByName.emplace(name, parameter);
        parameters.push_back(parameter);
    }

    const IR::Expression* lookupByName(cstring name) const
    { return get(parameterValues, name); }

    const IR::Expression* lookup(const IR::Parameter* param) const
    { return lookupByName(param->name.name); }

    bool contains(const IR::Parameter* param) const {
        if (!containsName(param->name.name))
            return false;
        return true;
    }

    bool empty() const
    { return parameterValues.empty(); }

    void populate(const IR::ParameterList* params,
                  const IR::Vector<IR::Expression>* args) {
        // Allow for binding only some parameters: used for actions
        BUG_CHECK(params->size() >= args->size(),
                  "Incompatible number of arguments for parameter list: %1% and %2%",
                  params, args);
        auto pe = params->getEnumerator();
        for (auto a : *args) {
            bool success = pe->moveNext();
            BUG_CHECK(success, "Enumerator finished too soon");
            add(pe->getCurrent(), a);
        }
    }

    Util::Enumerator<const IR::Parameter*>* getParameters() const
    { return Util::Enumerator<const IR::Parameter*>::createEnumerator(parameters); }

    void dbprint(std::ostream& out) const {
        for (auto s : parametersByName)
            out << dbp(s.second) << "=>" << dbp(lookupByName(s.first)) << std::endl;
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_PARAMETERSUBSTITUTION_H_ */
