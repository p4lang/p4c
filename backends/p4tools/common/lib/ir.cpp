#include "backends/p4tools/common/lib/ir.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/detail/default_ops.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/common/lib/zombie.h"
#include "lib/safe_vector.h"

namespace P4Tools {

/* =============================================================================================
 *  Types
 * ============================================================================================= */

const cstring IRUtils::Valid = "*valid";

const IR::Type_Bits* IRUtils::getBitType(int size, bool isSigned) {
    // Types are cached already.
    return IR::Type_Bits::get(size, isSigned);
}

const IR::Type_Bits* IRUtils::getBitTypeToFit(int value) {
    // To represent a number N, we need ceil(log2(N + 1)) bits.
    int width = ceil(log2(value + 1));
    return getBitType(width);
}

/* =============================================================================================
 *  Variables
 * ============================================================================================= */

const StateVariable& IRUtils::getZombieTableVar(const IR::Type* type, const IR::P4Table* table,
                                                cstring name, boost::optional<int> idx1_opt,
                                                boost::optional<int> idx2_opt) {
    // Mash the table name, the given name, and the optional indices together.
    // XXX To be nice, we should probably build a PathExpression, but that's annoying to do, and we
    // XXX can probably get away with this.
    std::stringstream out;
    out << table->name.toString() << "." << name;
    if (idx1_opt) {
        out << "." << *idx1_opt;
    }
    if (idx2_opt) {
        out << "." << *idx2_opt;
    }

    return Zombie::getVar(type, 0, out.str());
}

const StateVariable& IRUtils::getZombieVar(const IR::Type* type, int incarnation, cstring name) {
    return Zombie::getVar(type, incarnation, name);
}

const StateVariable& IRUtils::getZombieConst(const IR::Type* type, int incarnation, cstring name) {
    return Zombie::getConst(type, incarnation, name);
}

StateVariable IRUtils::getHeaderValidity(const IR::Expression* headerRef) {
    return new IR::Member(IR::Type::Boolean::get(), headerRef, Valid);
}

/* =============================================================================================
 *  Expressions
 * ============================================================================================= */

const IR::Constant* IRUtils::getConstant(const IR::Type* type, big_int v) {
    // Do not cache varbits.
    if (auto varbit = type->to<IR::Extracted_Varbits>()) {
        return new IR::Constant(varbit, v);
    }
    // Only cache bits with width lower than 16 bit to restrict the size of the cache.
    const auto* tb = type->to<IR::Type_Bits>();
    if (type->width_bits() > 16 || tb == nullptr) {
        return new IR::Constant(type, v);
    }
    // Constants are interned. Keys in the intern map are pairs of types and values.
    using key_t = std::tuple<int, bool, big_int>;
    static std::map<key_t, const IR::Constant*> constants;

    auto*& result = constants[{tb->width_bits(), tb->isSigned, v}];
    if (result == nullptr) {
        result = new IR::Constant(tb, v);
    }

    return result;
}

const IR::BoolLiteral* IRUtils::getBoolLiteral(bool value) {
    // Boolean literals are interned.
    static std::map<bool, const IR::BoolLiteral*> literals;

    auto*& result = literals[value];
    if (result == nullptr) {
        result = new IR::BoolLiteral(IR::Type::Boolean::get(), value);
    }
    return result;
}

const IR::TaintExpression* IRUtils::getTaintExpression(const IR::Type* type) {
    // Do not cache varbits.
    if (type->is<IR::Extracted_Varbits>()) {
        return new IR::TaintExpression(type);
    }
    // Only cache bits with width lower than 16 bit to restrict the size of the cache.
    const auto* tb = type->to<IR::Type_Bits>();
    if (type->width_bits() > 16 || tb == nullptr) {
        return new IR::TaintExpression(type);
    }
    // Taint expressions are interned. Keys in the intern map is the signedness and width of the
    // type.
    using key_t = std::tuple<int, bool>;
    static std::map<key_t, const IR::TaintExpression*> taints;

    auto*& result = taints[{tb->width_bits(), tb->isSigned}];
    if (result == nullptr) {
        result = new IR::TaintExpression(type);
    }

    return result;
}

const StateVariable& IRUtils::getConcolicMember(const IR::ConcolicVariable* var, int concolicId) {
    const auto* const concolicMember = var->concolicMember;
    auto* clonedMember = concolicMember->clone();
    clonedMember->member = std::to_string(concolicId).c_str();
    return *(new StateVariable(clonedMember));
}

const IR::Literal* IRUtils::getDefaultValue(const IR::Type* type) {
    if (type->is<IR::Type_Bits>()) {
        return getConstant(type, 0);
    }
    if (type->is<IR::Type_Boolean>()) {
        return getBoolLiteral(false);
    }
    P4C_UNIMPLEMENTED("Default value for type %s of type %s not implemented.", type,
                      type->node_type_name());
}

/* =============================================================================================
 *  Other helper functions
 * ============================================================================================= */

big_int IRUtils::getBigIntFromLiteral(const IR::Literal* l) {
    if (const auto* c = l->to<IR::Constant>()) {
        return c->value;
    }
    if (const auto* b = l->to<IR::BoolLiteral>()) {
        return b->value ? 1 : 0;
    }
    P4C_UNIMPLEMENTED("Literal %1% of type %2% not supported.", l, l->node_type_name());
}

int IRUtils::getIntFromLiteral(const IR::Literal* l) {
    if (const auto* c = l->to<IR::Constant>()) {
        if (!c->fitsInt()) {
            BUG("Value %1% too large for Int.", l);
        }
        return c->asInt();
    }
    if (const auto* b = l->to<IR::BoolLiteral>()) {
        return b->value ? 1 : 0;
    }
    P4C_UNIMPLEMENTED("Literal %1% of type %2% not supported.", l, l->node_type_name());
}

big_int IRUtils::getMaxBvVal(int bitWidth) { return pow(big_int(2), bitWidth) - 1; }

big_int IRUtils::getMaxBvVal(const IR::Type* t) {
    if (const auto* tb = t->to<IR::Type_Bits>()) {
        return tb->isSigned ? getMaxBvVal(tb->width_bits() - 1) : getMaxBvVal(tb->width_bits());
    }
    if (t->is<IR::Type_Boolean>()) {
        return 1;
    }
    P4C_UNIMPLEMENTED("Maximum value calculation for type %1% not implemented.", t);
}

big_int IRUtils::getMinBvVal(const IR::Type* t) {
    if (const auto* tb = t->to<IR::Type_Bits>()) {
        return (tb->isSigned) ? -(big_int(1) << tb->width_bits() - 1) : big_int(0);
    }
    if (t->is<IR::Type_Boolean>()) {
        return 0;
    }
    P4C_UNIMPLEMENTED("Maximum value calculation for type %1% not implemented.", t);
}

const IR::MethodCallExpression* IRUtils::generateInternalMethodCall(
    cstring methodName, const std::vector<const IR::Expression*>& argVector,
    const IR::Type* returnType) {
    auto* args = new IR::Vector<IR::Argument>();
    for (const auto* expr : argVector) {
        args->push_back(new IR::Argument(expr));
    }
    return new IR::MethodCallExpression(
        returnType,
        new IR::Member(new IR::Type_Method(new IR::ParameterList(), methodName),
                       new IR::PathExpression(new IR::Type_Extern("*"), new IR::Path("*")),
                       methodName),
        args);
}

std::vector<const IR::Expression*> IRUtils::flattenStructExpression(
    const IR::StructExpression* structExpr) {
    std::vector<const IR::Expression*> exprList;
    for (const auto* listElem : structExpr->components) {
        if (const auto* subStructExpr = listElem->expression->to<IR::StructExpression>()) {
            auto subList = flattenStructExpression(subStructExpr);
            exprList.insert(exprList.end(), subList.begin(), subList.end());
        } else {
            exprList.push_back(listElem->expression);
        }
    }
    return exprList;
}

std::vector<const IR::Expression*> IRUtils::flattenListExpression(
    const IR::ListExpression* listExpr) {
    std::vector<const IR::Expression*> exprList;
    for (const auto* listElem : listExpr->components) {
        if (const auto* subStructExpr = listElem->to<IR::StructExpression>()) {
            auto subList = flattenStructExpression(subStructExpr);
            exprList.insert(exprList.end(), subList.begin(), subList.end());
        } else {
            exprList.push_back(listElem);
        }
    }
    return exprList;
}

const IR::Constant* IRUtils::getRandConstantForWidth(int bitWidth) {
    auto maxVal = getMaxBvVal(bitWidth);
    auto randInt = TestgenUtils::getRandBigInt(maxVal);
    const auto* constType = getBitType(bitWidth);
    return getConstant(constType, randInt);
}

const IR::Constant* IRUtils::getRandConstantForType(const IR::Type_Bits* type) {
    auto maxVal = getMaxBvVal(type->width_bits());
    auto randInt = TestgenUtils::getRandBigInt(maxVal);
    return getConstant(type, randInt);
}

}  // namespace P4Tools
