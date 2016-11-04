/*
Copyright 2016 VMware, Inc.

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

#ifndef _BACKENDS_BMV2_COPYSTRUCTURES_H_
#define _BACKENDS_BMV2_COPYSTRUCTURES_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace BMV2 {

// Convert assignments between structures to assignments between fields
class DoCopyStructures : public Transform {
    P4::TypeMap* typeMap;
 public:
    explicit DoCopyStructures(P4::TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); setName("DoCopyStructures"); }
    const IR::Node* postorder(IR::AssignmentStatement* statement) override;
};

class CopyStructures : public PassRepeated {
 public:
    CopyStructures(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) :
            PassManager({}) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("CopyStructures");
        passes.emplace_back(new P4::TypeChecking(refMap, typeMap));
        passes.emplace_back(new DoCopyStructures(typeMap));
    }
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_COPYSTRUCTURES_H__ */
