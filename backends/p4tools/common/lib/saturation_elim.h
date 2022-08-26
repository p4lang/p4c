#ifndef COMMON_LIB_SATURATION_ELIM_H_
#define COMMON_LIB_SATURATION_ELIM_H_

#include "ir/ir.h"

namespace P4Tools {

/// Contains utility functions for eliminating saturating arithmetic.
class SaturationElim {
 public:
    /// @returns true when the given expression is a saturating addition or a saturating
    /// subtraction. Otherwise, returns false.
    static bool isSaturationOperation(const IR::Expression* expr);

    /// Eliminates saturating arithmetic by rewriting the given binary expression into a
    /// conditional ternary expression. The two operands in the given expression are assumed to be
    /// pure expressions (i.e., they have no side effects), and may be evaluated multiple times in
    /// the resulting expression. No checks are made to determine whether this assumption holds.
    static const IR::Mux* eliminate(const IR::Operation_Binary* binary);
};

}  // namespace P4Tools

#endif /* COMMON_LIB_SATURATION_ELIM_H_ */
