#ifndef BACKENDS_P4TOOLS_COMMON_LIB_IR_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_IR_H_

#include <vector>

#include <boost/none.hpp>
#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/lib/formulae.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/strengthReduction.h"
#include "ir/ir.h"
#include "lib/big_int_util.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/exceptions.h"

namespace P4Tools {

/// Utility functions for generating IR nodes.
//
// Some of these are just thin wrappers around functions in the IR, but it's nice having everything
// in one place.
class IRUtils {
 public:
    /* =========================================================================================
     *  Types
     * ========================================================================================= */

    /// To represent header validity, we pretend that every header has a field that reflects the
    /// header's validity state. This is the name of that field. This is not a valid P4 identifier,
    /// so it is guaranteed to not conflict with any other field in the header.
    static const cstring Valid;

    /// @returns a representation of bit<>. If isSigned is true, will return an int<>.
    static const IR::Type_Bits* getBitType(int size, bool isSigned = false);

    /// @returns a representation of bit<> that is just wide enough to fit the given value.
    static const IR::Type_Bits* getBitTypeToFit(int value);

    /* =========================================================================================
     *  Variables and symbolic constants.
     * ========================================================================================= */
 public:
    /// @see Zombie::getVar.
    static const StateVariable& getZombieVar(const IR::Type* type, int incarnation, cstring name);

    /// @see Zombie::getConst.
    static const StateVariable& getZombieConst(const IR::Type* type, int incarnation, cstring name);

    /// @see IRUtils::getZombieConst.
    /// This function is used to generated variables caused by undefined behavior. This is merely a
    /// wrapper function for the creation of a new Taint IR object.
    static const IR::TaintExpression* getTaintExpression(const IR::Type* type);

    /// Creates a new member variable from a concolic variable but replaces its concolic ID.
    /// This is used within concolic methods to map back several concolic variables from a single
    /// method call.
    static const StateVariable& getConcolicMember(const IR::ConcolicVariable* var, int concolicId);

    /// @returns the zombie variable with the given type, for tracking an aspect of the given
    /// table. The returned variable will be named p4t*zombie.table.t.name.idx1.idx2, where t is
    /// the name of the given table. The "idx1" and "idx2" components are produced only if idx1 and
    /// idx2 are given, respectively.
    static const StateVariable& getZombieTableVar(const IR::Type* type, const IR::P4Table* table,
                                                  cstring name,
                                                  boost::optional<int> idx1_opt = boost::none,
                                                  boost::optional<int> idx2_opt = boost::none);

    /// @returns the state variable for the validity of the given header instance. The resulting
    ///     variable will be boolean-typed.
    ///
    /// @param headerRef a header instance. This is either a Member or a PathExpression.
    static StateVariable getHeaderValidity(const IR::Expression* headerRef);

    /// @returns a StateVariable that is postfixed with "*". This is used for PathExpressions with
    /// are not yet members.
    static StateVariable addZombiePostfix(const IR::Expression* paramPath,
                                          const IR::Type_Base* baseType);

    /* =========================================================================================
     *  Expressions
     * ========================================================================================= */
 public:
    static const IR::Constant* getConstant(const IR::Type* type, big_int v);

    static const IR::BoolLiteral* getBoolLiteral(bool value);

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

    /// @returns the default value for a given type. The current mapping is
    /// Type_Bits       0
    /// Type_Varbits       0 (set to width of "size")
    /// Type_Boolean    false
    static const IR::Literal* getDefaultValue(const IR::Type* type);

    /* =========================================================================================
     *  Other helper functions
     * ========================================================================================= */
 public:
    /// @returns the big int value stored in a literal.
    static big_int getBigIntFromLiteral(const IR::Literal*);

    /// @returns the integer value stored in a literal. We use int here.
    static int getIntFromLiteral(const IR::Literal*);

    /// @returns the maximum value that can fit into this type.
    /// This function assumes a big int value, meaning it only supports bit vectors and booleans.
    // This is 2^(t->size) - 1 for unsigned and 2^(t->size - 1) - 1 for signed.
    static big_int getMaxBvVal(const IR::Type* t);

    /// @returns the maximum big_int value that can fit into this bit width.
    static big_int getMaxBvVal(int bitWidth);

    /// @returns the minimum value that can fit into this type.
    // This is 0 for unsigned and -(2^(t->size - 1)) for signed.
    static big_int getMinBvVal(const IR::Type* t);

    /// @returns a IR::Constant with a random big integer that fits the specified bit width.
    /// The type will be an unsigned Type_Bits with @param bitWidth.
    static const IR::Constant* getRandConstantForWidth(int bitWidth);

    /// @returns a IR::Constant with a random big integer that fits the specified @param type.
    static const IR::Constant* getRandConstantForType(const IR::Type_Bits* type);

    /// @returns a method call to an internal extern consumed by the interpreter. The return type
    /// is typically Type_Void.
    static const IR::MethodCallExpression* generateInternalMethodCall(
        cstring methodName, const std::vector<const IR::Expression*>& argVector,
        const IR::Type* returnType = IR::Type_Void::get());

    /// Given an IR::StructExpression, returns a flat vector of the expressions contained in that
    /// struct. Unfortunately, list and struct expressions are similar but have no common ancestors.
    /// This is why we require two separate methods.
    static std::vector<const IR::Expression*> flattenStructExpression(
        const IR::StructExpression* structExpr);

    /// Given an IR::ListExpression, returns a flat vector of the expressions contained in that
    /// list.
    static std::vector<const IR::Expression*> flattenListExpression(
        const IR::ListExpression* listExpr);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_IR_H_ */
