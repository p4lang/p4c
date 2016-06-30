/*
Copyright 2016 VMware, Inc.

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

#include <iostream>

#include "specialize.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/p4/evaluator/substituteParameters.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/p4/enumInstance.h"

namespace P4 {

std::vector<SpecializationInfo*>*
SpecializationMap::getSpecializations(const IR::IContainer* decl) {
    auto result = new std::vector<SpecializationInfo*>();
    for (auto n : newName) {
        auto invocation = n.first;
        SpecializationInfo* si = nullptr;
        if (invocation->is<IR::ConstructorCallExpression>()) {
            auto cce = invocation->to<IR::ConstructorCallExpression>();
            auto cc = ConstructorCall::resolve(cce, refMap, typeMap);
            auto ccc = cc->to<ContainerConstructorCall>();
            CHECK_NULL(ccc);
            if (ccc->container != decl)
                continue;
            si = new SpecializationInfo();
            si->constructorArguments = cce->arguments;
            si->typeArguments = ccc->typeArguments;
        } else {
            auto di = invocation->to<IR::Declaration_Instance>();
            const IR::Type_Name* type;
            const IR::Vector<IR::Type>* typeArgs;
            if (di->type->is<IR::Type_Specialized>()) {
                auto ts = di->type->to<IR::Type_Specialized>();
                type = ts->baseType;
                typeArgs = ts->arguments;
            } else {
                type = di->type->to<IR::Type_Name>();
                typeArgs = new IR::Vector<IR::Type>();
            }
            CHECK_NULL(type);
            auto d = refMap->getDeclaration(type->path, true);
            if (d->getNode() != decl->getNode())
                continue;
            si = new SpecializationInfo();
            si->constructorArguments = di->arguments;
            si->typeArguments = typeArgs;
        }

        if (si == nullptr) continue;
        si->name = n.second;
        si->invocation = invocation;
        result->push_back(si);
    }
    if (result->empty())
        return nullptr;
    return result;
}

namespace {

class IsConcreteType : public Inspector {
    const TypeMap* typeMap;
 public:
    bool hasTypeVariables = false;

    IsConcreteType(const TypeMap* typeMap) : typeMap(typeMap) { CHECK_NULL(typeMap); }
    void postorder(const IR::Type_Var*) override { hasTypeVariables = true; }
    void postorder(const IR::Type_Name* type) override {
        auto t = typeMap->getType(type, true);
        if (t->is<IR::Type_Var>())
            hasTypeVariables = true;
    }

    bool isConcrete(const IR::Type* type) {
        type->apply(*this);
        return hasTypeVariables;
    }
};

}  // namespace

bool FindSpecializations::isSimpleConstant(const IR::Expression* expr) const {
    CHECK_NULL(expr);
    if (expr->is<IR::Constant>())
        return true;
    if (expr->is<IR::BoolLiteral>())
        return true;
    if (expr->is<IR::ListExpression>()) {
        auto list = expr->to<IR::ListExpression>();
        for (auto e : *list->components)
            if (!isSimpleConstant(e))
                return false;
        return true;
    }
    auto ei = EnumInstance::resolve(expr, typeMap);
    if (ei != nullptr)
        return true;
    return false;
}

void FindSpecializations::postorder(const IR::ConstructorCallExpression* expression) {
    if (expression->arguments->size() == 0 &&
        !expression->constructedType->is<IR::Type_Specialized>())
        return;  // nothing to specialize

    auto type = typeMap->getType(expression, true);
    if (type->is<IR::Type_SpecializedCanonical>()) {
        auto ts = type->to<IR::Type_SpecializedCanonical>();
        IsConcreteType ct(typeMap);
        if (!ct.isConcrete(ts))
            return;  // no point in specializing if arguments have type variables
        type = ts->baseType;
    }
    if (type->is<IR::P4Parser>())
        type = type->to<IR::P4Parser>()->type;
    else if (type->is<IR::P4Control>())
        type = type->to<IR::P4Control>()->type;
    if (!type->is<IR::Type_Control>() && !type->is<IR::Type_Parser>())
        return;
    for (auto arg : *expression->arguments) {
        if (!isSimpleConstant(arg))
            return;
    }

    auto archtype = type->to<IR::Type_ArchBlock>();
    cstring newName = refMap->newName(archtype->getName());
    LOG1("Will specialize " << expression << " into " << newName);
    specMap->newName.emplace(expression, newName);
}

void FindSpecializations::postorder(const IR::Declaration_Instance* decl) {
    if (decl->arguments->size() == 0 &&
        !decl->type->is<IR::Type_Specialized>())
        return;

    auto type = typeMap->getType(decl, true);
    if (type->is<IR::Type_SpecializedCanonical>()) {
        auto ts = type->to<IR::Type_SpecializedCanonical>();
        IsConcreteType ct(typeMap);
        if (!ct.isConcrete(ts))
            return;  // no point in specializing if arguments have type variables
        type = ts->baseType;
    }
    if (!type->is<IR::Type_Control>() && !type->is<IR::Type_Parser>())
        return;
    for (auto arg : *decl->arguments) {
        if (!isSimpleConstant(arg))
            return;
    }

    auto archtype = type->to<IR::Type_ArchBlock>();
    cstring newName = refMap->newName(archtype->getName());
    LOG1("Will specialize " << decl << " into " << newName);
    specMap->newName.emplace(decl, newName);
}

const IR::Node* Specialize::postorder(IR::P4Parser* parser) {
    auto orig = getOriginal<IR::P4Parser>();
    auto specs = specMap->getSpecializations(orig);
    if (specs == nullptr)
        return parser;

    auto result = new IR::IndexedVector<IR::Node>();
    for (auto s : *specs) {
        TypeVariableSubstitution tvs;
        ParameterSubstitution    subst;

        subst.populate(parser->getConstructorParameters(), s->constructorArguments);
        tvs.setBindings(s->invocation, parser->getTypeParameters(), s->typeArguments);
        SubstituteParameters sp(refMap, &subst, &tvs);
        auto clone = orig->apply(sp)->to<IR::P4Parser>();
        CHECK_NULL(clone);
        auto newtype = new IR::Type_Parser(s->name, clone->type->annotations,
                                           new IR::TypeParameters(),
                                           clone->type->applyParams);
        auto prs = new IR::P4Parser(s->name, newtype, new IR::ParameterList(),
                                    clone->stateful, clone->states);
        LOG1("Created " << prs << " for " << parser);
        result->push_back(prs);
    }
    result->push_back(parser);
    return result;
}

const IR::Node* Specialize::postorder(IR::P4Control* control) {
    auto orig = getOriginal<IR::P4Control>();
    auto specs = specMap->getSpecializations(orig);
    if (specs == nullptr)
        return control;

    auto result = new IR::IndexedVector<IR::Node>();
    for (auto s : *specs) {
        TypeVariableSubstitution tvs;
        ParameterSubstitution    subst;

        subst.populate(control->getConstructorParameters(), s->constructorArguments);
        tvs.setBindings(s->invocation, control->getTypeParameters(), s->typeArguments);
        SubstituteParameters sp(refMap, &subst, &tvs);
        auto clone = orig->apply(sp)->to<IR::P4Control>();
        CHECK_NULL(clone);
        auto newtype = new IR::Type_Control(s->name, clone->type->annotations,
                                            new IR::TypeParameters(),
                                            clone->type->applyParams);
        auto ctrl = new IR::P4Control(s->name, newtype, new IR::ParameterList(),
                                     clone->stateful, clone->body);
        LOG1("Created " << ctrl << " for " << control);
        result->push_back(ctrl);
    }
    result->push_back(control);  // maybe it's still needed
    return result;
}

const IR::Node* Specialize::postorder(IR::ConstructorCallExpression* expression) {
    if (!specMap->isSpecialized(getOriginal()))
        return expression;
    cstring newName = specMap->get(getOriginal());
    auto typeRef = new IR::Type_Name(IR::ID(newName));
    auto result = new IR::ConstructorCallExpression(typeRef, new IR::Vector<IR::Expression>());
    LOG1("Replaced " << expression << " with " << result);
    return result;
}

const IR::Node* Specialize::postorder(IR::Declaration_Instance* decl) {
    if (!specMap->isSpecialized(getOriginal()))
        return decl;
    cstring newName = specMap->get(getOriginal());
    auto typeRef = new IR::Type_Name(IR::ID(newName));
    auto result = new IR::Declaration_Instance(
        decl->name, decl->annotations, typeRef, new IR::Vector<IR::Expression>(),
        decl->initializer);
    LOG1("Replaced " << decl << " with " << result);
    return result;
}

SpecializeAll::SpecializeAll(ReferenceMap* refMap, TypeMap* typeMap, bool isv1) :
        PassRepeated({}) {
    passes.emplace_back(new TypeChecking(refMap, typeMap, false, isv1));
    passes.emplace_back(new ConstantFolding(refMap, typeMap));
    passes.emplace_back(new TypeChecking(refMap, typeMap, false, isv1));
    passes.emplace_back(new FindSpecializations(refMap, typeMap, &specMap));
    passes.emplace_back(new Specialize(refMap, &specMap));
    specMap.refMap = refMap;
    specMap.typeMap = typeMap;
    setName("SpecializeAll");
}

}  // namespace P4
