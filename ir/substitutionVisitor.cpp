#include "substitutionVisitor.h"

namespace IR {

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
    auto result = new IR::IndexedVector<IR::Type_Var>();
    for (auto param : *tps->parameters) {
        if (bindings->containsKey(param))
            LOG1("Removing from generic parameters " << param);
        else
            result->push_back(param);
    }
    return new IR::TypeParameters(tps->srcInfo, result);
}

const IR::Node* TypeVariableSubstitutionVisitor::preorder(IR::Type_Var* typeVariable) {
    LOG1("Visiting " << getOriginal());
    const IR::Type* type = bindings->lookup(getOriginal<IR::Type_Var>());
    if (type == nullptr)
        return typeVariable;
    LOG1("Replacing " << getOriginal() << " with " << type);
    return type;
}

const IR::Node* TypeVariableSubstitutionVisitor::preorder(IR::Type_InfInt* typeVariable) {
    LOG1("Visiting " << getOriginal());
    const IR::Type* type = bindings->lookup(getOriginal<IR::Type_InfInt>());
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
