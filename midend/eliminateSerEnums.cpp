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

#include "frontends/p4/enumInstance.h"

#include "eliminateSerEnums.h"

namespace P4 {

const IR::Node* DoEliminateSerEnums::preorder(IR::Type_SerEnum*) {
    return nullptr;  // delete the declaration
}

const IR::Node* DoEliminateSerEnums::postorder(IR::Type_Name* type) {
    auto canontype = typeMap->getTypeType(getOriginal(), true);
    if (!canontype->is<IR::Type_SerEnum>())
        return type;
    if (findContext<IR::TypeNameExpression>() != nullptr)
        // This will be resolved by the caller.
        return type;
    auto enumType = canontype->to<IR::Type_SerEnum>();
    return enumType->type;
}

/// process enum expression, e.g., X.a
const IR::Node* DoEliminateSerEnums::postorder(IR::Member* expression) {
    auto ei = EnumInstance::resolve(getOriginal<IR::Member>(), typeMap);
    if (!ei) return expression;
    if (auto sei = ei->to<SerEnumInstance>())
        return sei->value;
    return expression;
}

}  // namespace P4
