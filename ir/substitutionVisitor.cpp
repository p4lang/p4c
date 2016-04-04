#include "substitutionVisitor.h"

namespace IR {

bool TypeOccursVisitor::preorder(const IR::Type_VarBase* typeVariable) {
    if (*typeVariable == *toFind) {
        occurs = true;
    }
    return occurs;
}

const IR::Node* TypeVariableSubstitutionVisitor::preorder(IR::TypeParameters *tps) {
    // remove all variables that were substituted
    for (auto it = tps->parameters.begin(); it != tps->parameters.end();) {
        if (bindings->containsKey(it->second)) {
            LOG1("Removing from generic parameters " << it->second);
            it = tps->parameters.erase(it);
        } else {
            ++it;
        }
    }
    return tps;
}

const IR::Node* TypeVariableSubstitutionVisitor::preorder(IR::Type_VarBase* typeVariable) {
    LOG1("Visiting " << getOriginal());
    const IR::Type* type = bindings->lookup(getOriginal<IR::Type_Var>());
    if (type == nullptr)
        return typeVariable;
    LOG1("Replacing " << getOriginal() << " with " << type);
    return type;
}

const IR::Node* TypeNameSubstitutionVisitor::preorder(IR::Type_Name* typeName) {
    auto type = bindings->lookup(typeName);
    if (type == nullptr)
        return typeName;
    return type;
}

}  // namespace IR
