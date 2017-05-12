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

#ifndef _FRONTENDS_P4_SIMPLIFYPARSERS_H_
#define _FRONTENDS_P4_SIMPLIFYPARSERS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/parserCallGraph.h"

namespace P4 {

/** @brief Remove unreachable parser states, and collapse simple chains of
 * states.
 *
 * Does not remove the "accept" state, even if it is not reachable.  A
 * transition between states ```s1``` and ```s2``` is part of a "simple" chain if:
 *  - there are no other outgoing edges from ```s1```,
 *  - there are no other incoming edges to ```s2```,
 *  - and ```s2``` does not have annotations.
 */
class DoSimplifyParsers : public Transform {
    ReferenceMap *refMap;
 public:
    explicit DoSimplifyParsers(ReferenceMap *refMap) : refMap(refMap) {
        CHECK_NULL(refMap);
        setName("DoSimplifyParsers");
    }

    const IR::Node* preorder(IR::P4Parser* parser) override;
    const IR::Node* preorder(IR::P4Control* control) override
    { prune(); return control; }
};

class SimplifyParsers : public PassManager {
 public:
    explicit SimplifyParsers(ReferenceMap* refMap) {
        passes.push_back(new ResolveReferences(refMap));
        passes.push_back(new DoSimplifyParsers(refMap));
        setName("SimplifyParsers");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_SIMPLIFYPARSERS_H_ */
