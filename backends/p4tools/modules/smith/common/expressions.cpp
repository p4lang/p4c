#include "backends/p4tools/modules/smith/common/expressions.h"

#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/scope.h"

namespace P4Tools::P4Smith {

const int Expressions::bit_widths[6];

IR::Type_Boolean *Expressions::genBoolType() { return new IR::Type_Boolean(); }

IR::Type_InfInt *Expressions::genIntType() { return new IR::Type_InfInt(); }

IR::Type *Expressions::pickRndBaseType(const std::vector<int64_t> &type_probs) {
    if (type_probs.size() != 7) {
        BUG("pickRndBaseType: Type probabilities must be exact");
    }
    IR::Type *tb = nullptr;
    switch (randInt(type_probs)) {
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

IR::Type *Expressions::pickRndType(TyperefProbs type_probs) {
    const std::vector<int64_t> &type_probs_vector = {
        type_probs.p4_bit,          type_probs.p4_signed_bit, type_probs.p4_varbit,
        type_probs.p4_int,          type_probs.p4_error,      type_probs.p4_bool,
        type_probs.p4_string,       type_probs.p4_enum,       type_probs.p4_header,
        type_probs.p4_header_stack, type_probs.p4_struct,     type_probs.p4_header_union,
        type_probs.p4_tuple,        type_probs.p4_void,       type_probs.p4_match_kind};

    const std::vector<int64_t> &basetype_probs = {
        type_probs.p4_bool, type_probs.p4_error,      type_probs.p4_int,   type_probs.p4_string,
        type_probs.p4_bit,  type_probs.p4_signed_bit, type_probs.p4_varbit};

    if (type_probs_vector.size() != 15) {
        BUG("pickRndType: Type probabilities must be exact");
    }
    IR::Type *tp = nullptr;
    size_t idx = randInt(type_probs_vector);
    switch (idx) {
        case 0: {
            // bit<>
            tp = Expressions::genBitType(false);
            break;
        }
        case 1: {
            // int<>
            tp = Expressions::genBitType(true);
            break;
        }
        case 2: {
            // varbit<>, this is not supported right now
            break;
        }
        case 3: {
            tp = Expressions::genIntType();
            break;
        }
        case 4: {
            // error, this is not supported right now
            break;
        }
        case 5: {
            // bool
            tp = Expressions::genBoolType();
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
            auto l_types = P4Scope::get_decls<IR::Type_Header>();
            if (l_types.size() == 0) {
                tp = Expressions().pickRndBaseType(basetype_probs);
                break;
            }
            auto candidate_type = l_types.at(getRndInt(0, l_types.size() - 1));
            auto type_name = candidate_type->name.name;
            // check if struct is forbidden
            if (P4Scope::not_initialized_structs.count(type_name) == 0) {
                tp = new IR::Type_Name(candidate_type->name.name);
            } else {
                tp = Expressions().pickRndBaseType(basetype_probs);
            }
            break;
        }
        case 9: {
            tp = Declarations().genHeaderStackType();
            break;
        }
        case 10: {
            // struct
            auto l_types = P4Scope::get_decls<IR::Type_Struct>();
            if (l_types.size() == 0) {
                tp = Expressions().pickRndBaseType(basetype_probs);
                break;
            }
            auto candidate_type = l_types.at(getRndInt(0, l_types.size() - 1));
            auto type_name = candidate_type->name.name;
            // check if struct is forbidden
            if (P4Scope::not_initialized_structs.count(type_name) == 0) {
                tp = new IR::Type_Name(candidate_type->name.name);
            } else {
                tp = Expressions().pickRndBaseType(basetype_probs);
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
            tp = new IR::Type_Void();
            break;
        }
        case 14: {
            // match kind, this is not supported right now
            break;
        }
    }
    if (!tp) {
        BUG("pickRndType: Chosen type is Null!");
    }

    return tp;
}

IR::BoolLiteral *Expressions::genBoolLiteral() {
    if (getRndInt(0, 1)) {
        return new IR::BoolLiteral(false);
    } else {
        return new IR::BoolLiteral(true);
    }
}

IR::Type_Bits *Expressions::genBitType(bool isSigned) {
    auto size = getRndInt(0, sizeof(bit_widths) / sizeof(int) - 1);

    return new IR::Type_Bits(bit_widths[size], isSigned);
}

IR::Constant *Expressions::genIntLiteral(size_t bit_width) {
    big_int min = -(((big_int)1 << bit_width - 1));
    if (P4Scope::req.not_negative) {
        min = 0;
    }
    big_int max = (((big_int)1 << bit_width - 1) - 1);
    big_int value = getRndBigInt(min, max);
    while (true) {
        if (P4Scope::req.not_zero && value == 0) {
            value = getRndBigInt(min, max);
            // retry until we generate a value that is !zero
            continue;
        }
        break;
    }
    return new IR::Constant(value);
}
IR::Constant *Expressions::genBitLiteral(const IR::Type *tb) {
    big_int max_size = ((big_int)1U << tb->width_bits());

    big_int value;
    if (P4Scope::req.not_zero) {
        value = getRndBigInt(1, max_size - 1);
    } else {
        value = getRndBigInt(0, max_size - 1);
    }
    return new IR::Constant(tb, value);
}

IR::Expression *Expressions::genExpression(const IR::Type *tp) {
    IR::Expression *expr = nullptr;

    // reset the expression depth
    P4Scope::prop.depth = 0;

    if (auto tb = tp->to<IR::Type_Bits>()) {
        expr = constructBitExpr(tb);
    } else if (tp->is<IR::Type_InfInt>()) {
        expr = constructIntExpr();
    } else if (tp->is<IR::Type_Boolean>()) {
        expr = constructBooleanExpr();
    } else if (auto tn = tp->to<IR::Type_Name>()) {
        expr = constructStructExpr(tn);
    } else {
        BUG("Expression: Type %s not yet supported", tp->node_type_name());
    }
    // reset the expression depth, just to be safe...
    P4Scope::prop.depth = 0;
    return expr;
}

IR::MethodCallExpression *Expressions::pickFunction(
    IR::IndexedVector<IR::Declaration> viable_functions, const IR::Type **ret_type) {
    // TODO: Make this more sophisticated
    if (viable_functions.size() == 0 || P4Scope::req.compile_time_known) {
        return nullptr;
    }

    size_t idx = getRndInt(0, viable_functions.size() - 1);
    cstring fun_name;
    const IR::ParameterList *params;
    if (auto p4_fun = viable_functions[idx]->to<IR::Function>()) {
        fun_name = p4_fun->name.name;
        params = p4_fun->getParameters();
        *ret_type = p4_fun->type->returnType;
    } else if (auto p4_extern = viable_functions[idx]->to<IR::Method>()) {
        fun_name = p4_extern->name.name;
        params = p4_extern->getParameters();
        *ret_type = p4_extern->type->returnType;
    } else {
        BUG("Unknown callable: Type %s not yet supported", viable_functions[idx]->node_type_name());
    }
    auto expr = genFunctionCall(fun_name, *params);
    // sometimes, functions may !be callable
    // because we do !have the right return values
    if (!expr || !ret_type) {
        return nullptr;
    }
    return expr;
}

IR::MethodCallExpression *Expressions::genFunctionCall(cstring method_name,
                                                       IR::ParameterList params) {
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();
    IR::IndexedVector<IR::StatOrDecl> decls;

    bool can_call = true;

    for (auto par : params) {
        if (!checkInputArg(par)) {
            can_call = false;
        } else {
            IR::Argument *arg;
            arg = new IR::Argument(genInputArg(par));
            args->push_back(arg);
        }
    }
    if (can_call) {
        auto path_expr = new IR::PathExpression(method_name);
        return new IR::MethodCallExpression(path_expr, args);
    } else {
        return nullptr;
    }
}

IR::ListExpression *Expressions::genExpressionList(IR::Vector<IR::Type> types, bool only_lval) {
    IR::Vector<IR::Expression> components;
    for (auto tb : types) {
        IR::Expression *expr;
        if (only_lval) {
            cstring lval_name = P4Scope::pick_lval(tb);
            expr = new IR::PathExpression(lval_name);
        } else {
            expr = genExpression(tb);
        }
        components.push_back(expr);
    }
    return new IR::ListExpression(components);
}

IR::Expression *Expressions::constructUnaryExpr(const IR::Type_Bits *tb) {
    IR::Expression *expr = nullptr;

    if (P4Scope::prop.depth > MAX_DEPTH) {
        return genBitLiteral(tb);
    }
    P4Scope::prop.depth++;

    // we want to avoid negation when we require no negative values
    int64_t neg_pct = PCT.EXPRESSION_BIT_UNARY_NEG;
    if (P4Scope::req.not_negative) {
        neg_pct = 0;
    }

    std::vector<int64_t> percent = {neg_pct, PCT.EXPRESSION_BIT_UNARY_CMPL,
                                    PCT.EXPRESSION_BIT_UNARY_CAST,
                                    PCT.EXPRESSION_BIT_UNARY_FUNCTION};

    switch (randInt(percent)) {
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
            auto p4_functions = P4Scope::get_decls<IR::Function>();
            auto p4_externs = P4Scope::get_decls<IR::Method>();

            IR::IndexedVector<IR::Declaration> viable_functions;
            for (auto fun : p4_functions) {
                if (fun->type->returnType->to<IR::Type_Bits>()) {
                    viable_functions.push_back(fun);
                }
            }
            for (auto fun : p4_externs) {
                if (fun->type->returnType->to<IR::Type_Bits>()) {
                    viable_functions.push_back(fun);
                }
            }
            const IR::Type *ret_type;
            expr = pickFunction(viable_functions, &ret_type);
            // can !find a suitable function, generate a default value
            if (!expr) {
                expr = genBitLiteral(tb);
                break;
            }
            // if the return value does !match try to cast it
            if (ret_type != tb) {
                expr = new IR::Cast(tb, expr);
            }
        } break;
    }
    return expr;
}

IR::Expression *Expressions::createSaturationOperand(const IR::Type_Bits *tb) {
    IR::Expression *expr = constructBitExpr(tb);

    int width = 0;
    if ((width = P4Scope::constraints.max_phv_container_width) != 0) {
        if (tb->width_bits() > width) {
            auto *type = new IR::Type_Bits(width, false);
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

IR::Expression *Expressions::constructBinaryBitExpr(const IR::Type_Bits *tb) {
    IR::Expression *expr = nullptr;

    if (P4Scope::prop.depth > MAX_DEPTH) {
        return genBitLiteral(tb);
    }
    P4Scope::prop.depth++;

    auto pct_sub = PCT.EXPRESSION_BIT_BINARY_SUB;
    auto pct_subsat = PCT.EXPRESSION_BIT_BINARY_SUBSAT;
    // we want to avoid subtraction when we require no negative values
    if (P4Scope::req.not_negative) {
        pct_sub = 0;
        pct_subsat = 0;
    }

    std::vector<int64_t> percent = {PCT.EXPRESSION_BIT_BINARY_MUL,
                                    PCT.EXPRESSION_BIT_BINARY_DIV,
                                    PCT.EXPRESSION_BIT_BINARY_MOD,
                                    PCT.EXPRESSION_BIT_BINARY_ADD,
                                    pct_sub,
                                    PCT.EXPRESSION_BIT_BINARY_ADDSAT,
                                    pct_subsat,
                                    PCT.EXPRESSION_BIT_BINARY_LSHIFT,
                                    PCT.EXPRESSION_BIT_BINARY_RSHIFT,
                                    PCT.EXPRESSION_BIT_BINARY_BAND,
                                    PCT.EXPRESSION_BIT_BINARY_BOR,
                                    PCT.EXPRESSION_BIT_BINARY_BXOR,
                                    PCT.EXPRESSION_BIT_BINARY_CONCAT};

    switch (randInt(percent)) {
        case 0: {
            IR::Expression *left = constructBitExpr(tb);
            IR::Expression *right = constructBitExpr(tb);
            // pick a multiplication that matches the type
            expr = new IR::Mul(tb, left, right);
        } break;
        case 1: {
            // pick a division that matches the type
            // TODO: Make more sophisticated
            // this requires only compile time known values
            IR::Expression *left = genBitLiteral(tb);
            P4Scope::req.not_zero = true;
            IR::Expression *right = genBitLiteral(tb);
            P4Scope::req.not_zero = false;
            expr = new IR::Div(tb, left, right);
        } break;
        case 2: {
            // pick a modulo that matches the type
            // TODO: Make more sophisticated
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
            // TODO: Make this more sophisticated,
            P4Scope::req.not_negative = true;
            IR::Expression *right = constructBitExpr(tb);
            P4Scope::req.not_negative = false;
            // TODO: Make this more sophisticated
            // shifts are limited to 8 bits
            if (P4Scope::constraints.const_lshift_count) {
                right = genBitLiteral(new IR::Type_Bits(P4Scope::req.shift_width, false));
            } else {
                right = new IR::Cast(new IR::Type_Bits(8, false), right);
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

            // TODO: Make this more sophisticated,
            P4Scope::req.not_negative = true;
            IR::Expression *right = constructBitExpr(tb);
            P4Scope::req.not_negative = false;
            // shifts are limited to 8 bits
            right = new IR::Cast(new IR::Type_Bits(8, false), right);
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
            size_t type_width = tb->width_bits();
            size_t split = getRndInt(1, type_width - 1);
            // TODO: lazy fallback
            if (split >= type_width) {
                return genBitLiteral(tb);
            }
            auto tl = new IR::Type_Bits(type_width - split, false);
            auto tr = new IR::Type_Bits(split, false);
            // width must be known so we cast
            // width must be known so we cast
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

IR::Expression *Expressions::constructTernaryBitExpr(const IR::Type_Bits *tb) {
    IR::Expression *expr = nullptr;

    if (P4Scope::prop.depth > MAX_DEPTH) {
        return genBitLiteral(tb);
    }
    P4Scope::prop.depth++;

    std::vector<int64_t> percent = {PCT.EXPRESSION_BIT_BINARY_SLICE, PCT.EXPRESSION_BIT_BINARY_MUX};

    switch (randInt(percent)) {
        case 0: {
            // TODO: Refine this...
            // pick a slice that matches the type
            auto type_width = tb->width_bits();
            // TODO this is some arbitrary value...
            auto new_type_size = getRndInt(1, 128) + type_width;
            auto slice_type = new IR::Type_Bits(new_type_size, false);
            auto slice_expr = constructBitExpr(slice_type);
            if (P4Scope::prop.width_unknown) {
                slice_expr = new IR::Cast(slice_type, slice_expr);
                P4Scope::prop.width_unknown = false;
            }
            auto margin = new_type_size - type_width;
            auto high = getRndInt(0, margin - 1) + type_width;
            auto low = high - type_width + 1;
            expr = new IR::Slice(slice_expr, high, low);
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

IR::Expression *Expressions::pickBitVar(const IR::Type_Bits *tb) {
    cstring node_name = tb->node_type_name();
    auto avail_bit_types = P4Scope::lval_map[node_name].size();
    if (P4Scope::check_lval(tb)) {
        cstring name = P4Scope::pick_lval(tb);
        return new IR::PathExpression(name);
    } else if (avail_bit_types > 0) {
        // even if we do !find anything we can still cast other bits
        auto new_tb = P4Scope::pick_declared_bit_type();
        cstring name = P4Scope::pick_lval(new_tb);
        return new IR::Cast(tb, new IR::PathExpression(name));
    }

    // fallback, just generate a literal
    return genBitLiteral(tb);
}

IR::Expression *Expressions::constructBitExpr(const IR::Type_Bits *tb) {
    IR::Expression *expr = nullptr;

    std::vector<int64_t> percent = {PCT.EXPRESSION_BIT_VAR,         PCT.EXPRESSION_BIT_INT_LITERAL,
                                    PCT.EXPRESSION_BIT_BIT_LITERAL, PCT.EXPRESSION_BIT_UNARY,
                                    PCT.EXPRESSION_BIT_BINARY,      PCT.EXPRESSION_BIT_TERNARY};

    switch (randInt(percent)) {
        case 0: {
            // pick a variable that matches the type
            // do !pick, if the requirement is to be a compile time known value
            // TODO: This is lazy, we can easily check
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

IR::Expression *Expressions::constructCmpExpr() {
    IR::Expression *expr = nullptr;

    // gen some random type
    // can be either bits, int, bool, or structlike
    // for now it is just bits
    auto new_type_size = getRndInt(1, 128);
    auto new_type = new IR::Type_Bits(new_type_size, false);
    IR::Expression *left = constructBitExpr(new_type);
    IR::Expression *right = constructBitExpr(new_type);

    std::vector<int64_t> percent = {PCT.EXPRESSION_BOOLEAN_CMP_EQU, PCT.EXPRESSION_BOOLEAN_CMP_NEQ};

    switch (randInt(percent)) {
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

IR::Expression *Expressions::constructBooleanExpr() {
    IR::Expression *expr = nullptr;
    IR::Expression *left;
    IR::Expression *right;

    std::vector<int64_t> percent = {
        PCT.EXPRESSION_BOOLEAN_VAR,      PCT.EXPRESSION_BOOLEAN_LITERAL, PCT.EXPRESSION_BOOLEAN_NOT,
        PCT.EXPRESSION_BOOLEAN_LAND,     PCT.EXPRESSION_BOOLEAN_LOR,     PCT.EXPRESSION_BOOLEAN_CMP,
        PCT.EXPRESSION_BOOLEAN_FUNCTION, PCT.EXPRESSION_BOOLEAN_BUILT_IN};

    switch (randInt(percent)) {
        case 0: {
            auto tb = new IR::Type_Boolean();
            // TODO: This is lazy, we can easily check
            if (P4Scope::req.compile_time_known) {
                expr = genBoolLiteral();
                break;
            }
            if (P4Scope::check_lval(tb)) {
                cstring name = P4Scope::pick_lval(tb);
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
            auto p4_functions = P4Scope::get_decls<IR::Function>();
            auto p4_externs = P4Scope::get_decls<IR::Method>();

            IR::IndexedVector<IR::Declaration> viable_functions;
            for (auto fun : p4_functions) {
                if (fun->type->returnType->to<IR::Type_Boolean>()) {
                    viable_functions.push_back(fun);
                }
            }
            for (auto fun : p4_externs) {
                if (fun->type->returnType->to<IR::Type_Boolean>()) {
                    viable_functions.push_back(fun);
                }
            }
            const IR::Type *ret_type;
            expr = pickFunction(viable_functions, &ret_type);
            // can !find a suitable function, generate a default value
            if (!expr) {
                expr = genBoolLiteral();
            }
        } break;
        case 7: {
            // get the expression
            auto tbl_set = P4Scope::get_callable_tables();

            // just generate a literal if there are no tables left
            if (tbl_set->size() == 0 || P4Scope::req.compile_time_known) {
                expr = genBoolLiteral();
                break;
            }
            auto idx = getRndInt(0, tbl_set->size() - 1);
            auto tbl_iter = std::begin(*tbl_set);

            std::advance(tbl_iter, idx);
            const IR::P4Table *tbl = *tbl_iter;
            expr = new IR::Member(new IR::MethodCallExpression(
                                      new IR::Member(new IR::PathExpression(tbl->name), "apply")),
                                  "hit");
            tbl_set->erase(tbl_iter);
        }
    }
    return expr;
}

IR::Expression *Expressions::constructUnaryIntExpr() {
    IR::Expression *expr = nullptr;

    if (P4Scope::prop.depth > MAX_DEPTH) {
        return genIntLiteral();
    }
    IR::Type *tp = new IR::Type_InfInt();
    P4Scope::prop.depth++;

    // we want to avoid negation when we require no negative values
    int64_t neg_pct = PCT.EXPRESSION_INT_UNARY_NEG;
    if (P4Scope::req.not_negative) {
        neg_pct = 0;
    }

    std::vector<int64_t> percent = {neg_pct, PCT.EXPRESSION_INT_UNARY_FUNCTION};

    switch (randInt(percent)) {
        case 0: {
            // pick a negation that matches the type
            expr = new IR::Neg(tp, constructIntExpr());
        } break;
        case 1: {
            auto p4_functions = P4Scope::get_decls<IR::Function>();
            auto p4_externs = P4Scope::get_decls<IR::Method>();

            IR::IndexedVector<IR::Declaration> viable_functions;
            for (auto fun : p4_functions) {
                if (fun->type->returnType->to<IR::Type_InfInt>()) {
                    viable_functions.push_back(fun);
                }
            }
            for (auto fun : p4_externs) {
                if (fun->type->returnType->to<IR::Type_InfInt>()) {
                    viable_functions.push_back(fun);
                }
            }
            const IR::Type *ret_type;
            expr = pickFunction(viable_functions, &ret_type);
            // can !find a suitable function, generate a default value
            if (!expr) {
                expr = genIntLiteral();
                break;
            }
        } break;
    }
    return expr;
}

IR::Expression *Expressions::constructBinaryIntExpr() {
    IR::Expression *expr = nullptr;
    if (P4Scope::prop.depth > MAX_DEPTH) {
        return genIntLiteral();
    }
    IR::Type *tp = new IR::Type_InfInt();
    P4Scope::prop.depth++;

    auto pct_sub = PCT.EXPRESSION_INT_BINARY_SUB;
    // we want to avoid subtraction when we require no negative values
    if (P4Scope::req.not_negative) {
        pct_sub = 0;
    }

    std::vector<int64_t> percent = {PCT.EXPRESSION_INT_BINARY_MUL,
                                    PCT.EXPRESSION_INT_BINARY_DIV,
                                    PCT.EXPRESSION_INT_BINARY_MOD,
                                    PCT.EXPRESSION_INT_BINARY_ADD,
                                    pct_sub,
                                    PCT.EXPRESSION_INT_BINARY_LSHIFT,
                                    PCT.EXPRESSION_INT_BINARY_RSHIFT,
                                    PCT.EXPRESSION_INT_BINARY_BAND,
                                    PCT.EXPRESSION_INT_BINARY_BOR,
                                    PCT.EXPRESSION_INT_BINARY_BXOR};

    switch (randInt(percent)) {
        case 0: {
            IR::Expression *left = constructIntExpr();
            IR::Expression *right = constructIntExpr();
            // pick a multiplication that matches the type
            expr = new IR::Mul(tp, left, right);
        } break;
        case 1: {
            // pick a division that matches the type
            // TODO: Make more sophisticated
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
            // TODO: Make more sophisticated
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
            // TODO: Make this more sophisticated,
            P4Scope::req.not_negative = true;
            IR::Expression *right = constructIntExpr();
            // shifts are limited to 8 bits
            right = new IR::Cast(new IR::Type_Bits(8, false), right);
            P4Scope::req.not_negative = false;
            expr = new IR::Shl(tp, left, right);
        } break;
        case 6: {
            // width must be known so we cast
            IR::Expression *left = constructIntExpr();
            // TODO: Make this more sophisticated,
            P4Scope::req.not_negative = true;
            IR::Expression *right = constructIntExpr();
            // shifts are limited to 8 bits
            right = new IR::Cast(new IR::Type_Bits(8, false), right);
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

IR::Expression *Expressions::pickIntVar() {
    IR::Type *tp = new IR::Type_InfInt();
    if (P4Scope::check_lval(tp)) {
        cstring name = P4Scope::pick_lval(tp);
        return new IR::PathExpression(name);
    }

    // fallback, just generate a literal
    return genIntLiteral();
}

IR::Expression *Expressions::constructIntExpr() {
    IR::Expression *expr = nullptr;

    std::vector<int64_t> percent = {PCT.EXPRESSION_INT_VAR, PCT.EXPRESSION_INT_INT_LITERAL,
                                    PCT.EXPRESSION_INT_UNARY, PCT.EXPRESSION_INT_BINARY};

    switch (randInt(percent)) {
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

IR::ListExpression *Expressions::genStructListExpr(const IR::Type_Name *tn) {
    IR::Vector<IR::Expression> components;
    cstring tn_name = tn->path->name.name;

    if (auto td = P4Scope::get_type_by_name(tn_name)) {
        if (auto tn_type = td->to<IR::Type_StructLike>()) {
            for (auto sf : tn_type->fields) {
                IR::Expression *expr;
                if (auto field_tn = sf->type->to<IR::Type_Name>()) {
                    // can!use another type here yet
                    expr = genStructListExpr(field_tn);
                    components.push_back(expr);
                } else if (auto field_ts = sf->type->to<IR::Type_Stack>()) {
                    auto stack_size = field_ts->getSize();
                    auto stack_type = field_ts->elementType;
                    if (auto s_type_name = stack_type->to<IR::Type_Name>()) {
                        for (size_t idx = 0; idx < stack_size; ++idx) {
                            expr = genStructListExpr(s_type_name);
                            components.push_back(expr);
                        }

                    } else {
                        BUG("genStructListExpr: Stack Type %s unsupported", tn_name);
                    }
                } else {
                    expr = genExpression(sf->type);
                    components.push_back(expr);
                }
            }
        } else {
            BUG("genStructListExpr: Requested Type %s not a struct-like type", tn_name);
        }
    } else {
        BUG("genStructListExpr: Requested Type %s not found", tn_name);
    }
    return new IR::ListExpression(components);
}

IR::Expression *Expressions::constructStructExpr(const IR::Type_Name *tn) {
    IR::Expression *expr = nullptr;
    std::vector<int64_t> percent = {PCT.EXPRESSION_STRUCT_VAR, PCT.EXPRESSION_STRUCT_LITERAL,
                                    PCT.EXPRESSION_STRUCT_FUNCTION};

    // because fallthrough is !very portable...
    bool use_default_expr = false;

    switch (randInt(percent)) {
        case 0:
            // pick a type from the available list
            // do !pick, if the requirement is to be a compile time known value
            if (P4Scope::check_lval(tn) && !P4Scope::req.compile_time_known) {
                cstring lval = P4Scope::pick_lval(tn);
                expr = new IR::TypeNameExpression(lval);
            } else {
                // if there is no suitable declaration we fall through
                use_default_expr = true;
            }
            break;
        case 1: {
            // construct a list expression out of base-types
            use_default_expr = true;
        } break;
        case 2: {
            // run a function call
            auto p4_functions = P4Scope::get_decls<IR::Function>();
            auto p4_externs = P4Scope::get_decls<IR::Method>();

            IR::IndexedVector<IR::Declaration> viable_functions;
            for (auto fun : p4_functions) {
                if (fun->type->returnType == tn) {
                    viable_functions.push_back(fun);
                }
            }
            for (auto fun : p4_externs) {
                if (fun->type->returnType == tn) {
                    viable_functions.push_back(fun);
                }
            }

            const IR::Type *ret_type;
            expr = pickFunction(viable_functions, &ret_type);
            // can !find a suitable function, generate a default value
            if (!expr) {
                use_default_expr = true;
                break;
            }
        }
    }
    if (use_default_expr) {
        expr = genStructListExpr(tn);
    }
    return expr;
}

IR::Expression *Expressions::genInputArg(const IR::Parameter *param) {
    if (param->direction == IR::Direction::In) {
        // this can be any value
        return Expressions().genExpression(param->type);
    } else if (param->direction == IR::Direction::None) {
        // such args can only be compile-time constants
        P4Scope::req.compile_time_known = true;
        auto expr = Expressions().genExpression(param->type);
        P4Scope::req.compile_time_known = false;
        return expr;
    } else {
        // for inout and out the value must be writeable
        return P4Scope::pick_lval_or_slice(param->type);
    }
}

bool Expressions::checkInputArg(const IR::Parameter *param) {
    if (param->direction == IR::Direction::In) {
        return P4Scope::check_lval(param->type, false);
    } else {
        return P4Scope::check_lval(param->type, true);
    }
}
}  // namespace P4Tools::P4Smith
