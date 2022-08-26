#ifndef COMMON_COMPILER_HS_INDEX_SIMPLIFY_H_
#define COMMON_COMPILER_HS_INDEX_SIMPLIFY_H_

#include "ir/ir.h"

namespace P4Tools {

/// The main class for finding non-concrete header stack indices.
class HSIndexToMember : public Transform {
 public:
    const IR::Node* postorder(IR::ArrayIndex* curArrayIndex) override;

    /// Convert a parent expression and an index into a member expression with that
    /// particular index as string member. The type is used to specify the member type.
    static const IR::Member* produceStackIndex(const IR::Type* type,
                                               const IR::Expression* expression, int arrayIndex);
};

}  // namespace P4Tools

#endif /* COMMON_COMPILER_HS_INDEX_SIMPLIFY_H_ */
