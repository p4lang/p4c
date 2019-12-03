/*
Copyright 2018 VMware, Inc.

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

#include "defaultArguments.h"
#include "frontends/p4/methodInstance.h"

namespace P4 {

class SubstituteTypeArguments : public Transform {
    TypeMap *typeMap;

 public:
    SubstituteTypeArguments(TypeMap* typeMap): typeMap(typeMap) {
        CHECK_NULL(typeMap); setName("SubstituteTypeArguments");
    }

    const IR::Node* preorder(IR::Type_Name* type) override {
        auto t = typeMap->getTypeType(getOriginal(), true);
        if (auto tv = t->to<IR::Type_Var>()) {
            auto subst = typeMap->getType(tv);
            if (subst != nullptr)
                return subst->getP4Type();
        }
        return type;
    }
};

/// Scans a parameter substitution and returns the new arguments to use if some
/// parameters have default values.
/// Returns nullptr if no arguments need to be changed.
static const IR::Vector<IR::Argument>* fillDefaults(
    TypeMap* typeMap,
    const ParameterSubstitution* subst) {
    auto args = new IR::Vector<IR::Argument>();
    bool changed = false;
    for (auto param : *subst->getParametersInOrder()) {
        auto arg = subst->lookup(param);
        if (arg != nullptr) {
            // there is a matching argument
            if (arg->name.name.isNullOrEmpty())
                // if it has no name add the name
                arg = new IR::Argument(arg->srcInfo, param->name, arg->expression);
        } else if (param->defaultValue != nullptr) {
            // Parameter with default value: add a corresponding argument
            const IR::Expression* value = param->defaultValue;
            SubstituteTypeArguments sta(typeMap);
            value = param->defaultValue->apply(sta);
            if (value != param->defaultValue) {
                arg = new IR::Argument(param->srcInfo, param->name, value);
                changed = true;
            }
        }
        args->push_back(arg);
    }

    if (changed)
        return args;
    return nullptr;
}

const IR::Node* DoDefaultArguments::postorder(IR::MethodCallExpression* mce) {
    auto mi = MethodInstance::resolve(mce, refMap, typeMap);
    auto args = fillDefaults(typeMap, &mi->substitution);
    if (args != nullptr)
        mce->arguments = args;
    return mce;
}

const IR::Node* DoDefaultArguments::postorder(IR::ConstructorCallExpression* cce) {
    auto cc = ConstructorCall::resolve(cce, refMap, typeMap);
    auto args = fillDefaults(typeMap, &cc->substitution);
    if (args != nullptr)
        cce->arguments = args;
    return cce;
}

const IR::Node* DoDefaultArguments::postorder(IR::Declaration_Instance* inst) {
    auto ii = Instantiation::resolve(inst, refMap, typeMap);
    auto args = fillDefaults(typeMap, &ii->substitution);
    if (args != nullptr)
        inst->arguments = args;
    return inst;
}

}  // namespace P4
