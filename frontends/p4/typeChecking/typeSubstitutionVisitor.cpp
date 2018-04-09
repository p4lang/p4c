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

#include "typeSubstitutionVisitor.h"

namespace P4 {

bool TypeOccursVisitor::preorder(const IR::Type_Var* typeVariable) {
    if (*typeVariable == *(toFind->asType()))
        occurs = true;
    return occurs;
}

bool TypeOccursVisitor::preorder(const IR::Type_InfInt* typeVariable) {
    if (*typeVariable == *(toFind->asType()))
        occurs = true;
    return occurs;
}

const IR::Node* TypeVariableSubstitutionVisitor::preorder(IR::TypeParameters *tps) {
    // remove all variables that were substituted
    for (auto it = tps->parameters.begin(); it != tps->parameters.end();) {
        const IR::Type* type = bindings->lookup(*it);
        if (type != nullptr && !replace) {
            LOG3("Removing from generic parameters " << *it);
            it = tps->parameters.erase(it);
        } else {
            if (type != nullptr)
                BUG_CHECK(type->is<IR::Type_Var>(),
                          "cannot replace a type parameter %1% with %2%:", *it, type);
            ++it;
        }
    }
    return tps;
}

const IR::Node* TypeVariableSubstitutionVisitor::replacement(IR::ITypeVar* typeVariable) {
    LOG3("Visiting " << dbp(getOriginal()));
    const IR::ITypeVar* current = getOriginal<IR::ITypeVar>();
    const IR::Type* replacement = nullptr;
    while (true) {
        // This will end because there should be no circular chain of variables pointing
        // to each other.
        const IR::Type* type = bindings->lookup(current);
        if (type == nullptr)
            break;
        replacement = type;
        if (!type->is<IR::ITypeVar>())
            break;
        current = type->to<IR::ITypeVar>();
    }
    if (replacement == nullptr)
        return typeVariable->getNode();
    LOG2("Replacing " << getOriginal() << " with " << replacement);
    return replacement;
}

const IR::Node* TypeNameSubstitutionVisitor::preorder(IR::Type_Name* typeName) {
    auto type = bindings->lookup(typeName);
    if (type == nullptr)
        return typeName;
    return type;
}

}  // namespace P4
