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

#include "ir.h"

namespace IR {

cstring Annotation::getName(const IR::Annotation* annotation) {
    BUG_CHECK(annotation->name == IR::Annotation::nameAnnotation,
              "%1%: Only works on name annotations", annotation);
    if (annotation->expr.size() != 1) {
        ::error("%1% should contain a string", annotation);
        return "";
    }
    auto str = annotation->expr[0]->to<IR::StringLiteral>();
    if (str == nullptr) {
        ::error("%1% should contain a string", annotation);
        return "";
    }
    return str->value;
}

cstring IDeclaration::externalName(cstring replace /* = cstring() */) const {
    if (!is<IAnnotated>())
        return getName().toString();

    auto anno = getAnnotation(IR::Annotation::nameAnnotation);
    if (anno != nullptr)
        return Annotation::getName(anno);
    if (replace)
        return replace;
    return getName().toString();
}

cstring IDeclaration::controlPlaneName(cstring replace /* = cstring() */) const {
    cstring name = externalName(replace);
    return name.startsWith(".") ? name.substr(1) : name;
}

}  // namespace IR
