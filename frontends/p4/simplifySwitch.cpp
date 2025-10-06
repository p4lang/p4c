/*
Copyright 2021 VMware, Inc.

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

#include "simplifySwitch.h"

#include "enumInstance.h"

namespace P4 {

bool DoSimplifySwitch::matches(const IR::Expression *left, const IR::Expression *right) const {
    // We know that left and right have matching types
    if (left->is<IR::DefaultExpression>()) return true;
    if (auto *rei = EnumInstance::resolve(right, typeMap)) {
        if (auto *lei = EnumInstance::resolve(left, typeMap)) return rei->equals(lei);
        if (auto *se = rei->to<SerEnumInstance>()) return matches(left, se->value);
    } else if (auto cr = right->to<IR::Constant>()) {
        if (auto cl = left->to<IR::Constant>()) return cr->value == cl->value;
        if (auto *lei = EnumInstance::resolve(left, typeMap))
            if (auto *se = lei->to<SerEnumInstance>()) return matches(se->value, right);
    } else if (auto br = right->to<IR::BoolLiteral>()) {
        if (auto bl = left->to<IR::BoolLiteral>()) return bl->value == br->value;
    }
    BUG("Unexpected comparison %1% with %2%", left, right);
    return false;
}

const IR::Node *DoSimplifySwitch::postorder(IR::SwitchStatement *stat) {
    if (!typeMap->isCompileTimeConstant(stat->expression)) return stat;
    if (stat->expression->is<IR::PathExpression>()) {
        // this must be a ref to a directionless arg in scope
        return stat;
    }
    bool foundMatch = false;
    for (auto ss : stat->cases) {
        if (matches(ss->label, stat->expression)) {
            foundMatch = true;
        }
        // In case of fallthrough, return next available statement body
        if (foundMatch && ss->statement) {
            return ss->statement;
        }
    }
    // If none of the labels matches none of the cases will be
    // executed.
    return new IR::EmptyStatement(stat->srcInfo);
}

}  // namespace P4
