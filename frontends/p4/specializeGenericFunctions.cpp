/*
Copyright 2020 VMware, Inc.

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

#include "specializeGenericFunctions.h"

namespace P4 {

namespace {

class TypeSubstitutionVisitor : public TypeVariableSubstitutionVisitor {
    TypeMap* typeMap;

 public:
    TypeSubstitutionVisitor(TypeMap* typeMap, TypeVariableSubstitution* ts) :
            TypeVariableSubstitutionVisitor(ts), typeMap(typeMap) {
        CHECK_NULL(typeMap); setName("TypeSubstitutionVisitor"); }
    const IR::Node* postorder(IR::PathExpression* path) override {
        // We want fresh nodes for variables, etc.
        return new IR::PathExpression(path->path->clone()); }
    const IR::Node* postorder(IR::Type_Name* type) override {
        auto actual = typeMap->getTypeType(getOriginal<IR::Type_Name>(), true);
        if (auto tv = actual->to<IR::ITypeVar>()) {
            LOG3("Replacing " << tv);
            return replacement(tv, type);
        }
        return type;
    }
};

}  // namespace

bool FindFunctionSpecializations::preorder(const IR::MethodCallExpression* mce) {
    if (!mce->typeArguments->size())
        return false;
    MethodInstance *mi = MethodInstance::resolve(mce, specMap->refMap, specMap->typeMap);
    if (auto func = mi->to<FunctionCall>()) {
        LOG3("Will specialize " << mce);
        specMap->add(mce, func->function);
    }
    return false;
}

const IR::Node* SpecializeFunctions::postorder(IR::Function* function) {
    auto result = new IR::Vector<IR::Node>();
    for (auto it : specMap->map) {
        if (it.second->specialized == getOriginal()) {
            auto methodCall = it.first;
            TypeVariableSubstitution ts;
            ts.setBindings(function, function->type->typeParameters, methodCall->typeArguments);
            TypeSubstitutionVisitor tsv(specMap->typeMap, &ts);
            LOG3("Substitution " << ts);
            auto specialized = function->apply(tsv)->to<IR::Function>();
            auto renamed = new IR::Function(
                specialized->srcInfo,
                it.second->name,
                specialized->type,
                specialized->body);
            result->push_back(renamed);
            LOG3("Specializing " << function << " as " << renamed);
        }
    }
    if (!result->size())
        return function;
    result->push_back(function);
    return result;
}

const IR::Node* SpecializeFunctions::postorder(IR::MethodCallExpression* mce) {
    if (auto fs = specMap->get(getOriginal<IR::MethodCallExpression>())) {
        LOG3("Substituting call to " << mce->method << " with " << fs->name);
        mce->method = new IR::PathExpression(new IR::Path(mce->srcInfo, fs->name));
        mce->typeArguments = new IR::Vector<IR::Type>();
    }
    return mce;
}

}  // namespace P4
