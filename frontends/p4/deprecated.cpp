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

#include "deprecated.h"

namespace p4c::P4 {

void Deprecated::warnIfDeprecated(const IR::IAnnotated *annotated, const IR::Node *errorNode) {
    if (annotated == nullptr) return;
    auto anno = annotated->getAnnotations()->getSingle(IR::Annotation::deprecatedAnnotation);
    if (anno == nullptr) return;

    std::string message;
    for (const auto *a : anno->expr) {
        if (const auto *str = a->to<IR::StringLiteral>()) message += str->value;
    }
    ::warning(ErrorType::WARN_DEPRECATED, "%1%: Using deprecated feature %2%. %3%", errorNode,
              annotated->getNode(), message);
}

bool Deprecated::preorder(const IR::PathExpression *expression) {
    auto decl = getDeclaration(expression->path, true);
    warnIfDeprecated(decl->to<IR::IAnnotated>(), expression);
    return false;
}

bool Deprecated::preorder(const IR::Type_Name *name) {
    auto decl = getDeclaration(name->path, true);
    warnIfDeprecated(decl->to<IR::IAnnotated>(), name);
    return false;
}

}  // namespace p4c::P4
