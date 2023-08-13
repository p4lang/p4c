#include "bindVariables.h"

#include <ostream>

#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/log.h"

namespace P4 {

namespace {
// Reports errors for all sub-expressions which have type InfInt.
class ErrorOnInfInt : public Inspector {
 public:
    const TypeMap *typeMap;

    explicit ErrorOnInfInt(const TypeMap *typeMap) : typeMap(typeMap) {}
    void postorder(const IR::Expression *expression) override {
        auto t = typeMap->getType(expression, true);
        if (t->is<IR::Type_InfInt>())
            ::error(ErrorType::ERR_TYPE_ERROR, "%1%: could not infer a width", expression);
    }
};

class HasInfInt : public Inspector {
 public:
    bool found = false;
    bool preorder(const IR::Type_Stack *type) override {
        visit(type->elementType);
        // We skip the array size, that's a constant
        return false;
    }
    bool preorder(const IR::Annotation *) override {
        // Ignore constants that may show up in annotations
        return false;
    }
    void postorder(const IR::Type_InfInt *) override { found = true; }
    static bool find(const IR::Node *node, const Visitor *calledBy) {
        HasInfInt hii;
        hii.setCalledBy(calledBy);
        node->apply(hii);
        return hii.found;
    }
};

}  // namespace

/// Validate the type of a type variable.  The type must not contain
/// Type_InfInt inside.
/// Return nullptr if the type is not suitable to assign to a type variable.
static const IR::Type *validateType(const IR::Type *type, const TypeMap *typeMap,
                                    const IR::Node *errorPosition, const Visitor *calledBy) {
    auto repl = type ? type->getP4Type() : nullptr;
    if (type == nullptr || repl == nullptr || HasInfInt::find(type, calledBy)) {
        auto eoi = new ErrorOnInfInt(typeMap);
        eoi->setCalledBy(calledBy);
        errorPosition->apply(*eoi);
        return nullptr;
    }
    return repl;
}

/// Lookup a type variable
const IR::Type *DoBindTypeVariables::getVarValue(const IR::Type_Var *var,
                                                 const IR::Node *errorPosition) const {
    auto type = typeMap->getSubstitution(var);
    if (type == nullptr) {
        ::error(ErrorType::ERR_TYPE_ERROR, "%1%: could not infer a type for variable %2%",
                errorPosition, var);
        return nullptr;
    }
    auto result = validateType(type, typeMap, errorPosition, this);
    LOG2("Replacing " << var << " with " << result);
    return result;
}

const IR::Node *DoBindTypeVariables::postorder(IR::Expression *expression) {
    // This is needed to handle newly created expressions because
    // their children have changed.
    auto type = typeMap->getType(getOriginal(), true);
    typeMap->setType(expression, type);
    return expression;
}

const IR::Node *DoBindTypeVariables::postorder(IR::Declaration_Instance *decl) {
    if (decl->type->is<IR::Type_Specialized>()) return decl;
    auto type = typeMap->getType(getOriginal(), true);
    if (auto tsc = type->to<IR::Type_SpecializedCanonical>()) type = tsc->substituted;
    BUG_CHECK(type->is<IR::IMayBeGenericType>(), "%1%: unexpected type %2% for declaration", decl,
              type);
    auto mt = type->to<IR::IMayBeGenericType>();
    if (mt->getTypeParameters()->empty()) return decl;
    auto typeArgs = new IR::Vector<IR::Type>();
    for (auto p : mt->getTypeParameters()->parameters) {
        auto type = getVarValue(p, getOriginal());
        if (type == nullptr) return decl;
        typeArgs->push_back(type);
    }
    decl->type =
        new IR::Type_Specialized(decl->type->srcInfo, decl->type->to<IR::Type_Name>(), typeArgs);
    return decl;
}

const IR::Node *DoBindTypeVariables::postorder(IR::MethodCallExpression *expression) {
    auto type = typeMap->getType(getOriginal(), true);
    typeMap->setType(expression, type);
    if (typeMap->isCompileTimeConstant(getOriginal<IR::Expression>()))
        typeMap->setCompileTimeConstant(expression);
    if (!expression->typeArguments->empty()) return expression;
    type = typeMap->getType(expression->method, true);
    BUG_CHECK(type->is<IR::IMayBeGenericType>(), "%1%: unexpected type %2% for method",
              expression->method, type);
    auto mt = type->to<IR::IMayBeGenericType>();
    if (mt->getTypeParameters()->empty()) return expression;
    auto typeArgs = new IR::Vector<IR::Type>();
    for (auto p : mt->getTypeParameters()->parameters) {
        auto type = getVarValue(p, getOriginal());
        if (type == nullptr) return expression;
        typeArgs->push_back(type);
    }
    expression->typeArguments = typeArgs;
    return expression;
}

const IR::Node *DoBindTypeVariables::postorder(IR::ConstructorCallExpression *expression) {
    if (expression->constructedType->is<IR::Type_Specialized>()) return expression;
    auto type = typeMap->getType(getOriginal(), true);
    BUG_CHECK(type->is<IR::IMayBeGenericType>(), "%1%: unexpected type %2% for expression",
              expression, type);
    auto mt = type->to<IR::IMayBeGenericType>();
    if (mt->getTypeParameters()->empty()) return expression;
    auto typeArgs = new IR::Vector<IR::Type>();
    for (auto p : mt->getTypeParameters()->parameters) {
        auto type = getVarValue(p, getOriginal());
        if (type == nullptr) return expression;
        typeArgs->push_back(type);
    }
    expression->constructedType =
        new IR::Type_Specialized(expression->constructedType->srcInfo,
                                 expression->constructedType->to<IR::Type_Name>(), typeArgs);
    return expression;
}

const IR::Node *DoBindTypeVariables::insertTypes(const IR::Node *node) {
    CHECK_NULL(node);
    CHECK_NULL(newTypes);
    if (newTypes->empty()) return node;
    newTypes->push_back(node);
    auto result = newTypes;
    newTypes = new IR::IndexedVector<IR::Node>();
    return result;
}

}  // namespace P4
