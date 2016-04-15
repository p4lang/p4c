#ifndef _BACKENDS_BMV2_CONTROLINLINING_H_
#define _BACKENDS_BMV2_CONTROLINLINING_H_

#include "ir/ir.h"
#include "midend/inlining.h"

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

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_CONTROLINLINING_H_ */
