/*
Copyright 2019 RT-RK Computer Based Systems.

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

#include "fillEnumMap.h"
#include "frontends/p4/enumInstance.h"

namespace P4 {

const IR::Node* FillEnumMap::preorder(IR::Type_Enum* type) {
    if (strstr(type->srcInfo.filename, "v1model") == nullptr) {
        unsigned long long count = type->members.size();
        unsigned long long width = policy->enumSize(count);
        auto r = new EnumRepresentation(type->srcInfo, width);
        auto canontype = typeMap->getTypeType(getOriginal(), true);
        BUG_CHECK(canontype->is<IR::Type_Enum>(),
                  "canon type of enum %s is non enum %s?", type, canontype);
        repr.emplace(canontype->to<IR::Type_Enum>(), r);
        for (auto d : type->members)
            r->add(d->name.name);
    }
    return type;
}

}  // namespace P4
