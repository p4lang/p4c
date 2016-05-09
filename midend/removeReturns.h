#ifndef _MIDEND_REMOVERETURNS_H_
#define _MIDEND_REMOVERETURNS_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/typeMap.h"

namespace P4 {

// This replaces 'returns' and/or 'exits' by ifs
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
            refMap(refMap), variableName(varName) { CHECK_NULL(refMap); }

    const IR::Node* preorder(IR::Function* function) override
    { prune(); return function; }  // We leave returns in functions alone
    const IR::Node* preorder(IR::BlockStatement* statement) override;
    const IR::Node* preorder(IR::ReturnStatement* statement) override;
    const IR::Node* preorder(IR::ExitStatement* statement) override;
    const IR::Node* preorder(IR::IfStatement* statement) override;
    const IR::Node* preorder(IR::SwitchStatement* statement) override;

    const IR::Node* preorder(IR::P4Action* action) override;
    const IR::Node* preorder(IR::P4Control* control) override;
};

class RemoveExits : public RemoveReturns {
    std::set<const IR::Node*> callsExit;  // actions, tables
    TypeMap* typeMap;
 public:
    explicit RemoveExits(ReferenceMap* refMap, TypeMap* typeMap) :
            RemoveReturns(refMap, "hasExited"), typeMap(typeMap) { CHECK_NULL(typeMap); }

    const IR::Node* preorder(IR::ExitStatement* action) override;
    const IR::Node* preorder(IR::P4Table* table) override;
};

}  // namespace P4

#endif /* _MIDEND_REMOVERETURNS_H_ */
