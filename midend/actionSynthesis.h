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

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
Policy which selects the control blocks where action
synthesis is applied.
*/
class ActionSynthesisPolicy {
 public:
    virtual ~ActionSynthesisPolicy() {}
    /**
       If the policy returns true the control block is processed,
       otherwise it is left unchanged.
    */
    virtual bool convert(const Visitor::Context *ctxt, const IR::P4Control* control) = 0;

    /**
       Called for each statement that may be put into an action when there are preceeding
       statements already put into an action --
        @param ctxt context of the code being processsed (control and parents)
        @param blk  previous statement(s) being put into an action
        @param stmt statement to be added to the block for a single action
        @returns
            true  statement should be added to the same action
            false statement should start a new action
    */
    virtual bool can_combine(const Visitor::Context *, const IR::BlockStatement *,
                             const IR::StatOrDecl *) {
        return true; }
};

/**
Convert direct action calls to table invocations.

control c() {
  action x(in bit b) { ... }
  apply { x(e); }
}

turns into

control c() {
  action x(in bit b) { ... }
  table _tmp() {
    actions = { x(e); }
    const default_action = x();
  }
  apply { _tmp.apply(); }
}

For this to work all variable declarations must have been moved to the beginning.
*/
class DoMoveActionsToTables : public Transform {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    std::vector<const IR::P4Table*> tables;  // inserted tables

 public:
    DoMoveActionsToTables(ReferenceMap* refMap, TypeMap* typeMap)
            : refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("DoMoveActionsToTables"); }
    const IR::Node* preorder(IR::P4Parser* parser) override
    { prune(); return parser; }
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* preorder(IR::P4Control* control) override
    { tables.clear(); return control; }
    const IR::Node* preorder(IR::P4Action* action) override
    { prune(); return action; }
    const IR::Node* postorder(IR::MethodCallStatement* statement) override;
};

/**
Convert some statements into action invocations by synthesizing new actions.
E.g.

control c(inout bit x) {
   apply { x = 1; }
}

is converted to:

control c(inout bit x) {
   action act() { x = 1; }
   apply { act(); }
}

For this to work all variable declarations must have been moved to the beginning.
*/
class DoSynthesizeActions : public Transform {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    std::vector<const IR::P4Action*> actions;  // inserted actions
    bool changes = false;
    ActionSynthesisPolicy* policy;

 public:
    // If true the statement must be moved to an action
    bool mustMove(const IR::MethodCallStatement* statement);
    bool mustMove(const IR::AssignmentStatement* statement);

    DoSynthesizeActions(ReferenceMap* refMap, TypeMap* typeMap, ActionSynthesisPolicy* policy) :
            refMap(refMap), typeMap(typeMap), policy(policy)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("DoSynthesizeActions"); }
    const IR::Node* preorder(IR::P4Parser* parser) override
    { prune(); return parser; }
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* preorder(IR::P4Control* control) override;
    const IR::Node* preorder(IR::P4Action* action) override
    { prune(); return action; }  // skip actions
    // We do not handle return: this pass should be called after it has been removed
    const IR::Node* preorder(IR::BlockStatement* statement) override;
    const IR::Node* preorder(IR::AssignmentStatement* statement) override;
    const IR::Node* preorder(IR::MethodCallStatement* statement) override;
    const IR::Node* preorder(IR::ExitStatement* statement) override;
    const IR::Node* preorder(IR::Function* function) override
    { prune(); return function; }

 protected:
    const IR::Statement* createAction(const IR::Statement* body);
};

class SynthesizeActions : public PassManager {
 public:
    SynthesizeActions(ReferenceMap* refMap, TypeMap* typeMap,
                      ActionSynthesisPolicy* policy = nullptr,
                      TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoSynthesizeActions(refMap, typeMap, policy));
        setName("SynthesizeActions");
    }
};

class MoveActionsToTables : public PassManager {
 public:
    MoveActionsToTables(ReferenceMap* refMap, TypeMap* typeMap,
            TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoMoveActionsToTables(refMap, typeMap));
        setName("MoveActionsToTables");
    }
};

}  // namespace P4

#endif /* _MIDEND_ACTIONSYNTHESIS_H_ */
