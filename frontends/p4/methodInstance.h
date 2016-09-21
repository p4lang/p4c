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

// Represents compile-time information about a MethodCallExpression
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
    const IR::IDeclaration* object;  // Declaration of object that method is applied to.
                                     // May be null for plain functions.
    const IR::Type_MethodBase* originalMethodType;  // the type of the *original* called method,
                                                    // without instantiated type parameters.
    const IR::Type_MethodBase* actualMethodType;  // type of called method,
                                                  // with instantiated type parameters.

    virtual bool isApply() const { return false; }
    virtual ~MethodInstance() {}

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

// The call of an Apply method on an object that implements IApply
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
};

// A method call on an extern object
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

// An extern function
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

// Calling an action directly
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

// A built-in method.
// These methods are:
// header.setValid(), header.setInvalid(), header.isValid(), stack.push(int), stack.pop(int)
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

// Similar to a MethodInstance, but for a constructor call expression
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

class ExternConstructorCall : public ConstructorCall {
    explicit ExternConstructorCall(const IR::Type_Extern* type) :
            type(type) { CHECK_NULL(type); }
    friend class ConstructorCall;
 public:
    const IR::Type_Extern* type;  // actual extern declaration in program IR
};

class ContainerConstructorCall : public ConstructorCall {
    explicit ContainerConstructorCall(const IR::IContainer* cont) :
            container(cont) { CHECK_NULL(cont); }
    friend class ConstructorCall;
 public:
    const IR::IContainer* container;  // actual container in program IR
};

// Abstraction for a method call: maintains mapping between arguments
// and parameters.  This will make it easier to introduce different
// calling conventions in the future.
// TODO: convert all code to use this class.
class MethodCallDescription {
 public:
    MethodInstance       *instance;
    // For each callee parameter the corresponding argument
    ParameterSubstitution substitution;

    MethodCallDescription(const IR::MethodCallExpression* mce,
                          ReferenceMap* refMap, TypeMap* typeMap);
};

}  // namespace P4

#endif /* _FRONTENDS_P4_METHODINSTANCE_H_ */
