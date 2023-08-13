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

#include "typeSubstitution.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "ir/indexed_vector.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "typeSubstitutionVisitor.h"

namespace P4 {
cstring TypeVariableSubstitution::compose(const IR::ITypeVar *var, const IR::Type *substitution) {
    LOG3("Adding " << var << "->" << dbp(substitution) << "=" << substitution
                   << " to substitution");
    if (substitution->is<IR::Type_Dontcare>()) return "";

    // Type variables that represent Type_InfInt can only be unified to bit<> types
    // or to other Type_InfInt types.
    if (var->is<IR::Type_InfInt>()) {
        while (substitution->is<IR::Type_Newtype>())
            substitution = substitution->to<IR::Type_Newtype>()->type;
        if (auto se = substitution->to<IR::Type_SerEnum>()) substitution = se->type;
        if (!substitution->is<IR::Type_InfInt>() && !substitution->is<IR::Type_Bits>() &&
            !substitution->is<IR::Type_Any>()) {
            return "'%1%' type can only be unified with 'int', 'bit<>', or 'signed<>' types, "
                   "not with '%2%'";
        }
    }

    // First check whether the substitution is legal.
    // It is not if var occurs in substitution
    TypeOccursVisitor occurs(var);
    substitution->apply(occurs);
    if (occurs.occurs) return "'%1%' cannot be replaced with '%2%' which already contains it";

    // Check to see whether we already have a binding for this variable
    if (containsKey(var)) {
        const IR::Type *bound = lookup(var);
        BUG("Two bindings for the same variable %1%: %2% and %3%", var->toString(),
            substitution->toString(), bound->toString());
    }

    // Replace var with substitution everywhere
    TypeVariableSubstitution *tvs = new TypeVariableSubstitution();
    bool success = tvs->setBinding(var, substitution);
    if (!success) BUG("Cannot set binding");

    TypeVariableSubstitutionVisitor visitor(tvs);
    bool cycle = false;  // set if we detect X -> V and V -> X substitutions.
    for (auto &bound : binding) {
        const IR::Type *type = bound.second;
        const IR::Node *newType = type->apply(visitor);
        if (newType == nullptr) return "Could not replace '%1%' with '%2%'";
        if (newType == type) continue;

        if (bound.first->asType() == newType) {
            cycle = true;
        } else {
            LOG3("Refining substitution for " << bound.first->getNode() << " to " << newType);
            bound.second = newType->to<IR::Type>();
        }
    }

    if (cycle) {
        LOG3("Ignoring substitution already implied " << var << "->" << substitution);
    } else {
        LOG3("Actual binding " << var << " " << dbp(var) << "->" << dbp(substitution) << "="
                               << substitution);
        success = setBinding(var, substitution);
        if (!success) BUG("Failed to insert binding");
    }
    return "";
}

void TypeVariableSubstitution::simpleCompose(const TypeVariableSubstitution *other) {
    CHECK_NULL(other);
    for (auto v : other->binding) {
        const IR::Type *subst = v.second;
        auto it = binding.find(v.first);
        if (it != binding.end())
            BUG("Changing binding for %1% from %2% to %3%", v.first, it->second, subst);
        LOG3("Setting substitution for " << v.first->getNode() << " to " << subst);
        binding[v.first] = subst;
    }
    debugValidate();
}

bool TypeVariableSubstitution::setBindings(const IR::Node *errorLocation,
                                           const IR::TypeParameters *params,
                                           const IR::Vector<IR::Type> *args) {
    if (params == nullptr || args == nullptr) BUG("Nullptr argument to setBindings");

    if (params->parameters.size() != args->size()) {
        ::error(ErrorType::ERR_TYPE_ERROR, "%1% has %2% type parameters, invoked with %3%",
                errorLocation, params->parameters.size(), args->size());
        return false;
    }

    auto it = args->begin();
    for (auto tp : params->parameters) {
        auto t = *it;
        ++it;

        bool success = setBinding(tp, t);
        if (!success) {
            ::error(ErrorType::ERR_TYPE_ERROR, "%1%: Cannot bind %2% to %3%", errorLocation, tp, t);
            return false;
        }
    }

    return true;
}

void TypeVariableSubstitution::debugValidate() {
#if 0
    // Turn on only for debugging
    for (auto v : binding) {
        const IR::Type *subst = v.second;
        TypeOccursVisitor occurs(v.first);
        subst->apply(occurs);
        BUG_CHECK(!occurs.occurs,
                  "'%1%' occurs in '%2%' which replaces it", v.first, v.second);
    }
#endif
}

// to call from gdb
void dump(P4::TypeVariableSubstitution &tvs) { std::cout << tvs << std::endl; }
void dump(P4::TypeVariableSubstitution *tvs) { std::cout << *tvs << std::endl; }

}  // namespace P4
