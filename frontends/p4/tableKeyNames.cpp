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

#include "tableKeyNames.h"

namespace P4 {

void KeyNameGenerator::error(const IR::Expression* expression) {
    ::error("%1%: Complex key expression requires a @name annotation",
            expression);
}

void KeyNameGenerator::postorder(const IR::PathExpression* expression)
{ name.emplace(expression, expression->path->toString()); }

void KeyNameGenerator::postorder(const IR::Member* expression) {
    cstring fname = expression->member.name;
    if (typeMap != nullptr) {
        auto type = typeMap->getType(expression->expr, true);
        if (type->is<IR::Type_StructLike>()) {
            auto st = type->to<IR::Type_StructLike>();
            auto field = st->getField(expression->member);
            if (field != nullptr)
                fname = field->externalName();
        }
    }
    if (cstring n = getName(expression->expr))
        name.emplace(expression, n + "." + fname);
}

void KeyNameGenerator::postorder(const IR::ArrayIndex* expression) {
    cstring l = getName(expression->left);
    cstring r = getName(expression->right);
    if (!l || !r)
        return;
    name.emplace(expression, l + "[" + r + "]");
}

void KeyNameGenerator::postorder(const IR::Constant* expression)
{ name.emplace(expression, expression->toString()); }

void KeyNameGenerator::postorder(const IR::Slice* expression) {
    cstring e0 = getName(expression->e0);
    cstring e1 = getName(expression->e1);
    cstring e2 = getName(expression->e2);
    if (!e0 || !e1 || !e2)
        return;
    name.emplace(expression, e0 + "[" + e1 + ":" + e2 + "]");
}

void KeyNameGenerator::postorder(const IR::MethodCallExpression* expression) {
    cstring m = getName(expression->method);
    if (!m)
        return;
    if (!m.endsWith(IR::Type_Header::isValid) || expression->arguments->size() != 0) {
        // This is a heuristic
        error(expression);
        return;
    }
    name.emplace(expression, m + "()");
}

const IR::Node* DoTableKeyNames::postorder(IR::KeyElement* keyElement) {
    LOG3("Visiting " << getOriginal());
    if (keyElement->getAnnotation(IR::Annotation::nameAnnotation) != nullptr)
        // already present: no changes
        return keyElement;
    KeyNameGenerator kng(typeMap);;
    (void)keyElement->expression->apply(kng);
    cstring name = kng.getName(keyElement->expression);

    LOG3("Generated name " << name);
    if (!name)
        return keyElement;
    keyElement->annotations = keyElement->annotations->addAnnotation(
        IR::Annotation::nameAnnotation,
        new IR::StringLiteral(keyElement->expression->srcInfo, name));
    return keyElement;
}

}  // namespace P4
