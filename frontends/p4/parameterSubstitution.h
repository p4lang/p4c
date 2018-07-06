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

/* Maps Parameters to Arguments via their name.  Note that
   parameter identity is not important, but the parameter name is. */
class ParameterSubstitution : public IHasDbPrint {
 protected:
    // Parameter names are unique for a procedure, so each name
    // should show up only once.
    std::map<cstring, const IR::Argument*> parameterValues;
    /// Map from parameter name to parameter.
    std::map<cstring, const IR::Parameter*>  parametersByName;
    /// Parameters in the order they were added.
    std::vector<const IR::Parameter*>        parameters;
    /// If created using populate this is non-null.
    const IR::ParameterList*                 paramList = nullptr;

    bool containsName(cstring name) const
    { return parameterValues.find(name) != parameterValues.end(); }

 public:
    ParameterSubstitution() = default;
    ParameterSubstitution(const ParameterSubstitution& other) = default;

    void add(const IR::Parameter* parameter, const IR::Argument* value);
    const IR::Argument* lookupByName(cstring name) const
    { return get(parameterValues, name); }

    const IR::Argument* lookup(const IR::Parameter* param) const
    { return lookupByName(param->name.name); }

    bool contains(const IR::Parameter* param) const {
        if (!containsName(param->name.name))
            return false;
        return true;
    }

    const IR::Parameter* findParameter(const IR::Argument* argument) const {
        for (auto p : *getParametersInOrder())
            if (lookup(p) == argument)
                return p;
        return nullptr;
    }

    bool empty() const
    { return parameterValues.empty(); }

    /// Only the parameters which have corresponding arguments
    /// will be bound.  PARAMETERS ARE ADDED IN THE ORDER OF THE ARGUMENTS.
    // For actions only some parameters may be bound.
    void populate(const IR::ParameterList* params,
                  const IR::Vector<IR::Argument>* args);

    /// Returns parameters in the order they were added
    Util::Enumerator<const IR::Parameter*>* getParametersInArgumentOrder() const
    { return Util::Enumerator<const IR::Parameter*>::createEnumerator(parameters); }

    /// Returns parameters in the order of the parameter list.
    /// Only works if parameters were inserted using populate.
    Util::Enumerator<const IR::Parameter*>* getParametersInOrder() const {
        if (paramList == nullptr)
            return nullptr;
        return paramList->getEnumerator();
    }

    void dbprint(std::ostream& out) const {
        for (auto s : parametersByName)
            out << dbp(s.second) << "=>" << dbp(lookupByName(s.first)) << std::endl;
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_PARAMETERSUBSTITUTION_H_ */
