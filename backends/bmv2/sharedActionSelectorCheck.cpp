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

#include "sharedActionSelectorCheck.h"

namespace BMV2 {

const Input*
SharedActionSelectorCheck::get_selector_input(const IR::Declaration_Instance* selector) {
    auto it = selector_input_map.find(selector);
    if (it == selector_input_map.end()) return nullptr;  // selector never used
    return &it->second;
}

bool
SharedActionSelectorCheck::preorder(const IR::P4Table* table) {
    auto implementation = table->properties->getProperty("implementation");
    if (implementation == nullptr) return false;
    if (!implementation->value->is<IR::ExpressionValue>()) {
        ::error("%1%: expected expression for property", implementation);
        return false;
    }
    auto propv = implementation->value->to<IR::ExpressionValue>();
    if (!propv->expression->is<IR::PathExpression>()) return false;
    auto pathe = propv->expression->to<IR::PathExpression>();
    auto decl = refMap->getDeclaration(pathe->path, true);
    if (!decl->is<IR::Declaration_Instance>()) {
        ::error("%1%: expected a reference to an instance", pathe);
        return false;
    }
    auto dcltype = typeMap->getType(pathe, true);
    if (!dcltype->is<IR::Type_Extern>()) {
        ::error("%1%: unexpected type for implementation", dcltype);
        return false;
    }
    auto type_extern_name = dcltype->to<IR::Type_Extern>()->name;
    if (type_extern_name != BMV2::TableImplementation::actionSelectorName) return false;

    auto key = table->getKey();
    Input input;
    for (auto ke : key->keyElements) {
        auto mt = refMap->getDeclaration(ke->matchType->path, true)->to<IR::Declaration_ID>();
        BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);
        if (mt->name.name != BMV2::MatchImplementation::selectorMatchTypeName) continue;
        input.push_back(ke->expression);
    }
    auto decl_instance = decl->to<IR::Declaration_Instance>();
    auto it = selector_input_map.find(decl_instance);
    if (it == selector_input_map.end()) {
        selector_input_map[decl_instance] = input;
        return false;
    }
    // returns true if inputs are the same, false otherwise
    auto cmp_inputs = [](const Input &i1, const Input &i2) {
        for (auto e1 : i1) {
            auto cmp_e = [e1](const IR::Expression *e2) {
                return checkSameKeyExpr(e1, e2);
            };
            if (std::find_if(i2.begin(), i2.end(), cmp_e) == i2.end()) return false;
        }
        return true;
    };

    if (!cmp_inputs(it->second, input)) {
        ::error(
                "Action selector '%1%' is used by multiple tables with different selector inputs",
                decl);
    }

    return false;
}

}  // namespace BMV2
