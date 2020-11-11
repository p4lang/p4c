/*
Copyright 2017 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http:// www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _FRONTENDS_P4_REMOVEDUPLICATEDSELECTLABELS_H_
#define _FRONTENDS_P4_REMOVEDUPLICATEDSELECTLABELS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
   This class contains postorder method that eliminate duplicated select labels.
*/

class DoRemoveDuplicatedSelectLabels : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;

 public:
    explicit DoRemoveDuplicatedSelectLabels(ReferenceMap* refMap, TypeMap* typeMap) :
                refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        setName("DoRemoveDuplicatedSelectLabels");
    }
    // Handler for duplicated labels
    const IR::Node* postorder(IR::SelectExpression* expression) override;
};

class RemoveDuplicatedSelectLabels : public PassManager {
 public:
    explicit RemoveDuplicatedSelectLabels(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new DoRemoveDuplicatedSelectLabels(refMap, typeMap));
        passes.push_back(new ResolveReferences(refMap));
        passes.push_back(new TypeInference(refMap, typeMap));
        setName("RemoveDuplicatedSelectLabels");
    }
};

}  // namespace P4

#endif  /* _FRONTENDS_P4_REMOVEDUPLICATEDSELECTLABELS_H_ */
