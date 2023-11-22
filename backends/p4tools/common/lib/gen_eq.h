#ifndef BACKENDS_P4TOOLS_COMMON_LIB_GEN_EQ_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_GEN_EQ_H_

#include "ir/ir.h"

namespace P4Tools::GenEq {

/// Generates a semantic equality on two input expressions, recursing into lists and structs.
/// This supports fuzzy matching on singleton lists: singleton lists are considered the same as
/// their singleton elements. This is implemented by eagerly recursing into singleton lists before
/// attempting to generate the equality. The expression types need to be semantically compatible.
const IR::Expression *equate(const IR::Expression *left, const IR::Expression *right);

}  // namespace P4Tools::GenEq

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_GEN_EQ_H_ */
