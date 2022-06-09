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

#ifndef _FRONTENDS_P4_METHODINSTANCE_H_
#define _FRONTENDS_P4_METHODINSTANCE_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/p4/methodInstance.h"

namespace P4 {

class InstanceBase {
 public:
    template<typename T> bool is() const { return to<T>() != nullptr; }
    template<typename T> const T* to() const { return dynamic_cast<const T*>(this); }

 protected:
    virtual ~InstanceBase() {}

 public:
    /// For each callee parameter the corresponding argument
    ParameterSubstitution          substitution;
    /// Substitution of the type parameters
    /// This may not be filled in if we resolved with 'incomplete'
    TypeVariableSubstitution       typeSubstitution;
};

/**
This class is very useful for extracting information out of
MethodCallExpressions.  Since there are no function pointers in P4,
methods can completely be resolved at compilation time.  The static
method 'resolve' will categorize each method call into one of several
kinds:
- apply method, could be of a table, control or parser
- extern function
- extern method (method of an extern object)
- action call
- built-in method (there are five of these: setValid, setInvalid, isValid, push, pop

See also the ConstructorCall class and the MethodCallDescription class
below.
*/
class MethodInstance : public InstanceBase {
 protected:
    MethodInstance(const IR::MethodCallExpression* mce,
                   const IR::IDeclaration* decl,
                   const IR::Type_MethodBase* originalMethodType,
                   const IR::Type_MethodBase* actualMethodType) :
            expr(mce), object(decl), originalMethodType(originalMethodType),
            actualMethodType(actualMethodType)
    { CHECK_NULL(mce); CHECK_NULL(originalMethodType); CHECK_NULL(actualMethodType); }

    void bindParameters() {
        auto params = getActualParameters();
        substitution.populate(params, expr->arguments);
    }

 public:
    const IR::MethodCallExpression* expr;
    /** Declaration of object that method is applied to.
        May be null for plain functions. */
    const IR::IDeclaration* object;
    /** The type of the *original* called method,
        without instantiated type parameters. */
    const IR::Type_MethodBase* originalMethodType;
    /** Type of called method,
        with instantiated type parameters. */
    const IR::Type_MethodBase* actualMethodType;
    virtual bool isApply() const { return false; }

    /** @param useExpressionType If true, the typeMap can be nullptr,
     *   and then mce->type is used.  For some technical reasons
     *   neither the refMap or the typeMap are const here.
     *  @param incomplete        If true we do not expect to have
     *                           all type arguments.
     */
    static MethodInstance* resolve(const IR::MethodCallExpression* mce,
                                   DeclarationLookup* refMap, TypeMap* typeMap,
                                   bool useExpressionType = false,
                                   const Visitor::Context *ctxt = nullptr,
                                   bool incomplete = false);
    static MethodInstance* resolve(const IR::MethodCallExpression* mce,
                                   DeclarationLookup* refMap, TypeMap* typeMap,
                                   const Visitor::Context *ctxt,
                                   bool incomplete = false)
    { return resolve(mce, refMap, typeMap, false, ctxt, incomplete); }
    static MethodInstance* resolve(const IR::MethodCallStatement* mcs,
                                   DeclarationLookup* refMap, TypeMap* typeMap,
                                   const Visitor::Context *ctxt = nullptr)
    { return resolve(mcs->methodCall, refMap, typeMap, false, ctxt, false); }
    static MethodInstance* resolve(const IR::MethodCallExpression* mce,
                                   DeclarationLookup* refMap,
                                   const Visitor::Context *ctxt = nullptr)
    { return resolve(mce, refMap, nullptr, true, ctxt, false); }
    static MethodInstance* resolve(const IR::MethodCallStatement* mcs,
                                   DeclarationLookup* refMap,
                                   const Visitor::Context *ctxt = nullptr)
    { return resolve(mcs->methodCall, refMap, nullptr, true, ctxt, false); }
    const IR::ParameterList* getOriginalParameters() const
    { return originalMethodType->parameters; }
    const IR::ParameterList* getActualParameters() const
    { return actualMethodType->parameters; }
};

/** Represents the call of an Apply method on an object that implements IApply:
    a table, control or parser */
class ApplyMethod final : public MethodInstance {
    ApplyMethod(const IR::MethodCallExpression* expr, const IR::IDeclaration* decl,
                const IR::IApply* applyObject) :
            MethodInstance(expr, decl, applyObject->getApplyMethodType(),
                           applyObject->getApplyMethodType()),
            applyObject(applyObject)
            { CHECK_NULL(applyObject); bindParameters(); }
    friend class MethodInstance;
 public:
    const IR::IApply* applyObject;
    bool isApply() const { return true; }
    bool isTableApply() const { return object->is<IR::P4Table>(); }
};

/** Represents a method call on an extern object */
class ExternMethod final : public MethodInstance {
    ExternMethod(const IR::MethodCallExpression* expr, const IR::IDeclaration* decl,
                 const IR::Method* method,
                 const IR::Type_Extern* originalExternType,
                 const IR::Type_Method* originalMethodType,
                 const IR::Type_Extern* actualExternType,
                 const IR::Type_Method* actualMethodType, bool incomplete) :
            MethodInstance(expr, decl, originalMethodType, actualMethodType), method(method),
            originalExternType(originalExternType), actualExternType(actualExternType) {
        CHECK_NULL(method); CHECK_NULL(originalExternType); CHECK_NULL(actualExternType);
        bindParameters();
        if (!incomplete)
            typeSubstitution.setBindings(expr, method->type->typeParameters, expr->typeArguments);
    }
    friend class MethodInstance;
 public:
    const IR::Method*      method;
    const IR::Type_Extern* originalExternType;    // type of object method is applied to
    const IR::Type_Extern* actualExternType;      // with type variables substituted

    /// Set of IR::Method and IR::Function objects that may be called by this method.
    // If this method is abstract, will consist of (just) the concrete implementation,
    // otherwise will consist of those methods that are @synchronous with this
    std::vector<const IR::IDeclaration *> mayCall() const;
};

/** Represents the call of an extern function */
class ExternFunction final : public MethodInstance {
    ExternFunction(const IR::MethodCallExpression* expr,
                   const IR::Method* method,
                   const IR::Type_Method* originalMethodType,
                   const IR::Type_Method* actualMethodType, bool incomplete) :
            MethodInstance(expr, nullptr, originalMethodType, actualMethodType), method(method) {
        CHECK_NULL(method);
        bindParameters();
        if (!incomplete)
            typeSubstitution.setBindings(expr, method->type->typeParameters, expr->typeArguments);
    }
    friend class MethodInstance;
 public:
    const IR::Method* method;
};

/** Represents the direct call of an action; This also works for
    correctly for actions declared in a table 'actions' list, and for
    action instantiations such as the default_action or the list of
    table entries */
class ActionCall final : public MethodInstance {
    ActionCall(const IR::MethodCallExpression* expr,
               const IR::P4Action* action,
               const IR::Type_Action* actionType) :
            // Actions are never generic
            MethodInstance(expr, nullptr, actionType, actionType), action(action)
    { CHECK_NULL(action); bindParameters(); }
    friend class MethodInstance;
 public:
    const IR::P4Action* action;
    /// Generate a version of the action where the parameters in the
    /// substitution have been replaced with the arguments.
    const IR::P4Action* specialize(ReferenceMap* refMap) const;
};

/**
  Represents the call of a function.
*/
class FunctionCall final : public MethodInstance {
    FunctionCall(const IR::MethodCallExpression* expr,
                 const IR::Function* function,
                 const IR::Type_Method* originalMethodType,
                 const IR::Type_Method* actualMethodType, bool incomplete) :
            MethodInstance(expr, nullptr, originalMethodType, actualMethodType),
            function(function) {
        CHECK_NULL(function);
        bindParameters();
        if (!incomplete)
            typeSubstitution.setBindings(
                function, function->type->typeParameters, expr->typeArguments);
    }
    friend class MethodInstance;
 public:
    const IR::Function* function;
};

/** This class represents the call of a built-in method:
These methods are:
- header.setValid(),
- header.setInvalid(),
- header.isValid(),
- union.isValid(),
- stack.push_front(int),
- stack.pop_front(int)
*/
class BuiltInMethod final : public MethodInstance {
    friend class MethodInstance;
    BuiltInMethod(const IR::MethodCallExpression* expr, IR::ID name,
                  const IR::Expression* appliedTo, const IR::Type_Method* methodType) :
            MethodInstance(expr, nullptr, methodType, methodType), name(name), appliedTo(appliedTo)
    { CHECK_NULL(appliedTo); bindParameters(); }
 public:
    const IR::ID name;
    const IR::Expression* appliedTo;  // object is an expression
};

////////////////////////////////////////////////////

/** This class is used to disambiguate constructor calls.
    The core method is the static method 'resolve', which will categorize a
    constructor as one of
    - Extern constructor
    - Container constructor (parser, control or package)
*/
class ConstructorCall : public InstanceBase {
 protected:
    virtual ~ConstructorCall() {}
    explicit ConstructorCall(const IR::ConstructorCallExpression* cce): cce(cce)
    { CHECK_NULL(cce); }
 public:
    const IR::ConstructorCallExpression* cce = nullptr;
    const IR::Vector<IR::Type>*          typeArguments = nullptr;
    const IR::ParameterList*             constructorParameters = nullptr;
    static ConstructorCall* resolve(const IR::ConstructorCallExpression* cce,
                                    DeclarationLookup* refMap,
                                    TypeMap* typeMap);
};

/** Represents a constructor call that allocates an Extern object */
class ExternConstructorCall : public ConstructorCall {
    explicit ExternConstructorCall(const IR::ConstructorCallExpression* cce,
                                   const IR::Type_Extern* type,
                                   const IR::Method* constructor) :
            ConstructorCall(cce), type(type), constructor(constructor)
    { CHECK_NULL(type); CHECK_NULL(constructor); }
    friend class ConstructorCall;
 public:
    const IR::Type_Extern* type;  // actual extern declaration in program IR
    const IR::Method* constructor;  // that is being invoked
};

/** Represents a constructor call that allocates an object that implements IContainer.
    These can be package, control or parser */
class ContainerConstructorCall : public ConstructorCall {
    explicit ContainerConstructorCall(const IR::ConstructorCallExpression* cce,
                                      const IR::IContainer* cont) :
            ConstructorCall(cce), container(cont) { CHECK_NULL(cont); }
    friend class ConstructorCall;
 public:
    const IR::IContainer* container;  // actual container in program IR
};

/////////////////////////////////////////////

/// Used to resolve a Declaration_Instance
class Instantiation : public InstanceBase {
 protected:
    void substitute() {
        substitution.populate(constructorParameters, constructorArguments);
        typeSubstitution.setBindings(instance, typeParameters, typeArguments);
    }

 public:
    Instantiation(const IR::Declaration_Instance* instance,
                  const IR::Vector<IR::Type>* typeArguments):
            instance(instance), typeArguments(typeArguments)
    { CHECK_NULL(instance); constructorArguments = instance->arguments; }

    const IR::Declaration_Instance* instance;
    const IR::Vector<IR::Type>*    typeArguments;
    const IR::Vector<IR::Argument>*constructorArguments;
    const IR::ParameterList*       constructorParameters;
    const IR::TypeParameters*      typeParameters;

    static Instantiation* resolve(const IR::Declaration_Instance* instance,
                                  DeclarationLookup* refMap,
                                  TypeMap* typeMap);
};

class ExternInstantiation : public Instantiation {
 public:
    ExternInstantiation(const IR::Declaration_Instance* instance,
                        const IR::Vector<IR::Type>* typeArguments,
                        const IR::Type_Extern* type) :
            Instantiation(instance, typeArguments), type(type) {
        auto constructor = type->lookupConstructor(constructorArguments);
        BUG_CHECK(constructor, "%1%: could not find constructor", type);
        constructorParameters = constructor->type->parameters;
        typeParameters = type->typeParameters;
        substitute();
    }
    const IR::Type_Extern* type;
};

class PackageInstantiation : public Instantiation {
 public:
    PackageInstantiation(const IR::Declaration_Instance* instance,
                         const IR::Vector<IR::Type>* typeArguments,
                         const IR::Type_Package* package) :
            Instantiation(instance, typeArguments), package(package) {
        constructorParameters = package->getConstructorParameters();
        typeParameters = package->typeParameters;
        substitute();
    }
    const IR::Type_Package*  package;
};

class ParserInstantiation : public Instantiation {
 public:
    ParserInstantiation(const IR::Declaration_Instance* instance,
                        const IR::Vector<IR::Type>* typeArguments,
                        const IR::P4Parser* parser) :
            Instantiation(instance, typeArguments), parser(parser) {
        typeParameters = parser->type->typeParameters;
        constructorParameters = parser->getConstructorParameters();
        substitute(); }
    const IR::P4Parser* parser;
};

class ControlInstantiation : public Instantiation {
 public:
    ControlInstantiation(const IR::Declaration_Instance* instance,
                         const IR::Vector<IR::Type>* typeArguments,
                         const IR::P4Control* control) :
            Instantiation(instance, typeArguments), control(control) {
        typeParameters = control->type->typeParameters;
        constructorParameters = control->getConstructorParameters();
        substitute(); }
    const IR::P4Control* control;
};

}  // namespace P4

#endif /* _FRONTENDS_P4_METHODINSTANCE_H_ */
