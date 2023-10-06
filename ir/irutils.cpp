#include "ir/irutils.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>
#include <typeindex>
#include <vector>

#include <boost/core/enable_if.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/detail/default_ops.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>

#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/exceptions.h"

namespace IR {

/* =============================================================================================
 *  Types
 * ============================================================================================= */

const Type_Bits *getBitType(int size, bool isSigned) {
    // Types are cached already.
    return Type_Bits::get(size, isSigned);
}

const Type_Bits *getBitTypeToFit(int value) {
    // To represent a number N, we need ceil(log2(N + 1)) bits.
    int width = ceil(log2(value + 1));
    return getBitType(width);
}

/* =============================================================================================
 *  Expressions
 * ============================================================================================= */

const Constant *getConstant(const Type *type, big_int v, const Util::SourceInfo &srcInfo) {
    // Only cache bits with width lower than 16 bit to restrict the size of the cache.
    const auto *tb = type->to<Type_Bits>();
    if (type->width_bits() > 16 || tb == nullptr) {
        return new Constant(srcInfo, type, v);
    }
    // Constants are interned. Keys in the intern map are pairs of types and values.
    using key_t = std::tuple<int, std::type_index, bool, big_int>;
    static std::map<key_t, const Constant *> CONSTANTS;

    auto *&result = CONSTANTS[{tb->width_bits(), typeid(*type), tb->isSigned, v}];
    if (result == nullptr) {
        result = new Constant(srcInfo, tb, v);
    }

    return result;
}

const IR::Constant *getMaxValueConstant(const Type *t, const Util::SourceInfo &srcInfo) {
    if (t->is<Type_Bits>()) {
        return IR::getConstant(t, IR::getMaxBvVal(t), srcInfo);
    }
    if (t->is<Type_Boolean>()) {
        return IR::getConstant(IR::getBitType(1), 1, srcInfo);
    }
    P4C_UNIMPLEMENTED("Maximum value calculation for type %1% not implemented.", t);
}

const BoolLiteral *getBoolLiteral(bool value, const Util::SourceInfo &srcInfo) {
    // Boolean literals are interned.
    static std::map<bool, const BoolLiteral *> LITERALS;

    auto *&result = LITERALS[value];
    if (result == nullptr) {
        result = new BoolLiteral(srcInfo, Type::Boolean::get(), value);
    }
    return result;
}

const IR::Constant *convertBoolLiteral(const IR::BoolLiteral *lit) {
    return IR::getConstant(IR::getBitType(1), lit->value ? 1 : 0, lit->getSourceInfo());
}

const IR::Expression *getDefaultValue(const IR::Type *type, const Util::SourceInfo &srcInfo,
                                      bool valueRequired) {
    if (const auto *tb = type->to<IR::Type_Bits>()) {
        // TODO: Use getConstant.
        return new IR::Constant(srcInfo, tb, 0);
    }
    if (type->is<IR::Type_Boolean>()) {
        // TODO: Use getBoolLiteral.
        return new BoolLiteral(srcInfo, Type::Boolean::get(), false);
    }
    if (type->is<IR::Type_InfInt>()) {
        return new IR::Constant(srcInfo, 0);
    }
    if (const auto *te = type->to<IR::Type_Enum>()) {
        return new IR::Member(srcInfo, new IR::TypeNameExpression(te->name),
                              te->members.at(0)->getName());
    }
    if (const auto *te = type->to<IR::Type_SerEnum>()) {
        return new IR::Cast(srcInfo, type->getP4Type(), new IR::Constant(srcInfo, te->type, 0));
    }
    if (const auto *te = type->to<IR::Type_Error>()) {
        return new IR::Member(srcInfo, new IR::TypeNameExpression(te->name), "NoError");
    }
    if (type->is<IR::Type_String>()) {
        return new IR::StringLiteral(srcInfo, cstring(""));
    }
    if (type->is<IR::Type_Varbits>()) {
        if (valueRequired) {
            P4C_UNIMPLEMENTED("%1%: No default value for varbit types.", srcInfo);
        }
        ::error(ErrorType::ERR_UNSUPPORTED, "%1% default values for varbit types", srcInfo);
        return nullptr;
    }
    if (const auto *ht = type->to<IR::Type_Header>()) {
        return new IR::InvalidHeader(ht->getP4Type());
    }
    if (const auto *hu = type->to<IR::Type_HeaderUnion>()) {
        return new IR::InvalidHeaderUnion(hu->getP4Type());
    }
    if (const auto *st = type->to<IR::Type_StructLike>()) {
        auto *vec = new IR::IndexedVector<IR::NamedExpression>();
        for (const auto *field : st->fields) {
            const auto *value = getDefaultValue(field->type, srcInfo);
            if (value == nullptr) {
                return nullptr;
            }
            vec->push_back(new IR::NamedExpression(field->name, value));
        }
        const auto *resultType = st->getP4Type();
        return new IR::StructExpression(srcInfo, resultType, resultType, *vec);
    }
    if (const auto *tf = type->to<IR::Type_Fragment>()) {
        return getDefaultValue(tf->type, srcInfo);
    }
    if (const auto *tt = type->to<IR::Type_BaseList>()) {
        auto *vec = new IR::Vector<IR::Expression>();
        for (const auto *field : tt->components) {
            const auto *value = getDefaultValue(field, srcInfo);
            if (value == nullptr) {
                return nullptr;
            }
            vec->push_back(value);
        }
        return new IR::ListExpression(srcInfo, *vec);
    }
    if (const auto *ts = type->to<IR::Type_Stack>()) {
        auto *vec = new IR::Vector<IR::Expression>();
        const auto *elementType = ts->elementType;
        for (size_t i = 0; i < ts->getSize(); i++) {
            const IR::Expression *invalid = nullptr;
            if (elementType->is<IR::Type_Header>()) {
                invalid = new IR::InvalidHeader(elementType->getP4Type());
            } else {
                BUG_CHECK(elementType->is<IR::Type_HeaderUnion>(),
                          "%1%: expected a header or header union stack", elementType);
                invalid = new IR::InvalidHeaderUnion(srcInfo, elementType->getP4Type());
            }
            vec->push_back(invalid);
        }
        const auto *resultType = ts->getP4Type();
        return new IR::HeaderStackExpression(srcInfo, resultType, *vec, resultType);
    }
    if (valueRequired) {
        P4C_UNIMPLEMENTED("%1%: No default value for type %2% (%3%).", srcInfo, type,
                          type->node_type_name());
    }
    ::error(ErrorType::ERR_INVALID, "%1%: No default value for type %2% (%3%)", srcInfo, type,
            type->node_type_name());
    return nullptr;
}

std::vector<const Expression *> flattenStructExpression(const StructExpression *structExpr) {
    std::vector<const Expression *> exprList;
    for (const auto *listElem : structExpr->components) {
        if (const auto *subStructExpr = listElem->expression->to<StructExpression>()) {
            auto subList = flattenStructExpression(subStructExpr);
            exprList.insert(exprList.end(), subList.begin(), subList.end());
        } else if (const auto *headerStackExpr =
                       listElem->expression->to<HeaderStackExpression>()) {
            for (const auto *headerStackElem : headerStackExpr->components) {
                // We assume there are no nested header stacks.
                auto subList =
                    flattenStructExpression(headerStackElem->checkedTo<IR::StructExpression>());
                exprList.insert(exprList.end(), subList.begin(), subList.end());
            }
        } else {
            exprList.emplace_back(listElem->expression);
        }
    }
    return exprList;
}

std::vector<const Expression *> flattenListExpression(const ListExpression *listExpr) {
    std::vector<const Expression *> exprList;
    for (const auto *listElem : listExpr->components) {
        if (const auto *subListExpr = listElem->to<ListExpression>()) {
            auto subList = flattenListExpression(subListExpr);
            exprList.insert(exprList.end(), subList.begin(), subList.end());
        } else {
            exprList.emplace_back(listElem);
        }
    }
    return exprList;
}

/* =============================================================================================
 *  Other helper functions
 * ============================================================================================= */

big_int getBigIntFromLiteral(const Literal *l) {
    if (const auto *c = l->to<Constant>()) {
        return c->value;
    }
    if (const auto *b = l->to<BoolLiteral>()) {
        return b->value ? 1 : 0;
    }
    P4C_UNIMPLEMENTED("Literal %1% of type %2% not supported.", l, l->node_type_name());
}

int getIntFromLiteral(const Literal *l) {
    if (const auto *c = l->to<Constant>()) {
        if (!c->fitsInt()) {
            BUG("Value %1% too large for Int.", l);
        }
        return c->asInt();
    }
    if (const auto *b = l->to<BoolLiteral>()) {
        return b->value ? 1 : 0;
    }
    P4C_UNIMPLEMENTED("Literal %1% of type %2% not supported.", l, l->node_type_name());
}

big_int getMaxBvVal(int bitWidth) { return pow(big_int(2), bitWidth) - 1; }

big_int getMaxBvVal(const Type *t) {
    if (const auto *tb = t->to<Type_Bits>()) {
        return tb->isSigned ? getMaxBvVal(tb->width_bits() - 1) : getMaxBvVal(tb->width_bits());
    }
    if (t->is<Type_Boolean>()) {
        return 1;
    }
    P4C_UNIMPLEMENTED("Maximum value calculation for type %1% not implemented.", t);
}

big_int getMinBvVal(const Type *t) {
    if (const auto *tb = t->to<Type_Bits>()) {
        return (tb->isSigned) ? -(big_int(1) << tb->width_bits() - 1) : big_int(0);
    }
    if (t->is<Type_Boolean>()) {
        return 0;
    }
    P4C_UNIMPLEMENTED("Maximum value calculation for type %1% not implemented.", t);
}

}  // namespace IR
