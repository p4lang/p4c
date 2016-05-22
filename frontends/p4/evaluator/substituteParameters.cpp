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

#include "substituteParameters.h"

namespace P4 {

const IR::Node* SubstituteParameters::postorder(IR::PathExpression* expr) {
    auto decl = refMap->getDeclaration(expr->path, true);
    auto param = decl->to<IR::Parameter>();
    if (param != nullptr && subst->contains(param)) {
        auto value = subst->lookup(param);
        LOG1("Replaced " << expr << " with " << value);
        return value;
    }

    // Path expressions always need to be cloned.
    IR::ID newid = expr->path->name;
    auto path = new IR::Path(expr->path->prefix, newid);
    auto result = new IR::PathExpression(path);
    LOG1("Cloned " << expr << " into " << result);
    refMap->setDeclaration(path, decl);
    return result;
}

const IR::Node* SubstituteParameters::postorder(IR::Type_Name* type) {
    auto decl = refMap->getDeclaration(type->path, true);
    auto var = decl->to<IR::Type_Var>();
    if (var != nullptr && bindings->containsKey(var)) {
        auto repl = bindings->lookup(var);
        LOG1("Replaced " << type << " with " << repl);
        return repl;
    }

    IR::ID newid = type->path->name;
    auto path = new IR::Path(type->path->prefix, newid);
    refMap->setDeclaration(path, decl);
    auto result = new IR::Type_Name(type->srcInfo, path);
    LOG1("Cloned " << type << " into " << result);
    return result;
}

}  // namespace P4
