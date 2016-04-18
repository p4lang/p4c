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
    SimpleControlsInliner(P4::ReferenceMap* refMap) : refMap(refMap), workToDo(nullptr) {}
    const IR::Node* preorder(IR::MethodCallStatement* statement) override;
    const IR::Node* preorder(IR::P4Control* caller) override;
    Visitor::profile_t init_apply(const IR::Node* node) override;
};

class SimpleActionsInliner : public P4::AbstractActionInliner {
    P4::ReferenceMap* refMap;
    std::map<const IR::MethodCallStatement*, const IR::P4Action*>* replMap;
 public: 
    SimpleActionsInliner(P4::ReferenceMap* refMap) : refMap(refMap), replMap(nullptr) {}
    Visitor::profile_t init_apply(const IR::Node* node) override;
    const IR::Node* preorder(IR::P4Parser* cont) override
    { prune(); return cont; }  // skip
    const IR::Node* preorder(IR::P4Action* action) override;
    const IR::Node* postorder(IR::P4Action* action) override;
    const IR::Node* preorder(IR::MethodCallStatement* statement) override;
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_INLINING_H_ */
