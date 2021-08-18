/*
Copyright 2021 VMware, Inc.

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

#ifndef _FRONTENDS_P4_REMOVEZEROWIDTH_H_
#define _FRONTENDS_P4_REMOVEZEROWIDTH_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
 * Remove zero-width fields from struct-like types
 */
class RemoveZeroWidthFields : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;
 public:
    RemoveZeroWidthFields(ReferenceMap* refMap, TypeMap* typeMap):
            refMap(refMap), typeMap(typeMap) {
        setName("RemoveZeroWidthFields");
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
    }

    const IR::Node* preorder(IR::StructField* field) override;
    const IR::Node* preorder(IR::NamedExpression* expression) override;
};

class RemoveZeroWidth : public PassManager {
 public:
    RemoveZeroWidth(ReferenceMap* refMap, TypeMap* typeMap) {
        setName("RemoveZeroWidth");
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new RemoveZeroWidthFields(refMap, typeMap));
        passes.push_back(new ClearTypeMap(typeMap));
    }
};

}  // namespace P4



#endif /* _FRONTENDS_P4_REMOVEZEROWIDTH_H_ */
