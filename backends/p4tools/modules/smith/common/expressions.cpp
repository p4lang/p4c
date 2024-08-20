#include "backends/p4tools/modules/smith/common/expressions.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/probabilities.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/core/target.h"
#include "ir/indexed_vector.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/exceptions.h"

namespace P4::P4Tools::P4Smith {

const IR::Type_Boolean *ExpressionGenerator::genBoolType() { return IR::Type_Boolean::get(); }

const IR::Type_InfInt *ExpressionGenerator::genIntType() { return IR::Type_InfInt::get(); }

const IR::Type *ExpressionGenerator::pickRndBaseType(const std::vector<int64_t> &type_probs) const {
    if (type_probs.size() != 7) {
        BUG("pickRndBaseType: Type probabilities must be exact");
    }
    const IR::Type *tb = nullptr;
    switch (Utils::getRandInt(type_probs)) {
        case 0: {
            // bool
            tb = genBoolType();
            break;
        }
        case 1: {
            // error, this is not supported right now
            break;
        }
        case 2: {
            // int, this is not supported right now
            tb = genIntType();
            break;
        }
        case 3: {
            // string, this is not supported right now
            break;
        }
        case 4: {
            // bit<>
            tb = genBitType(false);
            break;
        }
        case 5: {
            // int<>
            tb = genBitType(true);
            break;
        }
        case 6: {
            // varbit<>, this is not supported right now
            break;
        }
    }
    return tb;
}

const IR::Type *ExpressionGenerator::pickRndType(TyperefProbs type_probs) {
    const std::vector<int64_t> &typeProbsVector = {
        type_probs.p4_bit,          type_probs.p4_signed_bit, type_probs.p4_varbit,
        type_probs.p4_int,          type_probs.p4_error,      type_probs.p4_bool,
        type_probs.p4_string,       type_probs.p4_enum,       type_probs.p4_header,
        type_probs.p4_header_stack, type_probs.p4_struct,     type_probs.p4_header_union,
        type_probs.p4_tuple,        type_probs.p4_void,       type_probs.p4_match_kind};

    const std::vector<int64_t> &basetypeProbs = {
        type_probs.p4_bool, type_probs.p4_error,      type_probs.p4_int,   type_probs.p4_string,
        type_probs.p4_bit,  type_probs.p4_signed_bit, type_probs.p4_varbit};

    if (typeProbsVector.size() != 15) {
        BUG("pickRndType: Type probabilities must be exact");
    }
    const IR::Type *tp = nullptr;
    size_t idx = Utils::getRandInt(typeProbsVector);
    switch (idx) {
        case 0: {
            // bit<>
            tp = genBitType(false);
            break;
        }
        case 1: {
            // int<>
            tp = genBitType(true);
            break;
        }
        case 2: {
            // varbit<>, this is not supported right now
            break;
        }
        case 3: {
            tp = genIntType();
            break;
        }
        case 4: {
            // error, this is not supported right now
            break;
        }
        case 5: {
            // bool
            tp = genBoolType();
            break;
        }
        case 6: {
            // string, this is not supported right now
            break;
        }
        case 7: {
            // enum, this is not supported right now
            break;
        }
        case 8: {
            // header
            auto lTypes = P4Scope::getDecls<IR::Type_Header>();
            if (lTypes.empty()) {
                tp = pickRndBaseType(basetypeProbs);
                break;
            }
            const auto *candidateType = lTypes.at(Utils::getRandInt(0, lTypes.size() - 1));
            auto typeName = candidateType->name.name;
            // check if struct is forbidden
            if (P4Scope::notInitializedStructs.count(typeName) == 0) {
                tp = new IR::Type_Name(candidateType->name.name);
            } else {
                tp = pickRndBaseType(basetypeProbs);
            }
            break;
        }
        case 9: {
            tp = target().declarationGenerator().genHeaderStackType();
            break;
        }
        case 10: {
            // struct
            auto lTypes = P4Scope::getDecls<IR::Type_Struct>();
            if (lTypes.empty()) {
                tp = pickRndBaseType(basetypeProbs);
                break;
            }
            const auto *candidateType = lTypes.at(Utils::getRandInt(0, lTypes.size() - 1));
            auto typeName = candidateType->name.name;
            // check if struct is forbidden
            if (P4Scope::notInitializedStructs.count(typeName) == 0) {
                tp = new IR::Type_Name(candidateType->name.name);
            } else {
                tp = pickRndBaseType(basetypeProbs);
            }
            break;
        }
        case 11: {
            // header union, this is not supported right now
            break;
        }
        case 12: {
            // tuple, this is not supported right now
            break;
        }
        case 13: {
            // void
            tp = IR::Type_Void::get();
            break;
        }
        case 14: {
            // match kind, this is not supported right now
            break;
        }
    }
    if (tp == nullptr) {
        BUG("pickRndType: Chosen type is Null!");
    }

    return tp;
}

IR::BoolLiteral *ExpressionGenerator::genBoolLiteral() {
    if (Utils::getRandInt(0, 1) != 0) {
        return new IR::BoolLiteral(false);
    }
    return new IR::BoolLiteral(true);
}

const IR::Type_Bits *ExpressionGenerator::genBitType(bool isSigned) const {
    auto bitWidths = availableBitWidths();
    BUG_CHECK(!bitWidths.empty(), "No available bit widths");
    auto size = Utils::getRandInt(0, bitWidths.size() - 1);

    return IR::Type_Bits::get(bitWidths.at(size), isSigned);
}

IR::Constant *ExpressionGenerator::genIntLiteral(size_t bit_width) {
    big_int min = -((big_int(1) << bit_width - 1));
    if (P4Scope::req.not_negative) {
        min = 0;
    }
    big_int max = ((big_int(1) << bit_width - 1) - 1);
    big_int value = Utils::getRandBigInt(min, max);
    while (true) {
        if (P4Scope::req.not_zero && value == 0) {
            value = Utils::getRandBigInt(min, max);
            // retry until we generate a value that is !zero
            continue;
        }
        break;
    }
    return new IR::Constant(value);
}
IR::Constant *ExpressionGenerator::genBitLiteral(const IR::Type *tb) {
    big_int maxSize = IR::getMaxBvVal(tb->width_bits());
    big_int value;
    if (maxSize == 0) {
        value = 0;
        return new IR::Constant(tb, value);
    }
    return new IR::Constant(tb, Utils::getRandBigInt(P4Scope::req.not_zero ? 1 : 0, maxSize));
}

IR::Expression *ExpressionGenerator::genExpression(const IR::Type *tp) {
    IR::Expression *expr = nullptr;

    // reset the expression depth
    P4Scope::prop.depth = 0;

    if (const auto *tb = tp->to<IR::Type_Bits>()) {
        expr = constructBitExpr(tb);
    } else if (tp->is<IR::Type_InfInt>()) {
        expr = constructIntExpr();
    } else if (tp->is<IR::Type_Boolean>()) {
        expr = constructBooleanExpr();
    } else if (tp->is<IR::Type_Typedef>()) {
        expr = genExpression(tp->to<IR::Type_Typedef>()->type);
        // Generically perform explicit castings to all cases.
        const auto *explicitType = new IR::Type_Name(IR::ID(tp->to<IR::Type_Typedef>()->name));
        expr = new IR::Cast(explicitType, expr);
    } else if (const auto *enumType = tp->to<IR::Type_Enum>()) {
        if (enumType->members.empty()) {
            BUG("Expression: Enum %s has no members", enumType->name.name);
        }
        const auto *enumChoice =
            enumType->members.at(Utils::getRandInt(enumType->members.size() - 1));
        expr = new IR::Member(enumType,
                              new IR::PathExpression(enumType, new IR::Path(enumType->getName())),
                              enumChoice->getName());
    } else if (const auto *tn = tp->to<IR::Type_Name>()) {
        expr = constructStructExpr(tn);
    } else {
        BUG("Expression: Type %s not yet supported", tp->node_type_name());
    }
    // reset the expression depth, just to be safe...
    P4Scope::prop.depth = 0;
    return expr;
}

IR::MethodCallExpression *ExpressionGenerator::pickFunction(
    IR::IndexedVector<IR::Declaration> viable_functions, const IR::Type **ret_type) {
    // TODO(fruffy): Make this more sophisticated
    if (viable_functions.empty() || P4Scope::req.compile_time_known) {
        return nullptr;
    }

    size_t idx = Utils::getRandInt(0, viable_functions.size() - 1);
    cstring funName;
    const IR::ParameterList *params = nullptr;
    if (const auto *p4Fun = viable_functions[idx]->to<IR::Function>()) {
        funName = p4Fun->name.name;
        params = p4Fun->getParameters();
        *ret_type = p4Fun->type->returnType;
    } else if (const auto *p4Extern = viable_functions[idx]->to<IR::Method>()) {
        funName = p4Extern->name.name;
        params = p4Extern->getParameters();
        *ret_type = p4Extern->type->returnType;
    } else {
        BUG("Unknown callable: Type %s not yet supported", viable_functions[idx]->node_type_name());
    }
    auto *expr = genFunctionCall(funName, *params);
    // sometimes, functions may !be callable
    // because we do !have the right return values
    if ((expr == nullptr) || (ret_type == nullptr)) {
        return nullptr;
    }
    return expr;
}

IR::MethodCallExpression *ExpressionGenerator::genFunctionCall(cstring method_name,
                                                               IR::ParameterList params) {
    auto *args = new IR::Vector<IR::Argument>();
    IR::IndexedVector<IR::StatOrDecl> decls;

    bool canCall = true;

    for (const auto *par : params) {
        if (!checkInputArg(par)) {
            canCall = false;
        } else {
            IR::Argument *arg = nullptr;
            arg = new IR::Argument(genInputArg(par));
            args->push_back(arg);
        }
    }
    if (canCall) {
        auto *pathExpr = new IR::PathExpression(method_name);
        return new IR::MethodCallExpression(pathExpr, args);
    }
    return nullptr;
}

IR::ListExpression *ExpressionGenerator::genExpressionList(IR::Vector<IR::Type> types,
                                                           bool only_lval) {
    IR::Vector<IR::Expression> components;
    for (const auto *tb : types) {
        IR::Expression *expr = nullptr;
        if (only_lval) {
            cstring lvalName = P4Scope::pickLval(tb);
            expr = new IR::PathExpression(lvalName);
        } else {
            expr = genExpression(tb);
        }
        components.push_back(expr);
    }
    return new IR::ListExpression(components);
}

IR::Expression *ExpressionGenerator::constructUnaryExpr(const IR::Type_Bits *tb) {
    IR::Expression *expr = nullptr;

    if (P4Scope::prop.depth > MAX_DEPTH) {
        return genBitLiteral(tb);
    }
    P4Scope::prop.depth++;

    // we want to avoid negation when we require no negative values
    int64_t negPct = Probabilities::get().EXPRESSION_BIT_UNARY_NEG;
    if (P4Scope::req.not_negative) {
        negPct = 0;
    }

    std::vector<int64_t> percent = {negPct, Probabilities::get().EXPRESSION_BIT_UNARY_CMPL,
                                    Probabilities::get().EXPRESSION_BIT_UNARY_CAST,
                                    Probabilities::get().EXPRESSION_BIT_UNARY_FUNCTION};

    switch (Utils::getRandInt(percent)) {
        case 0: {
            // pick a negation that matches the type
            expr = new IR::Neg(tb, constructBitExpr(tb));
        } break;
        case 1: {
            // pick a complement that matches the type
            // width must be known so we cast
            expr = constructBitExpr(tb);
            if (P4Scope::prop.width_unknown) {
                expr = new IR::Cast(tb, expr);
                P4Scope::prop.width_unknown = false;
            }
            expr = new IR::Cmpl(tb, expr);
        } break;
        case 2: {
            // pick a cast to the type that matches the type
            // new bit type can be random here
            expr = new IR::Cast(tb, constructBitExpr(tb));
        } break;
        case 3: {
            auto p4Functions = P4Scope::getDecls<IR::Function>();
            auto p4Externs = P4Scope::getDecls<IR::Method>();

            IR::IndexedVector<IR::Declaration> viableFunctions;
            for (const auto *fun : p4Functions) {
                if (fun->type->returnType->to<IR::Type_Bits>() != nullptr) {
                    viableFunctions.push_back(fun);
                }
            }
            for (const auto *fun : p4Externs) {
                if (fun->type->returnType->to<IR::Type_Bits>() != nullptr) {
                    viableFunctions.push_back(fun);
                }
            }
            const IR::Type *retType = nullptr;
            expr = pickFunction(viableFunctions, &retType);
            // can !find a suitable function, generate a default value
            if (expr == nullptr) {
                expr = genBitLiteral(tb);
                break;
            }
            // if the return value does !match try to cast it
            if (retType != tb) {
                expr = new IR::Cast(tb, expr);
            }
        } break;
    }
    return expr;
}

IR::Expression *ExpressionGenerator::createSaturationOperand(const IR::Type_Bits *tb) {
    IR::Expression *expr = constructBitExpr(tb);

    int width = P4Scope::constraints.max_phv_container_width;
    if (width != 0) {
        if (tb->width_bits() > width) {
            const auto *type = IR::Type_Bits::get(width, false);
            expr = new IR::Cast(type, expr);
            expr->type = type;
            P4Scope::prop.width_unknown = false;
            return expr;
        }
    }

    // width must be known so we cast
    if (P4Scope::prop.width_unknown) {
        expr = new IR::Cast(tb, expr);
        P4Scope::prop.width_unknown = false;
    }

    expr->type = tb;
    return expr;
}

IR::Expression *ExpressionGenerator::constructBinaryBitExpr(const IR::Type_Bits *tb) {
    IR::Expression *expr = nullptr;

    if (P4Scope::prop.depth > MAX_DEPTH) {
        return genBitLiteral(tb);
    }
    P4Scope::prop.depth++;

    auto pctSub = Probabilities::get().EXPRESSION_BIT_BINARY_SUB;
    auto pctSubsat = Probabilities::get().EXPRESSION_BIT_BINARY_SUBSAT;
    // we want to avoid subtraction when we require no negative values
    if (P4Scope::req.not_negative) {
        pctSub = 0;
        pctSubsat = 0;
    }

    std::vector<int64_t> percent = {Probabilities::get().EXPRESSION_BIT_BINARY_MUL,
                                    Probabilities::get().EXPRESSION_BIT_BINARY_DIV,
                                    Probabilities::get().EXPRESSION_BIT_BINARY_MOD,
                                    Probabilities::get().EXPRESSION_BIT_BINARY_ADD,
                                    pctSub,
                                    Probabilities::get().EXPRESSION_BIT_BINARY_ADDSAT,
                                    pctSubsat,
                                    Probabilities::get().EXPRESSION_BIT_BINARY_LSHIFT,
                                    Probabilities::get().EXPRESSION_BIT_BINARY_RSHIFT,
                                    Probabilities::get().EXPRESSION_BIT_BINARY_BAND,
                                    Probabilities::get().EXPRESSION_BIT_BINARY_BOR,
                                    Probabilities::get().EXPRESSION_BIT_BINARY_BXOR,
                                    Probabilities::get().EXPRESSION_BIT_BINARY_CONCAT};

    switch (Utils::getRandInt(percent)) {
        case 0: {
            IR::Expression *left = constructBitExpr(tb);
            IR::Expression *right = constructBitExpr(tb);
            // pick a multiplication that matches the type
            expr = new IR::Mul(tb, left, right);
        } break;
        case 1: {
            // pick a division that matches the type
            // TODO(fruffy): Make more sophisticated
            // this requires only compile time known values
            IR::Expression *left = genBitLiteral(tb);
            P4Scope::req.not_zero = true;
            IR::Expression *right = genBitLiteral(tb);
            P4Scope::req.not_zero = false;
            expr = new IR::Div(tb, left, right);
        } break;
        case 2: {
            // pick a modulo that matches the type
            // TODO(fruffy): Make more sophisticated
            // this requires only compile time known values
            IR::Expression *left = genBitLiteral(tb);
            P4Scope::req.not_zero = true;
            IR::Expression *right = genBitLiteral(tb);
            P4Scope::req.not_zero = false;
            expr = new IR::Mod(tb, left, right);
        } break;
        case 3: {
            IR::Expression *left = constructBitExpr(tb);
            IR::Expression *right = constructBitExpr(tb);
            // pick an addition that matches the type
            expr = new IR::Add(tb, left, right);
        } break;
        case 4: {
            IR::Expression *left = constructBitExpr(tb);
            IR::Expression *right = constructBitExpr(tb);
            // pick a subtraction that matches the type
            expr = new IR::Sub(tb, left, right);
        } break;
        case 5: {
            IR::Expression *left = createSaturationOperand(tb);
            IR::Expression *right = createSaturationOperand(tb);
            // pick a saturating addition that matches the type
            BUG_CHECK(left->type->width_bits() == right->type->width_bits(),
                      "Operator must have same left and right types: %s, %s", left->type,
                      right->type);
            expr = new IR::AddSat(tb, left, right);
            // If we ended up constraining the operand width, the expr might not be the correct
            // type.
            if (left->type != tb) {
                expr = new IR::Cast(tb, expr);
            }
        } break;
        case 6: {
            IR::Expression *left = createSaturationOperand(tb);
            IR::Expression *right = createSaturationOperand(tb);
            // pick a saturating addition that matches the type
            BUG_CHECK(left->type->width_bits() == right->type->width_bits(),
                      "Operator must have same left and right types: %s, %s", left->type,
                      right->type);
            expr = new IR::SubSat(tb, left, right);
            // If we ended up constraining the operand width, the expr might not be the correct
            // type.
            if (left->type != tb) {
                expr = new IR::Cast(tb, expr);
            }
        } break;
        case 7: {
            // width must be known so we cast
            IR::Expression *left = constructBitExpr(tb);
            if (P4Scope::prop.width_unknown) {
                left = new IR::Cast(tb, left);
                P4Scope::prop.width_unknown = false;
            }
            // TODO(fruffy): Make this more sophisticated,
            P4Scope::req.not_negative = true;
            IR::Expression *right = constructBitExpr(tb);
            P4Scope::req.not_negative = false;
            // TODO(fruffy): Make this more sophisticated
            // shifts are limited to 8 bits
            if (P4Scope::constraints.const_lshift_count) {
                right = genBitLiteral(IR::Type_Bits::get(P4Scope::req.shift_width, false));
            } else {
                right = new IR::Cast(IR::Type_Bits::get(8, false), right);
            }
            // pick a left-shift that matches the type
            expr = new IR::Shl(tb, left, right);
        } break;
        case 8: {
            // width must be known so we cast
            IR::Expression *left = constructBitExpr(tb);
            if (P4Scope::prop.width_unknown) {
                left = new IR::Cast(tb, left);
                P4Scope::prop.width_unknown = false;
            }

            // TODO(fruffy): Make this more sophisticated,
            P4Scope::req.not_negative = true;
            IR::Expression *right = constructBitExpr(tb);
            P4Scope::req.not_negative = false;
            // shifts are limited to 8 bits
            right = new IR::Cast(IR::Type_Bits::get(8, false), right);
            // pick a right-shift that matches the type
            expr = new IR::Shr(tb, left, right);
        } break;
        case 9: {
            IR::Expression *left = constructBitExpr(tb);
            IR::Expression *right = constructBitExpr(tb);
            // pick an binary And that matches the type
            expr = new IR::BAnd(tb, left, right);
        } break;
        case 10: {
            IR::Expression *left = constructBitExpr(tb);
            IR::Expression *right = constructBitExpr(tb);
            // pick a binary Or and that matches the type
            expr = new IR::BOr(tb, left, right);
        } break;
        case 11: {
            IR::Expression *left = constructBitExpr(tb);
            IR::Expression *right = constructBitExpr(tb);
            // pick an binary Xor that matches the type
            expr = new IR::BXor(tb, left, right);
        } break;
        case 12: {
            // pick an concatenation that matches the type
            size_t typeWidth = tb->width_bits();
            size_t split = Utils::getRandInt(1, typeWidth - 1);
            // TODO(fruffy): lazy fallback
            if (split >= typeWidth) {
                return genBitLiteral(tb);
            }
            const auto *tl = IR::Type_Bits::get(typeWidth - split, false);
            const auto *tr = IR::Type_Bits::get(split, false);
            // width must be known so we cast
            printf("Concat: %s, %s\n", tl->toString().c_str(), tr->toString().c_str());
            IR::Expression *left = constructBitExpr(tl);
            if (P4Scope::prop.width_unknown) {
                left = new IR::Cast(tl, left);
                P4Scope::prop.width_unknown = false;
            }
            IR::Expression *right = constructBitExpr(tr);
            if (P4Scope::prop.width_unknown) {
                right = new IR::Cast(tr, right);
                P4Scope::prop.width_unknown = false;
            }
            expr = new IR::Concat(tb, left, right);
        } break;
    }
    return expr;
}

IR::Expression *ExpressionGenerator::constructTernaryBitExpr(const IR::Type_Bits *tb) {
    IR::Expression *expr = nullptr;

    if (P4Scope::prop.depth > MAX_DEPTH) {
        return genBitLiteral(tb);
    }
    P4Scope::prop.depth++;

    std::vector<int64_t> percent = {Probabilities::get().EXPRESSION_BIT_BINARY_SLICE,
                                    Probabilities::get().EXPRESSION_BIT_BINARY_MUX};

    switch (Utils::getRandInt(percent)) {
        case 0: {
            // TODO(fruffy): Refine this...
            // pick a slice that matches the type
            auto typeWidth = tb->width_bits();
            // TODO(fruffy): this is some arbitrary value...
            auto newTypeSize = Utils::getRandInt(typeWidth, P4Scope::constraints.max_bitwidth);
            const auto *sliceType = IR::Type_Bits::get(newTypeSize, false);
            auto *sliceExpr = constructBitExpr(sliceType);
            if (P4Scope::prop.width_unknown) {
                sliceExpr = new IR::Cast(sliceType, sliceExpr);
                P4Scope::prop.width_unknown = false;
            }
            auto margin = newTypeSize - typeWidth;
            auto high = Utils::getRandInt(0, margin) + typeWidth - 1;
            auto low = high - typeWidth + 1;
            expr = new IR::Slice(sliceExpr, high, low);
            break;
        }
        case 1: {
            // pick a mux that matches the type
            IR::Expression *cond = constructBooleanExpr();
            IR::Expression *left = constructBitExpr(tb);
            if (P4Scope::prop.width_unknown) {
                left = new IR::Cast(tb, left);
                P4Scope::prop.width_unknown = false;
            }
            IR::Expression *right = constructBitExpr(tb);
            if (P4Scope::prop.width_unknown) {
                right = new IR::Cast(tb, right);
                P4Scope::prop.width_unknown = false;
            }
            expr = new IR::Mux(tb, cond, left, right);
        } break;
    }
    return expr;
}

IR::Expression *ExpressionGenerator::pickBitVar(const IR::Type_Bits *tb) {
    cstring nodeName = tb->node_type_name();
    auto availBitTypes = P4Scope::lvalMap[nodeName].size();
    if (P4Scope::checkLval(tb)) {
        cstring name = P4Scope::pickLval(tb);
        return new IR::PathExpression(name);
    }
    if (availBitTypes > 0) {
        // even if we do !find anything we can still cast other bits
        auto *newTb = P4Scope::pickDeclaredBitType();
        cstring name = P4Scope::pickLval(newTb);
        return new IR::Cast(tb, new IR::PathExpression(name));
    }

    // fallback, just generate a literal
    return genBitLiteral(tb);
}

IR::Expression *ExpressionGenerator::constructBitExpr(const IR::Type_Bits *tb) {
    IR::Expression *expr = nullptr;

    std::vector<int64_t> percent = {Probabilities::get().EXPRESSION_BIT_VAR,
                                    Probabilities::get().EXPRESSION_BIT_INT_LITERAL,
                                    Probabilities::get().EXPRESSION_BIT_BIT_LITERAL,
                                    Probabilities::get().EXPRESSION_BIT_UNARY,
                                    Probabilities::get().EXPRESSION_BIT_BINARY,
                                    Probabilities::get().EXPRESSION_BIT_TERNARY};

    switch (Utils::getRandInt(percent)) {
        case 0: {
            // pick a variable that matches the type
            // do !pick, if the requirement is to be a compile time known value
            // TODO(fruffy): This is lazy, we can easily check
            if (P4Scope::req.compile_time_known) {
                expr = genBitLiteral(tb);
            } else {
                expr = pickBitVar(tb);
            }
        } break;
        case 1: {
            // pick an int literal, if allowed
            if (P4Scope::req.require_scalar) {
                expr = genBitLiteral(tb);
            } else {
                expr = constructIntExpr();
                P4Scope::prop.width_unknown = true;
            }
        } break;
        case 2: {
            // pick a bit literal that matches the type
            expr = genBitLiteral(tb);
        } break;
        case 3: {
            // pick a unary expression that matches the type
            expr = constructUnaryExpr(tb);
        } break;
        case 4: {
            // pick a binary expression that matches the type
            expr = constructBinaryBitExpr(tb);
        } break;
        case 5: {
            // pick a ternary expression that matches the type
            expr = constructTernaryBitExpr(tb);
        } break;
    }
    return expr;
}

IR::Expression *ExpressionGenerator::constructCmpExpr() {
    IR::Expression *expr = nullptr;

    // Generate some random type. Can be either bits, int, bool, or structlike
    // For now it is just bits.
    auto newTypeSize = Utils::getRandInt(1, P4Scope::constraints.max_bitwidth);
    const auto *newType = IR::Type_Bits::get(newTypeSize, false);
    IR::Expression *left = constructBitExpr(newType);
    IR::Expression *right = constructBitExpr(newType);

    std::vector<int64_t> percent = {Probabilities::get().EXPRESSION_BOOLEAN_CMP_EQU,
                                    Probabilities::get().EXPRESSION_BOOLEAN_CMP_NEQ};

    switch (Utils::getRandInt(percent)) {
        case 0: {
            expr = new IR::Equ(left, right);
            // pick an equals that matches the type
        } break;
        case 1: {
            expr = new IR::Neq(left, right);
            // pick a not-equals that matches the type
        } break;
    }
    return expr;
}

IR::Expression *ExpressionGenerator::constructBooleanExpr() {
    IR::Expression *expr = nullptr;
    IR::Expression *left = nullptr;
    IR::Expression *right = nullptr;

    std::vector<int64_t> percent = {Probabilities::get().EXPRESSION_BOOLEAN_VAR,
                                    Probabilities::get().EXPRESSION_BOOLEAN_LITERAL,
                                    Probabilities::get().EXPRESSION_BOOLEAN_NOT,
                                    Probabilities::get().EXPRESSION_BOOLEAN_LAND,
                                    Probabilities::get().EXPRESSION_BOOLEAN_LOR,
                                    Probabilities::get().EXPRESSION_BOOLEAN_CMP,
                                    Probabilities::get().EXPRESSION_BOOLEAN_FUNCTION,
                                    Probabilities::get().EXPRESSION_BOOLEAN_BUILT_IN};

    switch (Utils::getRandInt(percent)) {
        case 0: {
            const auto *tb = IR::Type_Boolean::get();
            // TODO(fruffy): This is lazy, we can easily check
            if (P4Scope::req.compile_time_known) {
                expr = genBoolLiteral();
                break;
            }
            if (P4Scope::checkLval(tb)) {
                cstring name = P4Scope::pickLval(tb);
                expr = new IR::TypeNameExpression(name);
            } else {
                expr = genBoolLiteral();
            }
        } break;
        case 1: {
            // pick a boolean literal
            expr = genBoolLiteral();
        } break;
        case 2: {
            // pick a Not expression
            expr = new IR::LNot(constructBooleanExpr());
        } break;
        case 3: {
            // pick an And expression
            left = constructBooleanExpr();
            right = constructBooleanExpr();
            expr = new IR::LAnd(left, right);
        } break;
        case 4: {
            // pick an Or expression
            left = constructBooleanExpr();
            right = constructBooleanExpr();
            expr = new IR::LOr(left, right);
        } break;
        case 5: {
            // pick a comparison
            expr = constructCmpExpr();
        } break;
        case 6: {
            auto p4Functions = P4Scope::getDecls<IR::Function>();
            auto p4Externs = P4Scope::getDecls<IR::Method>();

            IR::IndexedVector<IR::Declaration> viableFunctions;
            for (const auto *fun : p4Functions) {
                if (fun->type->returnType->to<IR::Type_Boolean>() != nullptr) {
                    viableFunctions.push_back(fun);
                }
            }
            for (const auto *fun : p4Externs) {
                if (fun->type->returnType->to<IR::Type_Boolean>() != nullptr) {
                    viableFunctions.push_back(fun);
                }
            }
            const IR::Type *retType = nullptr;
            expr = pickFunction(viableFunctions, &retType);
            // can !find a suitable function, generate a default value
            if (expr == nullptr) {
                expr = genBoolLiteral();
            }
        } break;
        case 7: {
            // get the expression
            auto *tblSet = P4Scope::getCallableTables();

            // just generate a literal if there are no tables left
            if (tblSet->empty() || P4Scope::req.compile_time_known) {
                expr = genBoolLiteral();
                break;
            }
            auto idx = Utils::getRandInt(0, tblSet->size() - 1);
            auto tblIter = std::begin(*tblSet);

            std::advance(tblIter, idx);
            const IR::P4Table *tbl = *tblIter;
            expr = new IR::Member(new IR::MethodCallExpression(
                                      new IR::Member(new IR::PathExpression(tbl->name), "apply")),
                                  "hit");
            tblSet->erase(tblIter);
        }
    }
    return expr;
}

IR::Expression *ExpressionGenerator::constructUnaryIntExpr() {
    IR::Expression *expr = nullptr;

    if (P4Scope::prop.depth > MAX_DEPTH) {
        return genIntLiteral();
    }
    const auto *tp = IR::Type_InfInt::get();
    P4Scope::prop.depth++;

    // we want to avoid negation when we require no negative values
    int64_t negPct = Probabilities::get().EXPRESSION_INT_UNARY_NEG;
    if (P4Scope::req.not_negative) {
        negPct = 0;
    }

    std::vector<int64_t> percent = {negPct, Probabilities::get().EXPRESSION_INT_UNARY_FUNCTION};

    switch (Utils::getRandInt(percent)) {
        case 0: {
            // pick a negation that matches the type
            expr = new IR::Neg(tp, constructIntExpr());
        } break;
        case 1: {
            auto p4Functions = P4Scope::getDecls<IR::Function>();
            auto p4Externs = P4Scope::getDecls<IR::Method>();

            IR::IndexedVector<IR::Declaration> viableFunctions;
            for (const auto *fun : p4Functions) {
                if (fun->type->returnType->to<IR::Type_InfInt>() != nullptr) {
                    viableFunctions.push_back(fun);
                }
            }
            for (const auto *fun : p4Externs) {
                if (fun->type->returnType->to<IR::Type_InfInt>() != nullptr) {
                    viableFunctions.push_back(fun);
                }
            }
            const IR::Type *retType = nullptr;
            expr = pickFunction(viableFunctions, &retType);
            // can !find a suitable function, generate a default value
            if (expr == nullptr) {
                expr = genIntLiteral();
                break;
            }
        } break;
    }
    return expr;
}

IR::Expression *ExpressionGenerator::constructBinaryIntExpr() {
    IR::Expression *expr = nullptr;
    if (P4Scope::prop.depth > MAX_DEPTH) {
        return genIntLiteral();
    }
    const auto *tp = IR::Type_InfInt::get();
    P4Scope::prop.depth++;

    auto pctSub = Probabilities::get().EXPRESSION_INT_BINARY_SUB;
    // we want to avoid subtraction when we require no negative values
    if (P4Scope::req.not_negative) {
        pctSub = 0;
    }

    std::vector<int64_t> percent = {Probabilities::get().EXPRESSION_INT_BINARY_MUL,
                                    Probabilities::get().EXPRESSION_INT_BINARY_DIV,
                                    Probabilities::get().EXPRESSION_INT_BINARY_MOD,
                                    Probabilities::get().EXPRESSION_INT_BINARY_ADD,
                                    pctSub,
                                    Probabilities::get().EXPRESSION_INT_BINARY_LSHIFT,
                                    Probabilities::get().EXPRESSION_INT_BINARY_RSHIFT,
                                    Probabilities::get().EXPRESSION_INT_BINARY_BAND,
                                    Probabilities::get().EXPRESSION_INT_BINARY_BOR,
                                    Probabilities::get().EXPRESSION_INT_BINARY_BXOR};

    switch (Utils::getRandInt(percent)) {
        case 0: {
            IR::Expression *left = constructIntExpr();
            IR::Expression *right = constructIntExpr();
            // pick a multiplication that matches the type
            expr = new IR::Mul(tp, left, right);
        } break;
        case 1: {
            // pick a division that matches the type
            // TODO(fruffy): Make more sophisticated
            P4Scope::req.not_negative = true;
            IR::Expression *left = genIntLiteral();
            P4Scope::req.not_zero = true;
            IR::Expression *right = genIntLiteral();
            P4Scope::req.not_zero = false;
            P4Scope::req.not_negative = false;
            expr = new IR::Div(tp, left, right);
        } break;
        case 2: {
            // pick a modulo that matches the type
            // TODO(fruffy): Make more sophisticated
            P4Scope::req.not_negative = true;
            IR::Expression *left = genIntLiteral();
            P4Scope::req.not_zero = true;
            IR::Expression *right = genIntLiteral();
            P4Scope::req.not_zero = false;
            P4Scope::req.not_negative = false;
            expr = new IR::Mod(tp, left, right);
        } break;
        case 3: {
            IR::Expression *left = constructIntExpr();
            IR::Expression *right = constructIntExpr();
            // pick an addition that matches the type
            expr = new IR::Add(tp, left, right);
        } break;
        case 4: {
            IR::Expression *left = constructIntExpr();
            IR::Expression *right = constructIntExpr();
            // pick a subtraction that matches the type
            expr = new IR::Sub(tp, left, right);
        } break;
        case 5: {
            // width must be known so we cast
            IR::Expression *left = constructIntExpr();
            // TODO(fruffy): Make this more sophisticated,
            P4Scope::req.not_negative = true;
            IR::Expression *right = constructIntExpr();
            // shifts are limited to 8 bits
            right = new IR::Cast(IR::Type_Bits::get(8, false), right);
            P4Scope::req.not_negative = false;
            expr = new IR::Shl(tp, left, right);
        } break;
        case 6: {
            // width must be known so we cast
            IR::Expression *left = constructIntExpr();
            // TODO(fruffy): Make this more sophisticated,
            P4Scope::req.not_negative = true;
            IR::Expression *right = constructIntExpr();
            // shifts are limited to 8 bits
            right = new IR::Cast(IR::Type_Bits::get(8, false), right);
            P4Scope::req.not_negative = false;
            expr = new IR::Shr(tp, left, right);
        } break;
        case 7: {
            IR::Expression *left = constructIntExpr();
            IR::Expression *right = constructIntExpr();
            // pick an binary And that matches the type
            expr = new IR::BAnd(tp, left, right);
        } break;
        case 8: {
            IR::Expression *left = constructIntExpr();
            IR::Expression *right = constructIntExpr();
            // pick a binary Or and that matches the type
            expr = new IR::BOr(tp, left, right);
        } break;
        case 9: {
            IR::Expression *left = constructIntExpr();
            IR::Expression *right = constructIntExpr();
            // pick an binary Xor that matches the type
            expr = new IR::BXor(tp, left, right);
        } break;
    }
    return expr;
}

IR::Expression *ExpressionGenerator::pickIntVar() {
    const auto *tp = IR::Type_InfInt::get();
    if (P4Scope::checkLval(tp)) {
        cstring name = P4Scope::pickLval(tp);
        return new IR::PathExpression(name);
    }

    // fallback, just generate a literal
    return genIntLiteral();
}

IR::Expression *ExpressionGenerator::constructIntExpr() {
    IR::Expression *expr = nullptr;

    std::vector<int64_t> percent = {
        Probabilities::get().EXPRESSION_INT_VAR, Probabilities::get().EXPRESSION_INT_INT_LITERAL,
        Probabilities::get().EXPRESSION_INT_UNARY, Probabilities::get().EXPRESSION_INT_BINARY};

    switch (Utils::getRandInt(percent)) {
        case 0: {
            expr = pickIntVar();
        } break;
        case 1: {
            // pick an int literal that matches the type
            expr = genIntLiteral();
        } break;
        case 2: {
            // pick a unary expression that matches the type
            expr = constructUnaryIntExpr();
        } break;
        case 3: {
            // pick a binary expression that matches the type
            expr = constructBinaryIntExpr();
        } break;
    }
    return expr;
}

IR::ListExpression *ExpressionGenerator::genStructListExpr(const IR::Type_Name *tn) {
    IR::Vector<IR::Expression> components;
    cstring tnName = tn->path->name.name;

    if (const auto *td = P4Scope::getTypeByName(tnName)) {
        if (const auto *tnType = td->to<IR::Type_StructLike>()) {
            for (const auto *sf : tnType->fields) {
                IR::Expression *expr = nullptr;
                if (const auto *fieldTn = sf->type->to<IR::Type_Name>()) {
                    if (const auto *typedefType = P4Scope::getTypeByName(fieldTn->path->name.name)
                                                      ->to<IR::Type_Typedef>()) {
                        expr = genExpression(typedefType);
                        components.push_back(expr);
                    } else {
                        expr = genStructListExpr(fieldTn);
                        components.push_back(expr);
                    }
                } else if (const auto *fieldTs = sf->type->to<IR::Type_Stack>()) {
                    auto stackSize = fieldTs->getSize();
                    const auto *stackType = fieldTs->elementType;
                    if (const auto *sTypeName = stackType->to<IR::Type_Name>()) {
                        for (size_t idx = 0; idx < stackSize; ++idx) {
                            expr = genStructListExpr(sTypeName);
                            components.push_back(expr);
                        }

                    } else {
                        BUG("genStructListExpr: Stack Type %s unsupported", tnName);
                    }
                } else {
                    expr = genExpression(sf->type);
                    components.push_back(expr);
                }
            }
        } else {
            BUG("genStructListExpr: Requested Type %s not a struct-like type", tnName);
        }
    } else {
        BUG("genStructListExpr: Requested Type %s not found", tnName);
    }
    return new IR::ListExpression(components);
}

IR::Expression *ExpressionGenerator::constructStructExpr(const IR::Type_Name *tn) {
    IR::Expression *expr = nullptr;
    std::vector<int64_t> percent = {Probabilities::get().EXPRESSION_STRUCT_VAR,
                                    Probabilities::get().EXPRESSION_STRUCT_LITERAL,
                                    Probabilities::get().EXPRESSION_STRUCT_FUNCTION};

    // because fallthrough is !very portable...
    bool useDefaultExpr = false;

    switch (Utils::getRandInt(percent)) {
        case 0:
            // pick a type from the available list
            // do !pick, if the requirement is to be a compile time known value
            if (P4Scope::checkLval(tn) && !P4Scope::req.compile_time_known) {
                cstring lval = P4Scope::pickLval(tn);
                expr = new IR::TypeNameExpression(lval);
            } else {
                // if there is no suitable declaration we fall through
                useDefaultExpr = true;
            }
            break;
        case 1: {
            // construct a list expression out of base-types
            useDefaultExpr = true;
        } break;
        case 2: {
            // run a function call
            auto p4Functions = P4Scope::getDecls<IR::Function>();
            auto p4Externs = P4Scope::getDecls<IR::Method>();

            IR::IndexedVector<IR::Declaration> viableFunctions;
            for (const auto *fun : p4Functions) {
                if (fun->type->returnType == tn) {
                    viableFunctions.push_back(fun);
                }
            }
            for (const auto *fun : p4Externs) {
                if (fun->type->returnType == tn) {
                    viableFunctions.push_back(fun);
                }
            }

            const IR::Type *retType = nullptr;
            expr = pickFunction(viableFunctions, &retType);
            // can !find a suitable function, generate a default value
            if (expr == nullptr) {
                useDefaultExpr = true;
                break;
            }
        }
    }
    if (useDefaultExpr) {
        expr = genStructListExpr(tn);
    }
    return expr;
}

IR::Expression *ExpressionGenerator::genInputArg(const IR::Parameter *param) {
    if (param->direction == IR::Direction::In) {
        // this can be any value
        return genExpression(param->type);
    }
    if (param->direction == IR::Direction::None) {
        // such args can only be compile-time constants
        P4Scope::req.compile_time_known = true;
        auto *expr = genExpression(param->type);
        P4Scope::req.compile_time_known = false;
        return expr;
    }
    // for inout and out the value must be writeable
    return pickLvalOrSlice(param->type);
}

bool ExpressionGenerator::checkInputArg(const IR::Parameter *param) {
    if (param->direction == IR::Direction::In || param->direction == IR::Direction::None) {
        return P4Scope::checkLval(param->type, false);
    }
    return P4Scope::checkLval(param->type, true);
}

size_t split(const std::string &txt, std::vector<cstring> &strs, char ch) {
    size_t pos = txt.find(ch);
    size_t initialPos = 0;
    strs.clear();

    // Decompose statement
    while (pos != std::string::npos) {
        strs.emplace_back(txt.substr(initialPos, pos - initialPos));
        initialPos = pos + 1;

        pos = txt.find(ch, initialPos);
    }

    // Add the last one
    strs.emplace_back(txt.substr(initialPos, std::min(pos, txt.size()) - initialPos + 1));

    return strs.size();
}

IR::Expression *ExpressionGenerator::editHdrStack(cstring lval) {
    // Check if there is a stack bracket inside the string.
    // Also, if we can not have variables inside the header stack index,
    // then just return the original expression.
    // FIXME: terrible but at least works for now
    if ((lval.find('[') == nullptr) || P4Scope::constraints.const_header_stack_index) {
        return new IR::PathExpression(lval);
    }

    std::vector<cstring> splitStr;
    int size = split(lval.c_str(), splitStr, '.');
    if (size < 1) {
        BUG("Unexpected split size. %d", size);
    }
    auto sIter = std::begin(splitStr);
    IR::Expression *expr = new IR::PathExpression(*sIter);
    for (advance(sIter, 1); sIter != splitStr.end(); ++sIter) {
        // if there is an index, convert it towards the proper expression
        auto subStr = *sIter;
        const auto *hdrBrkt = subStr.find('[');
        if (hdrBrkt != nullptr) {
            auto stackStr = subStr.substr(static_cast<size_t>(hdrBrkt - subStr + 1));
            const auto *stackSzEnd = stackStr.find(']');
            if (stackSzEnd == nullptr) {
                BUG("There should be a closing bracket.");
            }
            int stackSz = std::stoi(stackStr.before(stackSzEnd).c_str());
            expr = new IR::Member(expr, subStr.before(hdrBrkt));
            auto *tb = IR::Type_Bits::get(3, false);
            IR::Expression *idx = genExpression(tb);
            auto *args = new IR::Vector<IR::Argument>();
            args->push_back(new IR::Argument(idx));
            args->push_back(new IR::Argument(new IR::Constant(tb, stackSz)));
            idx = new IR::MethodCallExpression(new IR::PathExpression("max"), args);

            expr = new IR::ArrayIndex(expr, idx);
        } else {
            expr = new IR::Member(expr, subStr);
        }
    }
    return expr;
}

IR::Expression *ExpressionGenerator::pickLvalOrSlice(const IR::Type *tp) {
    cstring lvalStr = P4Scope::pickLval(tp, true);
    IR::Expression *expr = editHdrStack(lvalStr);
    expr->type = tp;

    if (const auto *tb = tp->to<IR::Type_Bits>()) {
        std::vector<int64_t> percent = {Probabilities::get().SCOPE_LVAL_PATH,
                                        Probabilities::get().SCOPE_LVAL_SLICE};
        switch (Utils::getRandInt(percent)) {
            case 0: {
                break;
            }
            case 1: {
                auto keyTypesOpt =
                    P4Scope::getWriteableLvalForTypeKey(IR::Type_Bits::static_type_name());
                if (!keyTypesOpt) {
                    break;
                }
                auto keyTypes = keyTypesOpt.value();
                size_t targetWidth = tb->width_bits();

                std::vector<std::pair<size_t, cstring>> candidates;

                for (const auto &bitBucket : keyTypes) {
                    size_t keySize = bitBucket.first;
                    if (keySize > targetWidth) {
                        for (cstring lval : keyTypes[keySize]) {
                            candidates.emplace_back(keySize, lval);
                        }
                    }
                }
                if (candidates.empty()) {
                    break;
                }
                auto idx = Utils::getRandInt(0, candidates.size() - 1);
                auto lval = std::begin(candidates);
                // "advance" the iterator idx times
                std::advance(lval, idx);
                auto candidateSize = lval->first;
                auto *sliceExpr =
                    new IR::PathExpression(IR::Type_Bits::get(candidateSize), lval->second);
                auto low = Utils::getRandInt(0, candidateSize - targetWidth);
                auto high = low + targetWidth - 1;
                expr = new IR::Slice(sliceExpr, high, low);
            } break;
        }
    }
    return expr;
}

}  // namespace P4::P4Tools::P4Smith
