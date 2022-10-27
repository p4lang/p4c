#include "ir/irutils.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>

#include "lib/safe_vector.h"

namespace IR {

/* =============================================================================================
 *  Types
 * ============================================================================================= */

const Type_Bits* IRUtils::getBitType(int size, bool isSigned) {
    // Types are cached already.
    return Type_Bits::get(size, isSigned);
}

const Type_Bits* IRUtils::getBitTypeToFit(int value) {
    // To represent a number N, we need ceil(log2(N + 1)) bits.
    int width = ceil(log2(value + 1));
    return getBitType(width);
}

/* =============================================================================================
 *  Expressions
 * ============================================================================================= */

const Constant* IRUtils::getConstant(const Type* type, big_int v) {
    // Do not cache varbits.
    if (const auto* varbit = type->to<Extracted_Varbits>()) {
        return new Constant(varbit, v);
    }
    // Only cache bits with width lower than 16 bit to restrict the size of the cache.
    const auto* tb = type->to<Type_Bits>();
    if (type->width_bits() > 16 || tb == nullptr) {
        return new Constant(type, v);
    }
    // Constants are interned. Keys in the intern map are pairs of types and values.
    using key_t = std::tuple<int, bool, big_int>;
    static std::map<key_t, const Constant*> constants;

    auto*& result = constants[{tb->width_bits(), tb->isSigned, v}];
    if (result == nullptr) {
        result = new Constant(tb, v);
    }

    return result;
}

const BoolLiteral* IRUtils::getBoolLiteral(bool value) {
    // Boolean literals are interned.
    static std::map<bool, const BoolLiteral*> literals;

    auto*& result = literals[value];
    if (result == nullptr) {
        result = new BoolLiteral(Type::Boolean::get(), value);
    }
    return result;
}

const Literal* IRUtils::getDefaultValue(const Type* type) {
    if (type->is<Type_Bits>()) {
        return getConstant(type, 0);
    }
    if (type->is<Type_Boolean>()) {
        return getBoolLiteral(false);
    }
    P4C_UNIMPLEMENTED("Default value for type %s of type %s not implemented.", type,
                      type->node_type_name());
}

/* =============================================================================================
 *  Other helper functions
 * ============================================================================================= */

big_int IRUtils::getBigIntFromLiteral(const Literal* l) {
    if (const auto* c = l->to<Constant>()) {
        return c->value;
    }
    if (const auto* b = l->to<BoolLiteral>()) {
        return b->value ? 1 : 0;
    }
    P4C_UNIMPLEMENTED("Literal %1% of type %2% not supported.", l, l->node_type_name());
}

int IRUtils::getIntFromLiteral(const Literal* l) {
    if (const auto* c = l->to<Constant>()) {
        if (!c->fitsInt()) {
            BUG("Value %1% too large for Int.", l);
        }
        return c->asInt();
    }
    if (const auto* b = l->to<BoolLiteral>()) {
        return b->value ? 1 : 0;
    }
    P4C_UNIMPLEMENTED("Literal %1% of type %2% not supported.", l, l->node_type_name());
}

big_int IRUtils::getMaxBvVal(int bitWidth) { return pow(big_int(2), bitWidth) - 1; }

big_int IRUtils::getMaxBvVal(const Type* t) {
    if (const auto* tb = t->to<Type_Bits>()) {
        return tb->isSigned ? getMaxBvVal(tb->width_bits() - 1) : getMaxBvVal(tb->width_bits());
    }
    if (t->is<Type_Boolean>()) {
        return 1;
    }
    P4C_UNIMPLEMENTED("Maximum value calculation for type %1% not implemented.", t);
}

big_int IRUtils::getMinBvVal(const Type* t) {
    if (const auto* tb = t->to<Type_Bits>()) {
        return (tb->isSigned) ? -(big_int(1) << tb->width_bits() - 1) : big_int(0);
    }
    if (t->is<Type_Boolean>()) {
        return 0;
    }
    P4C_UNIMPLEMENTED("Maximum value calculation for type %1% not implemented.", t);
}

std::vector<const Expression*> IRUtils::flattenStructExpression(
    const StructExpression* structExpr) {
    std::vector<const Expression*> exprList;
    for (const auto* listElem : structExpr->components) {
        if (const auto* subStructExpr = listElem->expression->to<StructExpression>()) {
            auto subList = flattenStructExpression(subStructExpr);
            exprList.insert(exprList.end(), subList.begin(), subList.end());
        } else {
            exprList.push_back(listElem->expression);
        }
    }
    return exprList;
}

std::vector<const Expression*> IRUtils::flattenListExpression(const ListExpression* listExpr) {
    std::vector<const Expression*> exprList;
    for (const auto* listElem : listExpr->components) {
        if (const auto* subListExpr = listElem->to<ListExpression>()) {
            auto subList = flattenListExpression(subListExpr);
            exprList.insert(exprList.end(), subList.begin(), subList.end());
        } else {
            exprList.push_back(listElem);
        }
    }
    return exprList;
}

}  // namespace IR
