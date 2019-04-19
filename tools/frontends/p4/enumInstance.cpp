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

#include "enumInstance.h"

namespace P4 {

// static
EnumInstance* EnumInstance::resolve(const IR::Expression* expression, const P4::TypeMap* typeMap) {
    if (!expression->is<IR::Member>())
        return nullptr;
    auto member = expression->to<IR::Member>();
    if (!member->expr->is<IR::TypeNameExpression>())
        return nullptr;
    auto type = typeMap->getType(expression, true);
    if (auto et = type->to<IR::Type_Enum>()) {
        return new SimpleEnumInstance(et, member->member);
    } else if (auto set = type->to<IR::Type_SerEnum>()) {
        auto decl = set->getDeclByName(member->member);
        CHECK_NULL(decl);
        auto sem = decl->to<IR::SerEnumMember>();
        CHECK_NULL(sem);
        return new SerEnumInstance(set, member->member, sem->value);
    }
    return nullptr;
}

}  // namespace P4
