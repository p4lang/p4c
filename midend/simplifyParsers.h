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

#ifndef _MIDEND_SIMPLIFYPARSERS_H_
#define _MIDEND_SIMPLIFYPARSERS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/callGraph.h"

namespace P4 {

// Invokes two optimizations:
// - remove unreachable parser states
// - collapse simple chains of states
class DoSimplifyParsers : public Transform {
    ReferenceMap *refMap;
 public:
    DoSimplifyParsers(ReferenceMap *refMap) : refMap(refMap) {
        CHECK_NULL(refMap);
        setName("DoSimplifyParsers");
    }

    const IR::Node* preorder(IR::P4Parser* parser) override;
    const IR::Node* preorder(IR::P4Control* control) override
    { prune(); return control; }
};

class SimplifyParsers : public PassManager {
 public:
    SimplifyParsers(ReferenceMap* refMap, bool isv1) {
        passes.push_back(new ResolveReferences(refMap, isv1));
        passes.push_back(new DoSimplifyParsers(refMap));
        setName("SimplifyParsers");
    }
};

}  // namespace P4

#endif /* _MIDEND_SIMPLIFYPARSERS_H_ */
