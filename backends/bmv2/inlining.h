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

#ifndef _BACKENDS_BMV2_INLINING_H_
#define _BACKENDS_BMV2_INLINING_H_

#include "ir/ir.h"
#include "midend/inlining.h"
#include "midend/actionsInlining.h"

namespace BMV2 {

// An inliner that only works for programs imported from P4 v1.0
class SimpleControlsInliner : public P4::AbstractInliner {
    P4::ReferenceMap* refMap;
    P4::InlineSummary::PerCaller* workToDo;
 public:
    explicit SimpleControlsInliner(P4::ReferenceMap* refMap) : refMap(refMap), workToDo(nullptr) {}
    const IR::Node* preorder(IR::MethodCallStatement* statement) override;
    const IR::Node* preorder(IR::P4Control* caller) override;
    Visitor::profile_t init_apply(const IR::Node* node) override;
};

class SimpleActionsInliner : public P4::AbstractActionInliner {
    P4::ReferenceMap* refMap;
    std::map<const IR::MethodCallStatement*, const IR::P4Action*>* replMap;
 public:
    explicit SimpleActionsInliner(P4::ReferenceMap* refMap) : refMap(refMap), replMap(nullptr) {}
    Visitor::profile_t init_apply(const IR::Node* node) override;
    const IR::Node* preorder(IR::P4Parser* cont) override
    { prune(); return cont; }  // skip
    const IR::Node* preorder(IR::P4Action* action) override;
    const IR::Node* postorder(IR::P4Action* action) override;
    const IR::Node* preorder(IR::MethodCallStatement* statement) override;
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_INLINING_H_ */
