/*
Copyright 2023 Mihai Budiu

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

#ifndef FRONTENDS_P4_ENTRYPRIORITIES_H_
#define FRONTENDS_P4_ENTRYPRIORITIES_H_

#include "coreLibrary.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"

namespace P4 {

/// Assigns priorities to table entries if they are not 'const'
class DoEntryPriorities : public Transform {
    ReferenceMap *refMap;
    P4::P4CoreLibrary &corelib;

    bool requiresPriority(const IR::KeyElement *ke) const;

 public:
    explicit DoEntryPriorities(ReferenceMap *refMap)
        : refMap(refMap), corelib(P4::P4CoreLibrary::instance()) {
        setName("DoEntryPriorities");
        CHECK_NULL(refMap);
    }
    const IR::Node *preorder(IR::EntriesList *entries) override;
};

class EntryPriorities : public PassManager {
 public:
    explicit EntryPriorities(ReferenceMap *refMap) {
        setName("EntryPriorities");
        passes.emplace_back(new ResolveReferences(refMap));
        passes.emplace_back(new DoEntryPriorities(refMap));
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_ENTRYPRIORITIES_H_ */
