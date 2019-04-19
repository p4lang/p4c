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

void KeyNameGenerator::postorder(const IR::Expression* expression) {
    error(expression);
}

void KeyNameGenerator::postorder(const IR::PathExpression* expression)
{ name.emplace(expression, expression->path->toString()); }

namespace {

/// @return a canonicalized string representation of the given Member
/// expression's right-hand side, suitable for use as part of a key name.
cstring keyComponentNameForMember(const IR::Member* expression,
                                  const P4::TypeMap* typeMap) {
    cstring fname = expression->member.name;

    // Without type information, we can't do any deeper analysis.
    if (!typeMap) return fname;
    auto* type = typeMap->getType(expression->expr, true);

    // Use `$valid$` to represent `isValid()` calls on headers and header
    // unions; this is what P4Runtime expects.
    // XXX(seth): Should we do this for header unions? It's not symmetric with
    // SynthesizeValidField, which leaves `isValid()` as-is for header unions,
    // but that's a BMV2-specific thing.
    if (type->is<IR::Type_Header>() || type->is<IR::Type_HeaderUnion>())
        if (expression->member == "isValid")
            return "$valid$";

    // If this Member represents a field which has an @name annotation, use it.
    if (type->is<IR::Type_StructLike>()) {
        auto* st = type->to<IR::Type_StructLike>();
        auto* field = st->getField(expression->member);
        if (field != nullptr)
            return field->externalName();
    }

    return fname;
}

}  // namespace

void KeyNameGenerator::postorder(const IR::Member* expression) {
    cstring fname = keyComponentNameForMember(expression, typeMap);

    // If the member name begins with `.`, it's a global name, and we can
    // discard the left-hand side of the Member expression. (We'll also strip
    // off the `.` so it doesn't appear in the key name.)
    if (fname.startsWith(".")) {
        name.emplace(expression, fname.substr(1));
        return;
    }

    // We can generate a name for the overall Member expression only if we were
    // able to generate a name for its left-hand side.
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

void KeyNameGenerator::postorder(const IR::BAnd* expression) {
    cstring left = getName(expression->left);
    if (!left) return;
    cstring right = getName(expression->right);
    if (!right) return;
    name.emplace(expression, left + " & " + right);
}

void KeyNameGenerator::postorder(const IR::MethodCallExpression* expression) {
    cstring m = getName(expression->method);
    if (!m)
        return;

    // This is a heuristic.
    if (m.endsWith("$valid$") && expression->arguments->size() == 0) {
        name.emplace(expression, m);
        return;
    }

    error(expression);
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
