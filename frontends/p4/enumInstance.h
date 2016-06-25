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

#ifndef _FRONTENDS_P4_ENUMINSTANCE_H_
#define _FRONTENDS_P4_ENUMINSTANCE_H_

#include "ir/ir.h"
#include "frontends/p4/typeMap.h"

namespace P4 {

// helps resolving references to compile-time enum fields, e.g., X.a
class EnumInstance {
    EnumInstance(const IR::Type_Enum* type, const IR::ID name) :
            type(type), name(name) {}
 public:
    const IR::Type_Enum* type;
    const IR::ID         name;

    // Returns nullptr if the expression is not a compile-time constant
    // referring to an enum
    static EnumInstance* resolve(const IR::Expression* expression, const P4::TypeMap* typeMap) {
        if (!expression->is<IR::Member>())
            return nullptr;
        auto member = expression->to<IR::Member>();
        if (!member->expr->is<IR::TypeNameExpression>())
            return nullptr;
        auto type = typeMap->getType(expression, true);
        if (!type->is<IR::Type_Enum>())
            return nullptr;
        return new EnumInstance(type->to<IR::Type_Enum>(), member->member);
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_ENUMINSTANCE_H_ */
