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

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "deprecated.h"

namespace P4 {

void CheckDeprecated::warnIfDeprecated(
    const IR::IAnnotated* annotated, const IR::Node* errorNode) {
    if (annotated == nullptr)
        return;
    auto anno =
        annotated->getAnnotations()->getSingle(IR::Annotation::deprecatedAnnotation);
    if (anno == nullptr)
        return;

    cstring message = "";
    for (auto a : anno->expr) {
        if (auto str = a->to<IR::StringLiteral>())
            message += str->value;
    }
    ::warning(ErrorType::WARN_DEPRECATED, "%1%: Using deprecated feature %2%. %3%",
              errorNode, annotated->getNode(), message);
}

bool CheckDeprecated::preorder(const IR::PathExpression* expression) {
    auto decl = refMap->getDeclaration(expression->path);
    CHECK_NULL(decl);
    warnIfDeprecated(decl->to<IR::IAnnotated>(), expression);
    return false;
}

bool CheckDeprecated::preorder(const IR::Type_Name* name) {
    auto decl = refMap->getDeclaration(name->path);
    CHECK_NULL(decl);
    warnIfDeprecated(decl->to<IR::IAnnotated>(), name);
    return false;
}

}  // namespace P4
