/*
Copyright 2017 VMware, Inc.

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

#ifndef _MIDEND_REMOVEEXITS_H_
#define _MIDEND_REMOVEEXITS_H_

#include "frontends/p4/removeReturns.h"

namespace P4 {

/**
This visitor removes "exit" calls.  It is significantly more involved than return removal,
since an exit in an action causes the calling control to terminate.
This pass assumes that each statement in a control block can
exit only once -- so it should be run after a pass that enforces this,
e.g., SideEffectOrdering.
(E.g., it does not handle:
if (t1.apply().hit && t2.apply().hit) { ... }
It also assumes that there are no global actions and that action calls have been inlined.
*/
class DoRemoveExits : public DoRemoveReturns {
    TypeMap* typeMap;
    // In this class "Return" (inherited from RemoveReturns) should be read as "Exit"
    std::set<const IR::Node*> callsExit;  // actions, tables
    void callExit(const IR::Node* node);
 public:
    DoRemoveExits(ReferenceMap* refMap, TypeMap* typeMap) :
            DoRemoveReturns(refMap, "hasExited"), typeMap(typeMap)
    { visitDagOnce = false; CHECK_NULL(typeMap); setName("DoRemoveExits"); }

    const IR::Node* preorder(IR::ExitStatement* action) override;
    const IR::Node* preorder(IR::P4Table* table) override;

    const IR::Node* preorder(IR::BlockStatement* statement) override;
    const IR::Node* preorder(IR::IfStatement* statement) override;
    const IR::Node* preorder(IR::SwitchStatement* statement) override;
    const IR::Node* preorder(IR::AssignmentStatement* statement) override;
    const IR::Node* preorder(IR::MethodCallStatement* statement) override;

    const IR::Node* preorder(IR::P4Action* action) override;
    const IR::Node* preorder(IR::P4Control* control) override;
};

class RemoveExits : public PassManager {
 public:
    RemoveExits(ReferenceMap* refMap, TypeMap* typeMap,
                TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoRemoveExits(refMap, typeMap));
        setName("RemoveExits");
    }
};

}  // namespace P4

#endif /* _MIDEND_REMOVEEXITS_H_ */
