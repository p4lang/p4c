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

#ifndef TYPECHECKING_TYPESUBSTITUTION_H_
#define TYPECHECKING_TYPESUBSTITUTION_H_

#include <map>
#include <sstream>

#include "ir/ir.h"
#include "lib/exceptions.h"

namespace P4 {

/* Maps objects of type T to types. */
template <class T>
class TypeSubstitution : public IHasDbPrint {
 protected:
    ordered_map<T, const IR::Type *> binding;

 public:
    TypeSubstitution() = default;
    TypeSubstitution(const TypeSubstitution &other) : binding(other.binding) {}

    /** True if this is the empty substitution, which does not replace anything. */
    bool isIdentity() const { return binding.size() == 0; }
    const IR::Type *lookup(T t) const { return ::get(binding, t); }
    const IR::Type *get(T t) const { return ::get(binding, t); }

    bool containsKey(T key) const { return binding.find(key) != binding.end(); }

    /* This can fail if id is already bound.
     * @return true on success. */
    virtual bool setBinding(T id, const IR::Type *type) {
        CHECK_NULL(id);
        CHECK_NULL(type);
        auto it = binding.find(id);
        if (it != binding.end()) {
            if (it->second != type) return false;
            return true;
        }
        binding.emplace(id, type);
        return true;
    }

    void dbprint(std::ostream &out) const {
        if (isIdentity()) out << "Empty substitution";
        bool first = true;
        for (auto it : binding) {
            if (!first) out << std::endl;
            out << dbp(it.first) << " " << it.first << " -> " << dbp(it.second) << " " << it.second;
            first = false;
        }
    }

    void clear() { binding.clear(); }
};

class TypeVariableSubstitution final : public TypeSubstitution<const IR::ITypeVar *> {
 public:
    TypeVariableSubstitution() = default;
    TypeVariableSubstitution(const TypeVariableSubstitution &other) = default;
    bool setBindings(const IR::Node *errorLocation, const IR::TypeParameters *params,
                     const IR::Vector<IR::Type> *args);
    /// Returns an empty string on error, or an error message format otherwise.
    /// The error message should be used with 'var' and 'substitution' as arguments when
    /// reporting an error (i.e., it may contain %1% and %2% inside).
    cstring compose(const IR::ITypeVar *var, const IR::Type *substitution);
    // In this variant of compose all variables in 'other' that are
    // assigned to are disjoint from all variables already in 'this'.
    void simpleCompose(const TypeVariableSubstitution *other);
    void debugValidate();
    bool setBinding(const IR::ITypeVar *id, const IR::Type *type) override {
        auto result = TypeSubstitution::setBinding(id, type);
        debugValidate();
        return result;
    }
};

}  // namespace P4

#endif /* TYPECHECKING_TYPESUBSTITUTION_H_ */
