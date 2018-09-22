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

#include "orderArguments.h"
#include "frontends/p4/methodInstance.h"

namespace P4 {

static IR::Vector<IR::Argument>* reorder(const ParameterSubstitution &substitution) {
    auto reordered = new IR::Vector<IR::Argument>();

    bool foundOptional = false;
    for (auto p : *substitution.getParametersInOrder()) {
        auto arg = substitution.lookup(p);
        if (arg == nullptr) {
            // This argument may be missing either because it is
            // optional or because it is an argument for an action
            // which has some control-plane arguments.
            foundOptional = true;
        } else {
            BUG_CHECK(!foundOptional, "Not all optional parameters are at the end");
            reordered->push_back(arg);
        }
    }
    return reordered;
}

const IR::Node* DoOrderArguments::postorder(IR::MethodCallExpression* expression) {
    auto mi = MethodInstance::resolve(expression, refMap, typeMap);
    expression->arguments = reorder(mi->substitution);
    return expression;
}

const IR::Node* DoOrderArguments::postorder(IR::ConstructorCallExpression* expression) {
    ConstructorCall* ccd = ConstructorCall::resolve(expression, refMap, typeMap);
    expression->arguments = reorder(ccd->substitution);
    return expression;
}

const IR::Node* DoOrderArguments::postorder(IR::Declaration_Instance* instance) {
    auto inst = Instantiation::resolve(instance, refMap, typeMap);
    instance->arguments = reorder(inst->substitution);
    return instance;
}


}  // namespace P4
