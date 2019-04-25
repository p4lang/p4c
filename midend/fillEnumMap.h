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

#ifndef _MIDEND_FILLENUMMAP_H_
#define _MIDEND_FILLENUMMAP_H_

#include "convertEnums.h"

namespace P4 {

class FillEnumMap : public Transform {
 public:
    std::map<const IR::Type_Enum*, EnumRepresentation*> repr;
    ChooseEnumRepresentation* policy;
    TypeMap* typeMap;
    FillEnumMap(ChooseEnumRepresentation* policy, TypeMap* typeMap)
            : policy(policy), typeMap(typeMap)
    { CHECK_NULL(policy); CHECK_NULL(typeMap); setName("FillEnumMap"); }
    const IR::Node* preorder(IR::Type_Enum* type) override;
};

}  // namespace P4

#endif /* _MIDEND_FILLENUMMAP_H_ */
