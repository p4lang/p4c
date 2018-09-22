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
#include "frontends/p4/unusedDeclarations.h"

namespace P4 {

const IR::Type_Declaration* SpecializationInfo::synthesize(ReferenceMap* refMap) const {
    TypeVariableSubstitution tvs;
    ParameterSubstitution    subst;

    subst.populate(specialized->getConstructorParameters(), constructorArguments);
    tvs.setBindings(invocation, specialized->getTypeParameters(), typeArguments);
    SubstituteParameters sp(refMap, &subst, &tvs);
    auto clone = specialized->getNode()->apply(sp);
    CHECK_NULL(clone);
    const IR::Type_Declaration* result = nullptr;
    if (clone->is<IR::P4Parser>()) {
        auto parser = clone->to<IR::P4Parser>();
        auto newtype = new IR::Type_Parser(name, parser->type->annotations,
                                           new IR::TypeParameters(),
                                           parser->getApplyParameters());
        declarations->append(parser->parserLocals);
        result = new IR::P4Parser(name, newtype, new IR::ParameterList(),
                                  *declarations, parser->states);
    } else if (clone->is<IR::P4Control>()) {
        auto control = clone->to<IR::P4Control>();
        auto newtype = new IR::Type_Control(name, control->type->annotations,
                                            new IR::TypeParameters(),
                                            control->getApplyParameters());
        declarations->append(control->controlLocals);
        result = new IR::P4Control(name, newtype, new IR::ParameterList(),
                                   *declarations, control->body);

    } else {
        BUG("%1%: unexpected type", specialized);
    }
    LOG2("Created " << result);
    return result;
}

const IR::Argument* SpecializationMap::convertArgument(
    const IR::Argument* arg, SpecializationInfo* spec, const IR::Parameter* param) {
    if (arg->expression->is<IR::ConstructorCallExpression>()) {
        auto cce = arg->expression->to<IR::ConstructorCallExpression>();
        cstring nName = refMap->newName(param->name);
        IR::ID id(param->srcInfo, nName, param->name);
        auto decl = new IR::Declaration_Instance(
            param->srcInfo, id, cce->constructedType, cce->arguments);
        spec->declarations->push_back(decl);
        auto path = new IR::PathExpression(param->srcInfo, new IR::Path(param->srcInfo, id));
        return new IR::Argument(arg->srcInfo, arg->name, path);
    } else {
        return arg;
    }
}

void SpecializationMap::addSpecialization(
    const IR::ConstructorCallExpression* invocation, const IR::IContainer* cont,
    const IR::Node* insertion) {
    LOG2("Will specialize " << dbp(invocation) << " of " << dbp(cont->getNode())
         << " " << cont->getNode() << " inserted after" << insertion);

    auto spec = new SpecializationInfo(invocation, cont, insertion);
    auto declaration = cont->to<IR::IDeclaration>();
    CHECK_NULL(declaration);
    spec->name = refMap->newName(declaration->getName());
    auto cc = ConstructorCall::resolve(invocation, refMap, typeMap);
    auto ccc = cc->to<ContainerConstructorCall>();
    CHECK_NULL(ccc);
    spec->constructorArguments = new IR::Vector<IR::Argument>();
    for (auto ca : *invocation->arguments) {
        auto param = cc->substitution.findParameter(ca);
        CHECK_NULL(param);
        auto arg = convertArgument(ca, spec, param);
        spec->constructorArguments->push_back(arg);
    }
    spec->typeArguments = ccc->typeArguments;
    specializations.emplace(invocation, spec);
}

void SpecializationMap::addSpecialization(
    const IR::Declaration_Instance* invocation, const IR::IContainer* cont,
    const IR::Node* insertion) {
    LOG2("Will specialize " << dbp(invocation) << " of " << dbp(cont->getNode()) <<
         " inserted after" << insertion);

    auto spec = new SpecializationInfo(invocation, cont, insertion);
    auto declaration = cont->to<IR::IDeclaration>();
    CHECK_NULL(declaration);
    spec->name = refMap->newName(declaration->getName());
    const IR::Type_Name* type;
    const IR::Vector<IR::Type>* typeArgs;
    if (invocation->type->is<IR::Type_Specialized>()) {
        auto ts = invocation->type->to<IR::Type_Specialized>();
        type = ts->baseType;
        typeArgs = ts->arguments;
    } else {
        type = invocation->type->to<IR::Type_Name>();
        typeArgs = new IR::Vector<IR::Type>();
    }
    Instantiation* inst = Instantiation::resolve(invocation, refMap, typeMap);

    spec->typeArguments = typeArgs;
    CHECK_NULL(type);
    for (auto ca : *invocation->arguments) {
        auto param = inst->substitution.findParameter(ca);
        CHECK_NULL(param);
        auto arg = convertArgument(ca, spec, param);
        spec->constructorArguments->push_back(arg);
    }
    specializations.emplace(invocation, spec);
}

IR::Vector<IR::Node>*
SpecializationMap::getSpecializations(const IR::Node* insertionPoint) const {
    IR::Vector<IR::Node>* result = nullptr;
    for (auto s : specializations) {
        if (s.second->insertBefore == insertionPoint) {
            if (result == nullptr)
                result = new IR::Vector<IR::Node>();
            auto node = s.second->synthesize(refMap);
            LOG2("Will insert " << node << " before " << insertionPoint);
            result->push_back(node);
        }
    }
    return result;
}

namespace {
class IsConcreteType : public Inspector {
    const TypeMap* typeMap;
 public:
    bool hasTypeVariables = false;

    explicit IsConcreteType(const TypeMap* typeMap) : typeMap(typeMap) { CHECK_NULL(typeMap); }
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
        for (auto e : list->components)
            if (!isSimpleConstant(e))
                return false;
        return true;
    }
    auto ei = EnumInstance::resolve(expr, specMap->typeMap);
    if (ei != nullptr)
        return true;
    if (expr->is<IR::ConstructorCallExpression>()) {
        auto cce = expr->to<IR::ConstructorCallExpression>();
        for (auto e : *cce->arguments)
            if (!isSimpleConstant(e->expression))
                return false;
        return true;
    }
    return false;
}

bool FindSpecializations::noParameters(const IR::IContainer* container) {
    return container->getTypeParameters()->empty() &&
            container->getConstructorParameters()->empty();
}

const IR::Node* FindSpecializations::findInsertionPoint() const {
    // Find location where the specialization is to be inserted.
    // This can be before a Parser, Control or a toplevel instance declaration
    const IR::Node* insert = findContext<IR::P4Parser>();
    if (insert != nullptr)
        return insert;
    insert = findContext<IR::P4Control>();
    if (insert != nullptr)
        return insert;
    insert = findContext<IR::Declaration_Instance>();
    return insert;
}

void FindSpecializations::postorder(const IR::ConstructorCallExpression* expression) {
    if (expression->arguments->size() == 0 &&
        !expression->constructedType->is<IR::Type_Specialized>())
        return;  // nothing to specialize

    auto cc = ConstructorCall::resolve(expression, specMap->refMap, specMap->typeMap);
    if (!cc->is<ContainerConstructorCall>())
        return;
    for (auto arg : *expression->arguments) {
        if (!isSimpleConstant(arg->expression))
            return;
    }

    auto insert = findInsertionPoint();
    auto decl = cc->to<ContainerConstructorCall>()->container;
    if (decl->is<IR::Type_Package>())
        return;
    specMap->addSpecialization(expression, decl, insert);
}

void FindSpecializations::postorder(const IR::Declaration_Instance* decl) {
    if (decl->arguments->size() == 0 &&
        !decl->type->is<IR::Type_Specialized>())
        return;

    auto type = specMap->typeMap->getType(decl, true);
    if (type->is<IR::Type_SpecializedCanonical>()) {
        auto ts = type->to<IR::Type_SpecializedCanonical>();
        IsConcreteType ct(specMap->typeMap);
        if (!ct.isConcrete(ts))
            return;  // no point in specializing if arguments have type variables
        type = ts->baseType;
    }
    for (auto arg : *decl->arguments) {
        if (!isSimpleConstant(arg->expression))
            return;
    }

    const IR::Type_Name *contDecl;
    if (decl->type->is<IR::Type_Name>()) {
        contDecl = decl->type->to<IR::Type_Name>();
    } else {
        BUG_CHECK(decl->type->is<IR::Type_Specialized>(), "%1%: unexpected type", decl->type);
        contDecl = decl->type->to<IR::Type_Specialized>()->baseType;
    }
    auto cont = specMap->refMap->getDeclaration(contDecl->path, true);
    if (!cont->is<IR::P4Parser>() && !cont->is<IR::P4Control>())
        return;
    auto insert = findInsertionPoint();
    if (insert == nullptr)
        insert = decl;
    specMap->addSpecialization(decl, cont->to<IR::IContainer>(), insert);
}

const IR::Node* Specialize::instantiate(const IR::Node* node) {
    auto specs = specMap->getSpecializations(getOriginal());
    if (specs == nullptr)
        return node;
    LOG2(specs->size() << " instantiations before " << node);
    specs->push_back(node);
    return specs;
}

const IR::Node* Specialize::postorder(IR::ConstructorCallExpression* expression) {
    auto name = specMap->getName(getOriginal());
    if (name.isNullOrEmpty())
        return expression;
    auto typeRef = new IR::Type_Name(IR::ID(name, nullptr));
    auto result = new IR::ConstructorCallExpression(typeRef, new IR::Vector<IR::Argument>());
    LOG2("Replaced " << expression << " with " << result);
    return result;
}

const IR::Node* Specialize::postorder(IR::Declaration_Instance* decl) {
    // replace instance with invocation of new type
    const IR::Node* replacement = decl;
    auto name = specMap->getName(getOriginal());
    if (!name.isNullOrEmpty()) {
        auto typeRef = new IR::Type_Name(IR::ID(name, nullptr));
        replacement = new IR::Declaration_Instance(
            decl->srcInfo, decl->name, decl->annotations, typeRef,
            new IR::Vector<IR::Argument>(), decl->initializer);
        LOG2("Replaced " << decl << " with " << replacement);
    }

    return instantiate(replacement);
}

SpecializeAll::SpecializeAll(ReferenceMap* refMap, TypeMap* typeMap) : PassRepeated({}) {
    passes.emplace_back(new ConstantFolding(refMap, typeMap));
    passes.emplace_back(new TypeChecking(refMap, typeMap));
    passes.emplace_back(new FindSpecializations(&specMap));
    passes.emplace_back(new Specialize(&specMap));
    passes.emplace_back(new RemoveAllUnusedDeclarations(refMap));
    specMap.refMap = refMap;
    specMap.typeMap = typeMap;
    setName("SpecializeAll");
}

}  // namespace P4
