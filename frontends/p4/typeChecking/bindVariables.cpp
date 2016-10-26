#include "bindVariables.h"

namespace P4 {

const IR::Type* BindTypeVariables::getVarValue(const IR::Type_Var* var) const {
    // Lookup a type variable
    auto type = typeMap->getSubstitution(var);
    CHECK_NULL(type);
    return getP4Type(type);
}

const IR::Type* BindTypeVariables::getP4Type(const IR::Type* type) const {
    auto rtype = type->getP4Type();
    CHECK_NULL(rtype);
    return rtype;
}

const IR::Node* BindTypeVariables::postorder(IR::Expression* expression) {
    // This is needed to handle newly created expressions because
    // their children have changed.
    if (auto type = typeMap->getType(getOriginal(), !findContext<IR::Annotation>()))
        typeMap->setType(expression, type);
    return expression;
}

const IR::Node* BindTypeVariables::postorder(IR::Declaration_Instance* decl) {
    if (decl->type->is<IR::Type_Specialized>())
        return decl;
    auto type = typeMap->getType(getOriginal(), true);
    BUG_CHECK(type->is<IR::IMayBeGenericType>(), "%1%: unexpected type %2% for declaration",
              decl, type);
    auto mt = type->to<IR::IMayBeGenericType>();
    if (mt->getTypeParameters()->empty())
        return decl;
    auto typeArgs = new IR::Vector<IR::Type>();
    for (auto p : *mt->getTypeParameters()->parameters) {
        auto type = getVarValue(p);
        typeArgs->push_back(type);
    }
    decl->type = new IR::Type_Specialized(
        decl->type->srcInfo, decl->type->to<IR::Type_Name>(), typeArgs);
    return decl;
}

const IR::Node* BindTypeVariables::postorder(IR::MethodCallExpression* expression) {
    if (!expression->typeArguments->empty())
        return expression;
    auto type = typeMap->getType(expression->method, true);
    BUG_CHECK(type->is<IR::IMayBeGenericType>(), "%1%: unexpected type %2% for method",
              expression->method, type);
    auto mt = type->to<IR::IMayBeGenericType>();
    if (mt->getTypeParameters()->empty())
        return expression;
    auto typeArgs = new IR::Vector<IR::Type>();
    for (auto p : *mt->getTypeParameters()->parameters) {
        auto type = getVarValue(p);
        typeArgs->push_back(type);
    }
    expression->typeArguments = typeArgs;
    return expression;
}

const IR::Node* BindTypeVariables::postorder(IR::ConstructorCallExpression* expression) {
    if (expression->constructedType->is<IR::Type_Specialized>())
        return expression;
    auto type = typeMap->getType(getOriginal(), true);
    BUG_CHECK(type->is<IR::IMayBeGenericType>(), "%1%: unexpected type %2% for expression",
              expression, type);
    auto mt = type->to<IR::IMayBeGenericType>();
    if (mt->getTypeParameters()->empty())
        return expression;
    auto typeArgs = new IR::Vector<IR::Type>();
    for (auto p : *mt->getTypeParameters()->parameters) {
        auto type = getVarValue(p);
        typeArgs->push_back(type);
    }
    expression->constructedType = new IR::Type_Specialized(
        expression->constructedType->srcInfo,
        expression->constructedType->to<IR::Type_Name>(), typeArgs);
    return expression;
}

const IR::Node* BindTypeVariables::insertTypes(const IR::Node* node) {
    CHECK_NULL(node);
    CHECK_NULL(newTypes);
    if (newTypes->empty())
        return node;
    newTypes->push_back(node);
    auto result = newTypes;
    newTypes = new IR::IndexedVector<IR::Node>();
    return result;
}

}  // namespace P4
