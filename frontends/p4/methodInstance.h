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
class MethodInstance {
 protected:
    MethodInstance(const IR::MethodCallExpression* mce,
                   const IR::IDeclaration* decl,
                   const IR::Type_MethodBase* originalMethodType,
                   const IR::Type_MethodBase* actualMethodType) :
            expr(mce), object(decl), originalMethodType(originalMethodType),
            actualMethodType(actualMethodType)
    { CHECK_NULL(mce); CHECK_NULL(originalMethodType); CHECK_NULL(actualMethodType); }

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
    virtual ~MethodInstance() {}

    /** @param useExpressionType If true, the typeMap can be nullptr,
        and then mce->type is used.  For some technical reasons
        neither the refMap or the typeMap are const here.  */
    static MethodInstance* resolve(const IR::MethodCallExpression* mce,
                                   ReferenceMap* refMap, TypeMap* typeMap,
                                   bool useExpressionType = false);
    static MethodInstance* resolve(const IR::MethodCallStatement* mcs,
                                   ReferenceMap* refMap, TypeMap* typeMap)
        { return resolve(mcs->methodCall, refMap, typeMap); }
    const IR::ParameterList* getOriginalParameters() const
    { return originalMethodType->parameters; }
    const IR::ParameterList* getActualParameters() const
    { return actualMethodType->parameters; }
    template<typename T> bool is() const { return to<T>() != nullptr; }
    template<typename T> const T* to() const { return dynamic_cast<const T*>(this); }
};

/** Represents the call of an Apply method on an object that implements IApply:
    a table, control or parser */
class ApplyMethod final : public MethodInstance {
    ApplyMethod(const IR::MethodCallExpression* expr, const IR::IDeclaration* decl,
                const IR::IApply* applyObject) :
            // TODO: is this correct?
            MethodInstance(expr, decl, applyObject->getApplyMethodType(),
                           applyObject->getApplyMethodType()),
            applyObject(applyObject)
    { CHECK_NULL(applyObject); }
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
                 const IR::Type_Method* actualMethodType) :
            MethodInstance(expr, decl, originalMethodType, actualMethodType), method(method),
            originalExternType(originalExternType), actualExternType(actualExternType)
    { CHECK_NULL(method); CHECK_NULL(originalExternType); CHECK_NULL(actualExternType); }
    friend class MethodInstance;
 public:
    const IR::Method*      method;
    const IR::Type_Extern* originalExternType;    // type of object method is applied to
    const IR::Type_Extern* actualExternType;      // with type variables substituted
};

/** Represents the call of an extern function */
class ExternFunction final : public MethodInstance {
    ExternFunction(const IR::MethodCallExpression* expr,
                   const IR::Method* method,
                   const IR::Type_Method* originalMethodType,
                   const IR::Type_Method* actualMethodType) :
            MethodInstance(expr, nullptr, originalMethodType, actualMethodType), method(method)
    { CHECK_NULL(method); }
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
    { CHECK_NULL(action); }
    friend class MethodInstance;
 public:
    const IR::P4Action* action;
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
    { CHECK_NULL(appliedTo); }
 public:
    const IR::ID name;
    const IR::Expression* appliedTo;  // object is an expression
};

/** This class is used to disambiguate constructor calls.
    The core method is the static method 'resolve', which will categorize a
    constructor as one of
    - Extern constructor
    - Container constructor (parser, control or package)
*/
class ConstructorCall {
 protected:
    virtual ~ConstructorCall() {}
 public:
    const IR::ConstructorCallExpression* cce;
    const IR::Vector<IR::Type>*          typeArguments;
    static ConstructorCall* resolve(const IR::ConstructorCallExpression* cce,
                                    ReferenceMap* refMap,
                                    TypeMap* typeMap);
    template<typename T> bool is() const { return to<T>() != nullptr; }
    template<typename T> const T* to() const { return dynamic_cast<const T*>(this); }
};

/** Represents a constructor call that allocates an Extern object */
class ExternConstructorCall : public ConstructorCall {
    explicit ExternConstructorCall(const IR::Type_Extern* type) :
            type(type) { CHECK_NULL(type); }
    friend class ConstructorCall;
 public:
    const IR::Type_Extern* type;  // actual extern declaration in program IR
};

/** Represents a constructor call that allocates an object that implements IContainer.
    These can be package, control or parser */
class ContainerConstructorCall : public ConstructorCall {
    explicit ContainerConstructorCall(const IR::IContainer* cont) :
            container(cont) { CHECK_NULL(cont); }
    friend class ConstructorCall;
 public:
    const IR::IContainer* container;  // actual container in program IR
};

/**
   Abstraction for a method call: in addition to information about the
   MethodInstance, this class also maintains a mapping between
   arguments and the corresponding parameters.  This will make it
   easier to introduce different calling conventions in the future,
   e.g. calls by specifying the name of the parameter.

   TODO: Today not all code paths use this class for matching
   arguments to parameters; we should convert all code to use this
   class.
*/
class MethodCallDescription {
 public:
    MethodInstance       *instance;
    /// For each callee parameter the corresponding argument
    ParameterSubstitution substitution;

    MethodCallDescription(const IR::MethodCallExpression* mce,
                          ReferenceMap* refMap, TypeMap* typeMap);
};

}  // namespace P4

#endif /* _FRONTENDS_P4_METHODINSTANCE_H_ */
