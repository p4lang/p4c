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

#include <string>
#include <vector>

#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parameterSubstitution.h"
#include "frontends/p4/typeChecking/typeSubstitution.h"
#include "frontends/p4/typeChecking/typeSubstitutionVisitor.h"
#include "ir/id.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/enumerator.h"

namespace P4 {

namespace {
class TypeNameSubstitutionVisitor : public TypeVariableSubstitutionVisitor {
    const TypeMap *typeMap;

 public:
    explicit TypeNameSubstitutionVisitor(const TypeVariableSubstitution *bindings,
                                         const TypeMap *typeMap)
        : TypeVariableSubstitutionVisitor(bindings, true), typeMap(typeMap) {
        CHECK_NULL(typeMap);
        setName("TypeNameSubstitution");
        visitDagOnce = false;
    }

    const IR::Node *preorder(IR::Type_Name *tn) override {
        auto t = typeMap->getTypeType(getOriginal<IR::Type>(), true);
        if (auto tv = t->to<IR::ITypeVar>())
            return replacement(tv, tn)->to<IR::Type>()->getP4Type();
        return tn;
    }
    // When cloning the value of an argument we want to make a fresh
    // copy for each new InfInt type, and not share the same one.
    const IR::Node *postorder(IR::Type_InfInt *) override { return new IR::Type_InfInt(); }
};
}  // namespace

/// Scans a parameter substitution and returns the new arguments to use if some
/// parameters have default values.
/// Returns nullptr if no arguments need to be changed.
static const IR::Vector<IR::Argument> *fillDefaults(const TypeMap *typeMap,
                                                    const ParameterSubstitution *subst,
                                                    const TypeVariableSubstitution *tsv) {
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
            const IR::Expression *value = param->defaultValue;
            TypeNameSubstitutionVisitor tsvv(tsv, typeMap);
            value = param->defaultValue->apply(tsvv);
            arg = new IR::Argument(param->srcInfo, param->name, value);
            changed = true;
        }
        args->push_back(arg);
    }

    if (changed) return args;
    return nullptr;
}

const IR::Node *DoDefaultArguments::postorder(IR::MethodCallExpression *mce) {
    auto mi = MethodInstance::resolve(mce, refMap, typeMap);
    auto args = fillDefaults(typeMap, &mi->substitution, &mi->typeSubstitution);
    if (args != nullptr) mce->arguments = args;
    return mce;
}

const IR::Node *DoDefaultArguments::postorder(IR::ConstructorCallExpression *cce) {
    auto cc = ConstructorCall::resolve(cce, refMap, typeMap);
    auto args = fillDefaults(typeMap, &cc->substitution, &cc->typeSubstitution);
    if (args != nullptr) cce->arguments = args;
    return cce;
}

const IR::Node *DoDefaultArguments::postorder(IR::Declaration_Instance *inst) {
    auto ii = Instantiation::resolve(inst, refMap, typeMap);
    auto args = fillDefaults(typeMap, &ii->substitution, &ii->typeSubstitution);
    if (args != nullptr) inst->arguments = args;
    return inst;
}

}  // namespace P4
