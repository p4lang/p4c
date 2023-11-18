#ifndef BACKENDS_P4TOOLS_COMMON_LIB_GEN_EQ_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_GEN_EQ_H_

#include "ir/ir.h"

namespace P4Tools {

/// Generates an equality on two input expressions, recursing into lists and structs.
/// This supports fuzzy matching on singleton lists: singleton lists are considered the same as
/// their singleton elements. This is implemented by eagerly recursing into singleton lists before
/// attempting to generate the equality.
class GenEq {
 public:
    static const IR::Expression *equate(const IR::Expression *left, const IR::Expression *right);

 private:
    /// Recursively resolve lists of size 1 by returning the expression contained within.
    static const IR::Expression *resolveSingletonList(const IR::Expression *expr);

    /// Flatten and compare two lists.
    /// Important, this equation assumes that struct expressions have been ordered.
    /// This calculation does not match the names of the struct expressions.
    static const IR::Expression *equateListTypes(const IR::Expression *left,
                                                 const IR::Expression *right);

    /// Convenience method for producing a typed Eq node on the given expressions.
    static const IR::Equ *mkEq(const IR::Expression *e1, const IR::Expression *e2);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_GEN_EQ_H_ */
