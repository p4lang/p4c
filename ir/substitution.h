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

#ifndef _IR_SUBSTITUTION_H_
#define _IR_SUBSTITUTION_H_

#include <map>
#include <sstream>

#include "ir/ir.h"
#include "lib/exceptions.h"

namespace IR {

/* Maps objects of type T to types. */
template <class T>
class TypeSubstitution {
 protected:
    std::map<T, const IR::Type*> binding;
    /* True if this is the empty substitution, which does not replace anything. */
 public:
    bool isIdentity() const { return binding.size() == 0; }
    const IR::Type* lookup(T t) const
    { return ::get(binding, t); }
    const IR::Type* get(T t) const
    { return ::get(binding, t); }

    bool containsKey(T key) const { return binding.find(key) != binding.end(); }

    /* Bind a variable to a value.  This can fail if the variable is already bound.
     * @param  id   Variable to bind
     * @param  type Type to bind to.
     * @return True on success. */
    bool setBinding(T id, const IR::Type* type) {
        if (id == nullptr || type == nullptr)
            BUG("Null argument for setBinding");
        auto it = binding.find(id);
        if (it != binding.end()) {
            if (it->second != type)
                return false;
            return true;
        }

        binding.emplace(id, type);
        return true;
    }

    void dbprint(std::ostream& out) const {
        if (isIdentity())
            out << "Empty substitution";
        bool first = true;
        for (auto it : binding) {
            if (!first)
                out << std::endl;
            out << it.first << " -> " << it.second;
            first = false;
        }
    }

    void clear() { binding.clear(); }
};

class TypeVariableSubstitution final : public TypeSubstitution<const IR::ITypeVar*> {
 private:
    const IR::ITypeVar* lookupKey(cstring name) const {
        for (auto it : binding)
            if (it.first->getVarName() == name)
                return it.first;
        return nullptr;
    }

 public:
    /* Lookup a type variable by name.
     * @param name  Name of variable to lookup.
     * @return      The type variable value associated with that name in this map,
     *              or nullptr if there is no such binding. */
    bool setBinding(const IR::ITypeVar* id, const IR::Type* type) {
        auto t = lookupKey(id->getVarName());
        if (t != nullptr)
            BUG("Two variables with the same name in a binding");

        return TypeSubstitution::setBinding(id, type);
    }

    /* Set bindings for multiple variables at the same time.
     * @param params   List of variables to set.
     * @param args     List of type arguments to set to variables.
     * @param root     P4 program element which is being processed. Used for error reporting.
     * @param reporter Used to report errors.
     * @return         True on success. */
    bool setBindings(const IR::Node* root,
                     const IR::TypeParameters* params,
                     const IR::Vector<IR::Type>* args);

    /* Apply this additional substitution to all substitutions already present.
     * Add this substitution to the list.
     * @param var           Variable to substitute everywhere.
     * @param substitution  Substitution for variable.
     * @param debug         If true print debugging messages.
     * @param reporter      Used to report errors.
     * @return              True on success.     */
    bool compose(const IR::ITypeVar* var, const IR::Type* substitution);
};

class TypeNameSubstitution final : public TypeSubstitution<const IR::Type_Name*> {};

}  // namespace IR

#endif /* _IR_SUBSTITUTION_H_ */

