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

#include "checkConstants.h"
#include "methodInstance.h"

namespace P4 {

void DoCheckConstants::postorder(const IR::MethodCallExpression* expression) {
    auto mi = MethodInstance::resolve(expression, refMap, typeMap);
    if (auto bi = mi->to<BuiltInMethod>()) {
        if (bi->name == IR::Type_Stack::push_front ||
            bi->name == IR::Type_Stack::pop_front) {
            BUG_CHECK(expression->arguments->size() == 1,
                      "Expected 1 argument for %1%", expression);
            auto arg0 = expression->arguments->at(0)->expression;
            if (!arg0->is<IR::Constant>())
                ::error(ErrorType::ERR_INVALID, "argument must be a constant", arg0);
        }
    }
}

void DoCheckConstants::postorder(const IR::KeyElement* key) {
    if (key->expression->is<IR::Literal>())
        ::warning(ErrorType::WARN_MISMATCH, "%1%: Constant key field", key->expression);
}

void DoCheckConstants::postorder(const IR::P4Table* table) {
    // This will print an error if the property exists and is not an integer
    (void)table->getSizeProperty();
}

}  // namespace P4
