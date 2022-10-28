#ifndef IR_IRUTILS_H_
#define IR_IRUTILS_H_

#include <vector>

#include <boost/none.hpp>
#include <boost/optional/optional.hpp>

#include "frontends/common/constantFolding.h"
#include "frontends/p4/strengthReduction.h"
#include "ir/ir.h"
#include "lib/big_int_util.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/exceptions.h"

namespace IR {

/// Utility functions for generating IR nodes.
//
// Some of these are just thin wrappers around functions in the IR, but it's nice having everything
// in one place.
class IRUtils {
 public:
    /* =========================================================================================
     *  Types
     * ========================================================================================= */

    /// @returns a representation of bit<>. If isSigned is true, will return an int<>.
    static const Type_Bits* getBitType(int size, bool isSigned = false);

    /// @returns a representation of bit<> that is just wide enough to fit the given value.
    static const Type_Bits* getBitTypeToFit(int value);

    /* =========================================================================================
     *  Expressions
     * ========================================================================================= */
 public:
    /// @returns a constant. The value is cached.
    static const Constant* getConstant(const Type* type, big_int v);

    /// @returns a bool literal. The value is cached.
    static const BoolLiteral* getBoolLiteral(bool value);

    /// Applies expression optimizations to the input node.
    /// Currently, performs constant folding and strength reduction.
    template <class T>
    static const T* optimizeExpression(const T* node) {
        auto pass = PassRepeated({
            new P4::StrengthReduction(nullptr, nullptr, nullptr),
            new P4::ConstantFolding(nullptr, nullptr, false),
        });
        node = node->apply(pass);
        BUG_CHECK(::errorCount() == 0, "Encountered errors while trying to optimize expressions.");
        return node;
    }

    /// @returns the "default" value for a given type. The current mapping is
    /// Type_Bits       0
    /// Type_Boolean    false
    static const Literal* getDefaultValue(const Type* type);

    /* =========================================================================================
     *  Other helper functions
     * ========================================================================================= */
 public:
    /// @returns the big int value stored in a literal.
    static big_int getBigIntFromLiteral(const Literal*);

    /// @returns the integer value stored in a literal. We use int here.
    static int getIntFromLiteral(const Literal*);

    /// @returns the maximum value that can fit into this type.
    /// This function assumes a big int value, meaning it only supports bit vectors and booleans.
    // This is 2^(t->size) - 1 for unsigned and 2^(t->size - 1) - 1 for signed.
    static big_int getMaxBvVal(const Type* t);

    /// @returns the maximum big_int value that can fit into this bit width.
    static big_int getMaxBvVal(int bitWidth);

    /// @returns the minimum value that can fit into this type.
    // This is 0 for unsigned and -(2^(t->size - 1)) for signed.
    static big_int getMinBvVal(const Type* t);

    /// Given an StructExpression, returns a flat vector of the expressions contained in that
    /// struct. Unfortunately, list and struct expressions are similar but have no common ancestors.
    /// This is why we require two separate methods.
    static std::vector<const Expression*> flattenStructExpression(
        const StructExpression* structExpr);

    /// Given an ListExpression, returns a flat vector of the expressions contained in that
    /// list.
    static std::vector<const Expression*> flattenListExpression(const ListExpression* listExpr);
};

}  // namespace IR

#endif /* IR_IRUTILS_H_ */
