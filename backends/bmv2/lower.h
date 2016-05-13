#ifndef _BACKENDS_BMV2_LOWER_H_
#define _BACKENDS_BMV2_LOWER_H_

#include "ir/ir.h"
#include "frontends/common/typeMap.h"

namespace BMV2 {

// Convert expressions not supported on BMv2
class LowerExpressions : public Transform {
    P4::TypeMap* typeMap;
    // Cannot shift with a value larger than 8 bits
    const int maxShiftWidth = 8;

    const IR::Expression* shift(const IR::Operation_Binary* expression) const;
 public:
    explicit LowerExpressions(P4::TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); setName("LowerExpressions"); }
    const IR::Node* postorder(IR::Shl* expression) override
    { return shift(expression); }
    const IR::Node* postorder(IR::Shr* expression) override
    { return shift(expression); }
    const IR::Node* postorder(IR::Cast* expression) override;
    const IR::Node* postorder(IR::Neg* expression) override;
    const IR::Node* postorder(IR::Slice* expression) override;
    const IR::Node* postorder(IR::Concat* expression) override;
    const IR::Node* preorder(IR::P4Table* table) override
    { prune(); return table; }  // don't simplify expressions in table
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_LOWER_H_ */
