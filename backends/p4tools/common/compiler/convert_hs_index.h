#ifndef BACKENDS_P4TOOLS_COMMON_COMPILER_CONVERT_HS_INDEX_H_
#define BACKENDS_P4TOOLS_COMMON_COMPILER_CONVERT_HS_INDEX_H_

#include <cstddef>

#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4Tools {

/// The main class for finding non-concrete header stack indices.
class HSIndexToMember : public Transform {
 public:
    const IR::Node *postorder(IR::ArrayIndex *curArrayIndex) override;

    /// Convert a parent expression and an index into a member expression with that
    /// particular index as string member. The type is used to specify the member type.
    static const IR::ArrayIndex *produceStackIndex(const IR::Type *type,
                                                   const IR::Expression *expression,
                                                   size_t arrayIndex);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_COMPILER_CONVERT_HS_INDEX_H_ */
