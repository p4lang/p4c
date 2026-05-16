// Copyright 2018 VMware, Inc.
// SPDX-FileCopyrightText: 2018 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "enumInstance.h"

namespace P4 {

// static
EnumInstance *EnumInstance::resolve(const IR::Expression *expression, const P4::TypeMap *typeMap) {
    CHECK_NULL(typeMap);
    if (!expression->is<IR::Member>()) return nullptr;
    auto member = expression->to<IR::Member>();
    if (!member->expr->is<IR::TypeNameExpression>()) return nullptr;
    auto type = typeMap->getType(expression, true);
    if (auto et = type->to<IR::Type_Enum>()) {
        return new SimpleEnumInstance(et, member->member, typeMap);
    } else if (auto set = type->to<IR::Type_SerEnum>()) {
        auto decl = set->getDeclByName(member->member);
        CHECK_NULL(decl);
        auto sem = decl->to<IR::SerEnumMember>();
        CHECK_NULL(sem);
        return new SerEnumInstance(set, member->member, sem->value, typeMap);
    }
    return nullptr;
}

}  // namespace P4
