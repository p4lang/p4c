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

#include "methodInstance.h"
#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

// If useExpressionType is true trust the type in mce->type
MethodInstance*
MethodInstance::resolve(const IR::MethodCallExpression* mce, ReferenceMap* refMap,
                        TypeMap* typeMap, bool useExpressionType) {
    auto mt = typeMap->getType(mce->method);
    if (mt == nullptr && useExpressionType)
        mt = mce->method->type;
    CHECK_NULL(mt);
    BUG_CHECK(mt->is<IR::Type_MethodBase>(), "%1%: expected a MethodBase type", mt);
    auto originalType = mt->to<IR::Type_MethodBase>();
    auto actualType = originalType;
    if (!mce->typeArguments->empty()) {
        auto t = TypeInference::specialize(originalType, mce->typeArguments);
        CHECK_NULL(t);
        actualType = t->to<IR::Type_MethodBase>();
        TypeInference tc(refMap, typeMap, true);
        (void)actualType->apply(tc);  // may need to learn new type components
        CHECK_NULL(actualType);
    }
    // mt can be Type_Method or Type_Action
    if (mce->method->is<IR::Member>()) {
        auto mem = mce->method->to<IR::Member>();
        auto basetype = typeMap->getType(mem->expr);
        if (basetype == nullptr) {
            if (useExpressionType)
                basetype = mem->expr->type;
            else
                BUG("Could not find type for %1%", mem->expr);
        }
        if (basetype->is<IR::Type_HeaderUnion>()) {
            if (mem->member == IR::Type_Header::isValid)
                return new BuiltInMethod(mce, mem->member, mem->expr, mt->to<IR::Type_Method>());
        } else if (basetype->is<IR::Type_Header>()) {
            if (mem->member == IR::Type_Header::setValid ||
                mem->member == IR::Type_Header::setInvalid ||
                mem->member == IR::Type_Header::isValid)
                return new BuiltInMethod(mce, mem->member, mem->expr, mt->to<IR::Type_Method>());
        } else if (basetype->is<IR::Type_Stack>()) {
            if (mem->member == IR::Type_Stack::push_front ||
                mem->member == IR::Type_Stack::pop_front)
                return new BuiltInMethod(mce, mem->member, mem->expr, mt->to<IR::Type_Method>());
        } else {
            const IR::IDeclaration *decl = nullptr;
            const IR::Type *type = nullptr;
            if (auto th = mem->expr->to<IR::This>()) {
                type = basetype;
                decl = refMap->getDeclaration(th, true);
            } else if (auto pe = mem->expr->to<IR::PathExpression>()) {
                decl = refMap->getDeclaration(pe->path, true);
                type = typeMap->getType(decl->getNode());
            } else if (auto mc = mem->expr->to<IR::MethodCallExpression>()) {
                auto mi = resolve(mc, refMap, typeMap, useExpressionType);
                decl = mi->object;
                type = mi->actualMethodType->returnType;
            } else if (auto cce = mem->expr->to<IR::ConstructorCallExpression>()) {
                auto cc = ConstructorCall::resolve(cce, refMap, typeMap);
                decl = cc->to<ExternConstructorCall>()->type;
                type = typeMap->getTypeType(cce->constructedType, true);
            } else {
                BUG("unexpected expression %1% resolving method instance", mem->expr); }
            if (type->is<IR::Type_SpecializedCanonical>())
                type = type->to<IR::Type_SpecializedCanonical>()->substituted->to<IR::Type>();
            BUG_CHECK(type != nullptr, "Could not resolve type for %1%", decl);
            if (type->is<IR::IApply>() &&
                mem->member == IR::IApply::applyMethodName) {
                return new ApplyMethod(mce, decl, type->to<IR::IApply>());
            } else if (type->is<IR::Type_Extern>()) {
                auto et = type->to<IR::Type_Extern>();
                auto methodType = mt->to<IR::Type_Method>();
                CHECK_NULL(methodType);
                auto method = et->lookupMethod(mem->member, mce->arguments->size());
                // TODO: do we need to also substitute the extern instantiation type
                // parameters into actualMethodType?
                return new ExternMethod(mce, decl, method, et, methodType,
                                        type->to<IR::Type_Extern>(),
                                        actualType->to<IR::Type_Method>());
            }
        }
    } else if (mce->method->is<IR::PathExpression>()) {
        auto pe = mce->method->to<IR::PathExpression>();
        auto decl = refMap->getDeclaration(pe->path, true);
        if (decl->is<IR::Method>()) {
            auto methodType = mt->to<IR::Type_Method>();
            CHECK_NULL(methodType);
            return new ExternFunction(mce, decl->to<IR::Method>(), methodType,
                                      actualType->to<IR::Type_Method>());
        } else if (decl->is<IR::P4Action>()) {
            return new ActionCall(mce, decl->to<IR::P4Action>(), mt->to<IR::Type_Action>());
        }
    }

    BUG("Unexpected method call %1%", mce);
    return nullptr;  // unreachable
}

ConstructorCall*
ConstructorCall::resolve(const IR::ConstructorCallExpression* cce,
                         ReferenceMap* refMap, TypeMap* typeMap) {
    auto t = typeMap->getTypeType(cce->constructedType, true);
    ConstructorCall* result;
    const IR::Vector<IR::Type>* typeArguments = nullptr;
    const IR::Type_Name* type;

    if (cce->constructedType->is<IR::Type_Specialized>()) {
        auto spec = cce->constructedType->to<IR::Type_Specialized>();
        type = spec->baseType;
        typeArguments = cce->constructedType->to<IR::Type_Specialized>()->arguments;
    } else {
        type = cce->constructedType->to<IR::Type_Name>();
        CHECK_NULL(type);
        typeArguments = new IR::Vector<IR::Type>();
    }

    if (t->is<IR::Type_SpecializedCanonical>()) {
        auto tsc = t->to<IR::Type_SpecializedCanonical>();
        t = typeMap->getTypeType(tsc->baseType, true);
    }

    if (t->is<IR::Type_Extern>()) {
        auto ext = refMap->getDeclaration(type->path, true);
        BUG_CHECK(ext->is<IR::Type_Extern>(), "%1%: expected an extern type", dbp(ext));
        result = new ExternConstructorCall(ext->to<IR::Type_Extern>());
    } else if (t->is<IR::IContainer>()) {
        auto cont = refMap->getDeclaration(type->path, true);
        BUG_CHECK(cont->is<IR::IContainer>(), "%1%: expected a container", dbp(cont));
        result = new ContainerConstructorCall(cont->to<IR::IContainer>());
    } else {
        BUG("Unexpected constructor call %1%; type is %2%", dbp(cce), dbp(t));
    }
    result->cce = cce;
    result->typeArguments = typeArguments;
    return result;
}

MethodCallDescription::MethodCallDescription(const IR::MethodCallExpression* mce,
                                             ReferenceMap* refMap, TypeMap* typeMap) {
    instance = MethodInstance::resolve(mce, refMap, typeMap);
    auto params = instance->getActualParameters();
    substitution.populate(params, mce->arguments);
}

}  // namespace P4
