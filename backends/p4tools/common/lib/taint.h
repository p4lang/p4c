#ifndef COMMON_LIB_TAINT_H_
#define COMMON_LIB_TAINT_H_

#include "backends/p4tools/common/lib/model.h"
#include "ir/ir.h"

namespace P4Tools {

class Taint {
 public:
    static const IR::StringLiteral taintedStringLiteral;

    /// @returns the mask for the corresponding program packet. In the first step, the packet
    /// expression is collapsed so that only concatenations remain. Any taint variable bubbles up.
    /// In the second step, taint variables are replaces with zero bit vectors, whereas any other
    /// expression is filled with the maximum value.
    static const IR::Literal* buildTaintMask(const SymbolicMapType& varMap,
                                             const Model* completedModel,
                                             const IR::Expression* programPacket);

    /// @returns a expression in which taint has been propagated upwards. At the end, this will
    /// either return a literal, a member/pathexpression, or a concatenation. Any non-tainted
    /// variable is replaced with a zero constant. This function is used for the generation of taint
    /// masks.
    static const IR::Expression* propagateTaint(const SymbolicMapType& varMap,
                                                const IR::Expression* expr);

    /// @returns whether the given expression is tainted. For most expressions, this just checks
    /// whether it containts a member or path expression that has the *taint keyword. This does not
    /// hold for slices and concatenations. Here we need to check whether the slice range overlaps
    /// with the tainted range of the concatenation. We do this by retrieving all the tainted slices
    /// of the concatenation expression and then checking whether the slice overlaps
    static bool hasTaint(const SymbolicMapType& varMap, const IR::Expression* expr);
};

}  // namespace P4Tools

#endif /* COMMON_LIB_TAINT_H_ */
