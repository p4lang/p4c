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
//     actions = { x; }
//     const default_action = x(e);
//   }
//   apply { _tmp.apply(); }
// }
// For this to work all variable declarations must have been moved to the beginning.
class MoveActionsToTables : public Transform {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    IR::Vector<IR::P4Table> tables;  // inserted tables
    
 public:
    MoveActionsToTables(ReferenceMap* refMap, TypeMap* typeMap)
            : refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); }
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
    IR::Vector<IR::P4Action> actions;  // inserted actions
    bool moveEmits = false;
    bool changes = false;
 public:
    // If true the statement must be moved to an action
    bool mustMove(const IR::MethodCallStatement* statement);
    
    // If moveEmits is true, move emit statements to actions, else
    // leave them in control blocks.
    SynthesizeActions(ReferenceMap* refMap, TypeMap* typeMap, bool moveEmits = false) :
            refMap(refMap), typeMap(typeMap), moveEmits(moveEmits)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); }
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
