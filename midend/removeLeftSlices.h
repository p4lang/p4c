#ifndef _MIDEND_REMOVELEFTSLICES_H_
#define _MIDEND_REMOVELEFTSLICES_H_

#include "ir/ir.h"
#include "frontends/common/typeMap.h"

namespace P4 {

// Remove Slices on the lhs of an assignment
class RemoveLeftSlices : public Transform {
    P4::TypeMap* typeMap;
 public:
    explicit RemoveLeftSlices(P4::TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); setName("RemoveLeftSlices"); }
    const IR::Node* postorder(IR::AssignmentStatement* stat) override;
};

}  // namespace P4

#endif /* _MIDEND_REMOVELEFTSLICES_H_ */
