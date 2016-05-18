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

#ifndef _MIDEND_ACTIONSYNTHESIS_H_
#define _MIDEND_ACTIONSYNTHESIS_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/typeMap.h"

namespace P4 {

// Convert direct action calls to table invocations.
// control c() {
//   action x(in bit b) { ... }
//   apply { x(e); }
// }
// turns into
// control c() {
//   action x(in bit b) { ... }
//   table _tmp() {
//     actions = { x(e); }
//     const default_action = x();
//   }
//   apply { _tmp.apply(); }
// }
// For this to work all variable declarations must have been moved to the beginning.
class MoveActionsToTables : public Transform {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    std::vector<const IR::P4Table*> tables;  // inserted tables

 public:
    MoveActionsToTables(ReferenceMap* refMap, TypeMap* typeMap)
            : refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("MoveActionsToTables"); }
    const IR::Node* preorder(IR::P4Parser* parser) override
    { prune(); return parser; }
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* preorder(IR::P4Control* control) override
    { tables.clear(); return control; }
    const IR::Node* preorder(IR::P4Action* action) override
    { prune(); return action; }
    const IR::Node* postorder(IR::MethodCallStatement* statement) override;
};

// Convert some statements into action invocations by synthesizing new actions.
// E.g.
// control c(inout bit x) {
//    apply { x = 1; }
// }
// is converted to:
// control c(inout bit x) {
//    action act() { x = 1; }
//    apply { act(); }
// }
// For this to work all variable declarations must have been moved to the beginning.
class SynthesizeActions : public Transform {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    std::vector<const IR::P4Action*> actions;  // inserted actions
    bool moveEmits = false;
    bool changes = false;

 public:
    // If true the statement must be moved to an action
    bool mustMove(const IR::MethodCallStatement* statement);

    // If moveEmits is true, move emit statements to actions, else
    // leave them in control blocks.
    SynthesizeActions(ReferenceMap* refMap, TypeMap* typeMap, bool moveEmits = false) :
            refMap(refMap), typeMap(typeMap), moveEmits(moveEmits)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("SynthesizeActions"); }
    const IR::Node* preorder(IR::P4Parser* parser) override
    { prune(); return parser; }
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* preorder(IR::P4Control* control) override
    { actions.clear(); changes = false; return control; }
    const IR::Node* preorder(IR::P4Action* action) override
    { prune(); return action; }  // skip actions
    // We do not handle return and exit: this pass should be called after
    // these have been removed
    const IR::Node* preorder(IR::BlockStatement* statement) override;
    const IR::Node* preorder(IR::AssignmentStatement* statement) override;
    const IR::Node* preorder(IR::MethodCallStatement* statement) override;

 protected:
    const IR::Statement* createAction(const IR::Statement* body);
};

}  // namespace P4

#endif /* _MIDEND_ACTIONSYNTHESIS_H_ */
