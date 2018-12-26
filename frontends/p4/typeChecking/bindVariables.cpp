#include "bindVariables.h"

namespace P4 {

class FindUntypedInt : public Inspector {
    const IR::Node *pos;
    bool preorder(const IR::Node *) override { return pos == nullptr; }
    bool preorder(const IR::Type *) override { return false; }
    void postorder(const IR::Constant *c) override { if (c->type->is<IR::Type_InfInt>()) pos = c; }
    Visitor::profile_t init_apply(const IR::Node *root) override {
        pos = nullptr;
        return Inspector::init_apply(root); }

 public:
    explicit FindUntypedInt(const IR::Node *n) { n->apply(*this); }
    operator const IR::Node *() const { return pos; }
};

/// Lookup a type variable
const IR::Type* DoBindTypeVariables::getVarValue(
    const IR::Type_Var* var, const IR::Node* errorPosition) const {
    const IR::Type* rtype = nullptr;
    auto type = typeMap->getSubstitution(var);
    if (type != nullptr)
        rtype = type->getP4Type();
    else
        rtype = new IR::Type_Dontcare;
    if (rtype == nullptr) {
        cstring errorMessage;
        errorMessage = "%1%: cannot infer type for type parameter %2%";
        if (type != nullptr) {
            // FIXME -- what we really want here for the error message is the expression which
            // caused us to be unable to infer a usable type -- generally an untyped integer
            // constant in the code.  There's no easy way to extract that, however.  We need
            // to keep a map<Type_Var, set<Expression>> from type inferencing containing all
            // the expressions used to infer the type of the Type Var.  For now we use a best
            // guess trying to print something relevant.
            if (type->is<IR::Type_InfInt>()) {
                errorMessage = "%1%: cannot infer bitwidth for integer-valued type parameter %2%";
            } else if (const IR::Node *pos = FindUntypedInt(errorPosition)) {
                errorMessage = "%1%: cannot infer bitwidth for untyped integer constant used "
                               "in type parameter %2%";
                errorPosition = pos; } }
        ::error(errorMessage.c_str(), errorPosition, var); }
    return rtype;  // This may be nullptr
}

const IR::Node* DoBindTypeVariables::postorder(IR::Expression* expression) {
    // This is needed to handle newly created expressions because
    // their children have changed.
    auto type = typeMap->getType(getOriginal(), true);
    typeMap->setType(expression, type);
    return expression;
}

const IR::Node* DoBindTypeVariables::postorder(IR::Declaration_Instance* decl) {
    if (decl->type->is<IR::Type_Specialized>())
        return decl;
    auto type = typeMap->getType(getOriginal(), true);
    BUG_CHECK(type->is<IR::IMayBeGenericType>(), "%1%: unexpected type %2% for declaration",
              decl, type);
    auto mt = type->to<IR::IMayBeGenericType>();
    if (mt->getTypeParameters()->empty())
        return decl;
    auto typeArgs = new IR::Vector<IR::Type>();
    for (auto p : mt->getTypeParameters()->parameters) {
        auto type = getVarValue(p, decl);
        if (type == nullptr)
            return decl;
        typeArgs->push_back(type);
    }
    decl->type = new IR::Type_Specialized(
        decl->type->srcInfo, decl->type->to<IR::Type_Name>(), typeArgs);
    return decl;
}

const IR::Node* DoBindTypeVariables::postorder(IR::MethodCallExpression* expression) {
    if (!expression->typeArguments->empty())
        return expression;
    auto type = typeMap->getType(expression->method, true);
    BUG_CHECK(type->is<IR::IMayBeGenericType>(), "%1%: unexpected type %2% for method",
              expression->method, type);
    auto mt = type->to<IR::IMayBeGenericType>();
    if (mt->getTypeParameters()->empty())
        return expression;
    auto typeArgs = new IR::Vector<IR::Type>();
    for (auto p : mt->getTypeParameters()->parameters) {
        auto type = getVarValue(p, expression);
        if (type == nullptr)
            return expression;
        typeArgs->push_back(type);
    }
    expression->typeArguments = typeArgs;
    return expression;
}

const IR::Node* DoBindTypeVariables::postorder(IR::ConstructorCallExpression* expression) {
    if (expression->constructedType->is<IR::Type_Specialized>())
        return expression;
    auto type = typeMap->getType(getOriginal(), true);
    BUG_CHECK(type->is<IR::IMayBeGenericType>(), "%1%: unexpected type %2% for expression",
              expression, type);
    auto mt = type->to<IR::IMayBeGenericType>();
    if (mt->getTypeParameters()->empty())
        return expression;
    auto typeArgs = new IR::Vector<IR::Type>();
    for (auto p : mt->getTypeParameters()->parameters) {
        auto type = getVarValue(p, expression);
        if (type == nullptr)
            return expression;
        typeArgs->push_back(type);
    }
    expression->constructedType = new IR::Type_Specialized(
        expression->constructedType->srcInfo,
        expression->constructedType->to<IR::Type_Name>(), typeArgs);
    return expression;
}

const IR::Node* DoBindTypeVariables::insertTypes(const IR::Node* node) {
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
