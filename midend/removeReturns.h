#ifndef _MIDEND_REMOVERETURNS_H_
#define _MIDEND_REMOVERETURNS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/typeMap.h"

namespace P4 {

// This replaces 'returns' by ifs:
// e.g.
// control c(inout bit x) { apply {
//   if (x) return;
//   x = !x;
// }}
// becomes:
// control c(inout bit x) {
//   bool hasReturned;
//   apply {
//     hasReturned = false;
//     if (x) hasReturned = true;
//     if (!hasReturned)
//       x = !x;
// }}
class RemoveReturns : public Transform {
 protected:
    P4::ReferenceMap* refMap;
    IR::ID            returnVar;  // one for each context
    cstring           variableName;

    enum class Returns {
        Yes,
        No,
        Maybe
    };

    std::vector<Returns> stack;
    void push() { stack.push_back(Returns::No); }
    void pop() { stack.pop_back(); }
    void set(Returns r) { BUG_CHECK(!stack.empty(), "Empty stack"); stack.back() = r; }
    Returns hasReturned() { BUG_CHECK(!stack.empty(), "Empty stack"); return stack.back(); }

 public:
    explicit RemoveReturns(P4::ReferenceMap* refMap, cstring varName = "hasReturned") :
            refMap(refMap), variableName(varName)
    { visitDagOnce = false; CHECK_NULL(refMap); setName("RemoveReturns"); }

    const IR::Node* preorder(IR::Function* function) override
    { prune(); return function; }  // We leave returns in functions alone
    const IR::Node* preorder(IR::BlockStatement* statement) override;
    const IR::Node* preorder(IR::ReturnStatement* statement) override;
    const IR::Node* preorder(IR::ExitStatement* statement) override;
    const IR::Node* preorder(IR::IfStatement* statement) override;
    const IR::Node* preorder(IR::SwitchStatement* statement) override;

    const IR::Node* preorder(IR::P4Action* action) override;
    const IR::Node* preorder(IR::P4Control* control) override;
    const IR::Node* preorder(IR::P4Parser* parser) override
    { prune(); return parser; }
};

// This removes "exit" calls.  It is significantly more involved than return removal,
// since an exit in an action causes the calling control to terminate.
// This pass assumes that each statement in a control block can
// exit only once - so it should be run after a pass that enforces this.
// (E.g., it does not handle:
// if (t1.apply().hit && t2.apply().hit) { ... }
// It also assumes that there are no global actions and that action calls have been inlined.
class RemoveExits : public RemoveReturns {
    TypeMap* typeMap;
    // In this class "Return" (inherited from RemoveReturns) should be read as "Exit"
    std::set<const IR::Node*> callsExit;  // actions, tables
    void callExit(const IR::Node* node);
 public:
    RemoveExits(ReferenceMap* refMap, TypeMap* typeMap) :
            RemoveReturns(refMap, "hasExited"), typeMap(typeMap)
    { visitDagOnce = false; CHECK_NULL(typeMap); setName("RemoveExits"); }

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

}  // namespace P4

#endif /* _MIDEND_REMOVERETURNS_H_ */
