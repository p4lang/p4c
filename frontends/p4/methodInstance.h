#ifndef _FRONTENDS_P4_METHODINSTANCE_H_
#define _FRONTENDS_P4_METHODINSTANCE_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/typeMap.h"

namespace P4 {

// Represents compile-time information about a MethodCallExpression
class MethodInstance {
 protected:
    MethodInstance(const IR::MethodCallExpression* mce,
                   const IR::IDeclaration* decl) :
            expr(mce), object(decl) { CHECK_NULL(mce); }
 public:
    const IR::MethodCallExpression* expr;
    const IR::IDeclaration* object;  // Object that method is applied to.
                                     // May be null for plain functions.
    virtual bool isApply() const { return false; }
    virtual ~MethodInstance() {}

    static MethodInstance* resolve(const IR::MethodCallExpression* mce,
                                   const P4::ReferenceMap* refMap,
                                   const P4::TypeMap* typeMap,
                                   bool useExpressionType = false);
    static MethodInstance* resolve(const IR::MethodCallStatement* mcs,
                                   const P4::ReferenceMap* refMap,
                                   const P4::TypeMap* typeMap)
        { return resolve(mcs->methodCall, refMap, typeMap); }
    template<typename T> bool is() const { return to<T>() != nullptr; }
    template<typename T> const T* to() const { return dynamic_cast<const T*>(this); }
};

// The call of an Apply method on an object that implements IApply
class ApplyMethod final : public MethodInstance {
    ApplyMethod(const IR::MethodCallExpression* expr, const IR::IDeclaration* decl,
                const IR::IApply* type) :
            MethodInstance(expr, decl), type(type) { CHECK_NULL(type); }
    friend class MethodInstance;
 public:
    const IR::IApply* type;
    bool isApply() const { return true; }
    const IR::ParameterList* getParameters() const
    { return type->getApplyMethodType()->parameters; }
};

// A method call on an extern object
class ExternMethod final : public MethodInstance {
    ExternMethod(const IR::MethodCallExpression* expr, const IR::IDeclaration* decl,
                 const IR::Method* method, const IR::Type_Extern* type,
                 const IR::Type_Method* methodType) :
            MethodInstance(expr, decl), method(method), type(type), methodType(methodType)
    { CHECK_NULL(method); CHECK_NULL(type); CHECK_NULL(methodType); }
    friend class MethodInstance;
 public:
    const IR::Method*      method;
    const IR::Type_Extern* type;    // type of object method is applied to
    const IR::Type_Method* methodType;  // actual type of called method
    // (may be different from method->type, if the method->type is generic).
};

// An extern function
class ExternFunction final : public MethodInstance {
    ExternFunction(const IR::MethodCallExpression* expr,
                   const IR::Method* method, const IR::Type_Method* methodType) :
            MethodInstance(expr, nullptr), method(method), methodType(methodType)
    { CHECK_NULL(method); CHECK_NULL(methodType); }
    friend class MethodInstance;
 public:
    const IR::Method* method;
    const IR::Type_Method* methodType;  // actual type of called method
    // (may be different from method->type, if method->type is generic).
};

// Calling an action directly
class ActionCall final : public MethodInstance {
    ActionCall(const IR::MethodCallExpression* expr,
               const IR::P4Action* action) :
            MethodInstance(expr, nullptr), action(action) { CHECK_NULL(action); }
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
                  const IR::Expression* appliedTo) :
            MethodInstance(expr, nullptr), name(name), appliedTo(appliedTo)
    { CHECK_NULL(appliedTo); }
 public:
    const IR::ID name;
    const IR::Expression* appliedTo;  // object is an expression
};

// Similar to a MethodInstance, but for a constructor call expression
class ConstructorCall {
 protected:
    explicit ConstructorCall(const IR::ConstructorCallExpression* cce) : cce(cce)
    { CHECK_NULL(cce); }
    virtual ~ConstructorCall() {}
 public:
    const IR::ConstructorCallExpression* cce;
    static ConstructorCall* resolve(const IR::ConstructorCallExpression* mce,
                                    const P4::TypeMap* typeMap);
    template<typename T> bool is() const { return to<T>() != nullptr; }
    template<typename T> const T* to() const { return dynamic_cast<const T*>(this); }
};

class ExternConstructorCall : public ConstructorCall {
    ExternConstructorCall(const IR::ConstructorCallExpression* cce, const IR::Type_Extern* type) :
            ConstructorCall(cce), type(type) { CHECK_NULL(type); }
    friend class ConstructorCall;
 public:
    const IR::Type_Extern* type;
};

class ContainerConstructorCall : public ConstructorCall {
    ContainerConstructorCall(const IR::ConstructorCallExpression* cce, const IR::IContainer* cont) :
            ConstructorCall(cce), container(cont) { CHECK_NULL(cont); }
    friend class ConstructorCall;
 public:
    const IR::IContainer* container;
};

}  // namespace P4

#endif /* _FRONTENDS_P4_METHODINSTANCE_H_ */
