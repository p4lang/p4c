/*
Copyright 2017 VMware, Inc.

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

#include "dontcareArgs.h"

namespace P4 {

const IR::Node* DontcareArgs::postorder(IR::MethodCallExpression* expression) {
    bool changes = false;
    auto vec = new IR::Vector<IR::Argument>();

    auto mi = MethodInstance::resolve(expression, refMap, typeMap);
    for (auto p : *mi->substitution.getParametersInArgumentOrder()) {
        auto a = mi->substitution.lookup(p);
        if (a->expression->is<IR::DefaultExpression>()) {
            cstring name = refMap->newName("arg");
            auto ptype = p->type;
            if (ptype->is<IR::Type_Dontcare>()) {
                ::error("Could not infer type for %1%", a);
                return expression;
            }
            auto decl = new IR::Declaration_Variable(IR::ID(name), ptype, nullptr);
            toAdd.push_back(decl);
            changes = true;
            vec->push_back(new IR::Argument(
                a->srcInfo, a->name, new IR::PathExpression(IR::ID(name))));
        } else {
            vec->push_back(a);
        }
    }
    if (changes)
        expression->arguments = vec;
    return expression;
}

}  // namespace P4
