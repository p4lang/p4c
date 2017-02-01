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

class KeyNameGenerator : public Inspector {
    std::map<const IR::Expression*, cstring> name;

 public:
    KeyNameGenerator() { setName("KeyNameGenerator"); }

    void error(const IR::Expression* expression) {
        ::error("%1%: Complex key expression requires a @name annotation", expression);
    }

    void postorder(const IR::Expression* expression) override { error(expression); }

    void postorder(const IR::PathExpression* expression) override {
        name.emplace(expression, expression->path->toString());
    }

    void postorder(const IR::Member* expression) override {
        if (cstring n = getName(expression->expr))
            name.emplace(expression, n + "." + expression->member);
    }

    void postorder(const IR::ArrayIndex* expression) override {
        cstring l = getName(expression->left);
        cstring r = getName(expression->right);
        if (!l || !r)
            return;
        name.emplace(expression, l + "[" + r + "]");
    }

    void postorder(const IR::Constant* expression) override {
        name.emplace(expression, expression->toString());
    }

    void postorder(const IR::Slice* expression) override {
        cstring e0 = getName(expression->e0);
        cstring e1 = getName(expression->e1);
        cstring e2 = getName(expression->e2);
        if (!e0 || !e1 || !e2)
            return;
        name.emplace(expression, e0 + "[" + e1 + ":" + e2 + "]");
    }

    void postorder(const IR::MethodCallExpression* expression) override {
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

    cstring getName(const IR::Expression* expression) {
        return ::get(name, expression);
    }
};

const IR::Node* TableKeyNames::postorder(IR::KeyElement* keyElement) {
    LOG3("Visiting " << getOriginal());
    if (keyElement->annotations->getSingle(IR::Annotation::nameAnnotation) != nullptr)
        // already present: no changes
        return keyElement;
    KeyNameGenerator kng;
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
