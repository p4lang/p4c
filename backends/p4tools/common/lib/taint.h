#ifndef BACKENDS_P4TOOLS_COMMON_LIB_TAINT_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_TAINT_H_

#include "backends/p4tools/common/lib/model.h"
#include "ir/ir.h"

namespace P4Tools {

class Taint {
 public:
    static const IR::StringLiteral TAINTED_STRING_LITERAL;

    /// @returns a expression in which taint has been propagated upwards. At the end, this will
    /// either return a literal, a Member/PathExpression, or a concatenation. Any non-tainted
    /// variable is replaced with a zero constant. This function is used for the generation of taint
    /// masks.
    static const IR::Expression *propagateTaint(const IR::Expression *expr);

    /// @returns whether the given expression is tainted. An expression is tainted if one or more
    /// bits of the expression are expected to evaluate to (possibly part of) IR::TaintExpression.
    static bool hasTaint(const IR::Expression *expr);

    /// @returns the mask for the corresponding program packet, indicating bits of the expression
    /// which are not tainted.
    static const IR::Literal *buildTaintMask(const Model *evaluatedModel,
                                             const IR::Expression *programPacket);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_TAINT_H_ */
