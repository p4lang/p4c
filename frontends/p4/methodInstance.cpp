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
#include "frontends/p4/evaluator/substituteParameters.h"

namespace P4 {

// If useExpressionType is true trust the type in mce->type
MethodInstance*
MethodInstance::resolve(const IR::MethodCallExpression* mce, DeclarationLookup* refMap,
                        TypeMap* typeMap, bool useExpressionType, const Visitor::Context *ctxt,
                        bool incomplete) {
    auto mt = typeMap ? typeMap->getType(mce->method) : nullptr;
    if (mt == nullptr && useExpressionType)
        mt = mce->method->type;
    CHECK_NULL(mt);
    BUG_CHECK(mt->is<IR::Type_MethodBase>(), "%1%: expected a MethodBase type", mt);
    auto originalType = mt->to<IR::Type_MethodBase>();
    auto actualType = originalType;
    if (typeMap && !mce->typeArguments->empty()) {
        auto t = TypeInference::specialize(originalType, mce->typeArguments);
        CHECK_NULL(t);
        actualType = t->to<IR::Type_MethodBase>();
        // FIXME -- currently refMap is always a ReferenceMap, but this arg should soon go away
        TypeInference tc(dynamic_cast<ReferenceMap*>(refMap), typeMap, true);
        (void)actualType->apply(tc, ctxt);  // may need to learn new type components
        CHECK_NULL(actualType);
    }
    // mt can be Type_Method or Type_Action
    if (mce->method->is<IR::Member>()) {
        auto mem = mce->method->to<IR::Member>();
        auto basetype = typeMap ? typeMap->getType(mem->expr) : mem->expr->type;
        if (basetype == nullptr) {
            if (useExpressionType)
                basetype = mem->expr->type;
            else
                BUG("Could not find type for %1%", mem->expr);
        }
        if (auto sc = basetype->to<IR::Type_SpecializedCanonical>())
            basetype = sc->baseType;
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
                type = typeMap ? typeMap->getType(decl->getNode()) : pe->type;
            } else if (auto mc = mem->expr->to<IR::MethodCallExpression>()) {
                auto mi = resolve(mc, refMap, typeMap, useExpressionType);
                decl = mi->object;
                type = mi->actualMethodType->returnType;
            } else if (auto cce = mem->expr->to<IR::ConstructorCallExpression>()) {
                auto cc = ConstructorCall::resolve(cce, refMap, typeMap);
                decl = cc->to<ExternConstructorCall>()->type;
                type = typeMap ? typeMap->getTypeType(cce->constructedType, true) : cce->type;
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
                auto method = et->lookupMethod(mem->member, mce->arguments);
                if (method == nullptr)
                    return nullptr;
                return new ExternMethod(mce, decl, method, et, methodType,
                                        type->to<IR::Type_Extern>(),
                                        actualType->to<IR::Type_Method>(), incomplete);
            }
        }
    } else if (mce->method->is<IR::PathExpression>()) {
        auto pe = mce->method->to<IR::PathExpression>();
        auto decl = refMap->getDeclaration(pe->path, true);
        if (auto meth = decl->to<IR::Method>()) {
            auto methodType = mt->to<IR::Type_Method>();
            CHECK_NULL(methodType);
            return new ExternFunction(mce, meth, methodType,
                                      actualType->to<IR::Type_Method>(), incomplete);
        } else if (auto act = decl->to<IR::P4Action>()) {
            return new ActionCall(mce, act, mt->to<IR::Type_Action>());
        } else if (auto func = decl->to<IR::Function>()) {
            auto methodType = mt->to<IR::Type_Method>();
            CHECK_NULL(methodType);
            return new FunctionCall(mce, func, methodType,
                                    actualType->to<IR::Type_Method>(), incomplete);
        }
    }

    BUG("Unexpected method call %1%", mce);
    return nullptr;  // unreachable
}

const IR::P4Action* ActionCall::specialize(ReferenceMap* refMap) const {
    SubstituteParameters sp(refMap, &substitution, new TypeVariableSubstitution());
    auto result = action->apply(sp);
    return result->to<IR::P4Action>();
}

ConstructorCall*
ConstructorCall::resolve(const IR::ConstructorCallExpression* cce,
                         DeclarationLookup* refMap, TypeMap* typeMap) {
    auto ct = typeMap ? typeMap->getTypeType(cce->constructedType, true) : cce->type;
    ConstructorCall* result;
    const IR::Vector<IR::Type>* typeArguments;
    const IR::Type_Name* type;
    const IR::ParameterList* constructorParameters;

    if (cce->constructedType->is<IR::Type_Specialized>()) {
        auto spec = cce->constructedType->to<IR::Type_Specialized>();
        type = spec->baseType;
        typeArguments = cce->constructedType->to<IR::Type_Specialized>()->arguments;
    } else {
        type = cce->constructedType->to<IR::Type_Name>();
        CHECK_NULL(type);
        typeArguments = new IR::Vector<IR::Type>();
    }

    if (auto tsc = ct->to<IR::Type_SpecializedCanonical>())
        ct = typeMap ? typeMap->getTypeType(tsc->baseType, true) : tsc;

    if (ct->is<IR::Type_Extern>()) {
        auto decl = refMap->getDeclaration(type->path, true);
        auto ext = decl->to<IR::Type_Extern>();
        BUG_CHECK(ext, "%1%: expected an extern type", dbp(decl));
        auto constr = ext->lookupConstructor(cce->arguments);
        result = new ExternConstructorCall(cce, ext->to<IR::Type_Extern>(), constr);
        BUG_CHECK(constr, "%1%: constructor not found", ext);
        constructorParameters = constr->type->parameters;
    } else if (ct->is<IR::IContainer>()) {
        auto decl = refMap->getDeclaration(type->path, true);
        auto cont = decl->to<IR::IContainer>();
        BUG_CHECK(cont, "%1%: expected a container", dbp(decl));
        result = new ContainerConstructorCall(cce, cont);
        constructorParameters = cont->getConstructorParameters();
    } else {
        BUG("Unexpected constructor call %1%; type is %2%", dbp(cce), dbp(ct));
    }
    result->typeArguments = typeArguments;
    result->constructorParameters = constructorParameters;
    result->substitution.populate(result->constructorParameters, cce->arguments);
    return result;
}

Instantiation* Instantiation::resolve(const IR::Declaration_Instance* instance,
                                      DeclarationLookup* ,
                                      TypeMap* typeMap) {
    auto type = typeMap ? typeMap->getTypeType(instance->type, true) : instance->type;
    auto simpleType = type;
    const IR::Vector<IR::Type>* typeArguments;

    if (auto st = type->to<IR::Type_SpecializedCanonical>()) {
        simpleType = st->baseType;
        typeArguments = st->arguments;
    } else {
        typeArguments = new IR::Vector<IR::Type>();
    }

    if (auto et = simpleType->to<IR::Type_Extern>()) {
        return new ExternInstantiation(instance, typeArguments, et);
    } else if (auto pt = simpleType->to<IR::Type_Package>()) {
        return new PackageInstantiation(instance, typeArguments, pt);
    } else if (auto pt = simpleType->to<IR::P4Parser>()) {
        return new ParserInstantiation(instance, typeArguments, pt);
    } else if (auto ct = simpleType->to<IR::P4Control>()) {
        return new ControlInstantiation(instance, typeArguments, ct);
    }
    BUG("Unexpected instantiation %1%", instance);
    return nullptr;  // unreachable
}

std::vector<const IR::IDeclaration *> ExternMethod::mayCall() const {
    std::vector<const IR::IDeclaration *> rv;
    auto *di = object->to<IR::Declaration_Instance>();
    if (!di || !di->initializer) {
        rv.push_back(method);
    } else if (auto *em_decl = di->initializer->components
                                .getDeclaration<IR::IDeclaration>(method->name)) {
        rv.push_back(em_decl);
    } else {
        for (auto meth : originalExternType->methods) {
            auto sync = meth->getAnnotation(IR::Annotation::synchronousAnnotation);
            if (!sync) continue;
            for (auto m : sync->expr) {
                auto mname = m->to<IR::PathExpression>();
                if (!mname ||  method->name != mname->path->name)
                    continue;
                if (auto *am = di->initializer->components
                                .getDeclaration<IR::IDeclaration>(meth->name)) {
                    rv.push_back(am);
                } else if (!meth->getAnnotation(IR::Annotation::optionalAnnotation)) {
                    error(ErrorType::ERR_INVALID,
                          "No implementation for abstract %s in %s called via %s",
                          meth, di, method); } } } }
    return rv;
}

}  // namespace P4
