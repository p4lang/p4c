#ifndef _BACKENDS_BMV2_LOWER_H_
#define _BACKENDS_BMV2_LOWER_H_

#include "ir/ir.h"
#include "frontends/p4/evaluator/blockMap.h"

namespace BMV2 {

// Convert expressions not supported on BMv2
class LowerExpressions : public Transform {
    P4::TypeMap* typeMap;

    // Cannot shift with a value larger than 8 bits
    const int maxShiftWidth = 8;
    
    const IR::Expression* shift(const IR::Operation_Binary* expression) const;
 public:
    explicit LowerExpressions(P4::TypeMap* typeMap) : typeMap(typeMap) {}

    const IR::Node* postorder(IR::Shl* expression) override
    { return shift(expression); }
    const IR::Node* postorder(IR::Shr* expression) override
    { return shift(expression); }
    const IR::Node* postorder(IR::Cast* expression) override;
    const IR::Node* postorder(IR::Slice* expression) override;
    const IR::Node* postorder(IR::Concat* expression) override;
    const IR::Node* preorder(IR::P4Table* table) override
    { prune(); return table; }  // don't simplify expressions in table
};

// Remove Slices on the lhs of an assignment
class RemoveLeftSlices : public Transform {
    P4::TypeMap* typeMap;
 public:
    explicit RemoveLeftSlices(P4::TypeMap* typeMap) : typeMap(typeMap) {}
    const IR::Node* postorder(IR::AssignmentStatement* stat) override;
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_LOWER_H_ */
