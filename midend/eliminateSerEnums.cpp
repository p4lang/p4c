// Copyright 2018 VMware, Inc.
// SPDX-FileCopyrightText: 2018 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "eliminateSerEnums.h"

#include "frontends/p4/enumInstance.h"

namespace P4 {

const IR::Node *DoEliminateSerEnums::preorder(IR::Type_SerEnum *serEnum) {
    if (getParent<IR::P4Program>()) return nullptr;  // delete the declaration
    return serEnum->type;
}

const IR::Node *DoEliminateSerEnums::postorder(IR::Type_Name *type) {
    auto canontype = typeMap->getTypeType(getOriginal(), true);
    if (!canontype->is<IR::Type_SerEnum>()) return type;
    if (isInContext<IR::TypeNameExpression>())
        // This will be resolved by the caller.
        return type;
    auto enumType = canontype->to<IR::Type_SerEnum>();
    LOG2("Replacing " << type << " with " << enumType->type);
    return enumType->type;
}

/// process enum expression, e.g., X.a
const IR::Node *DoEliminateSerEnums::postorder(IR::Member *expression) {
    auto ei = EnumInstance::resolve(getOriginal<IR::Member>(), typeMap);
    if (!ei) return expression;
    if (auto sei = ei->to<SerEnumInstance>()) {
        LOG2("Replacing " << expression << " with " << sei->value);
        return sei->value;
    }
    return expression;
}

}  // namespace P4
