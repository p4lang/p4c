/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "elim_cast.h"
#include "bf-p4c/common/slice.h"

namespace BFN {

const IR::Node* EliminateWidthCasts::preorder(IR::Cast* expression) {
    const IR::Type* srcType = expression->expr->type;
    const IR::Type* dstType = expression->destType;

    if (srcType->is<IR::Type_Bits>() && dstType->is<IR::Type_Bits>()) {
        if (srcType->to<IR::Type_Bits>()->isSigned != dstType->to<IR::Type_Bits>()->isSigned)
            return expression;

        if (srcType->width_bits() > dstType->width_bits()) {
            // This code block handle the funnel_shift_right() P4_14 operation.
            // P4_14 code:                funnel_shift_right(dest, src_hi, src_lo, shift)
            // is translated in P4_16 as: dest = (bit<dest>)Shr((src_hi ++ src_lo), shift)
            //
            // It is possible to slice the source and the destination instead of doing the
            // shift operation. Sometime it is a good option but sometime it is impossible.
            // The code below will use the funnel-shift if one of the source is also the
            // destination operand in which case slicing is impossible. Other cases will
            // be handled through slicing.
            if (auto *shift = expression->expr->to<IR::Shr>()) {
                if (auto *concat = shift->left->to<IR::Concat>()) {
                    auto *assignment = findContext<IR::AssignmentStatement>();
                    if (assignment != nullptr) {
                        // If the destination is one of the source, don't slice it and let
                        // the instruction selection backend pass convert it to a funnel-shift
                        // operation.
                        if (assignment->left->equiv(*concat->left) ||
                            assignment->left->equiv(*concat->right))
                            return expression;
                    }
                }
            }
            return new IR::Slice(expression->srcInfo,
                    expression->destType, expression->expr,
                    new IR::Constant(dstType->width_bits() - 1), new IR::Constant(0));
        } else if (srcType->width_bits() < dstType->width_bits()) {
            // Special Shift case when destination field is larger than a container size while
            // the source is not. The instruction is broke in two parts up to a destination
            // field size of 64 bits.
            if ((expression->expr->is<IR::Shl>()) && (dstType->width_bits() > MAX_CONTAINER_SIZE) &&
                (srcType->width_bits() <= MAX_CONTAINER_SIZE)) {
                auto *shift = expression->expr->to<IR::Shl>();
                auto *value = shift->right->to<IR::Constant>();
                if (!value) {
                    error("%s: shift count must be a constant in %s", expression->srcInfo,
                          expression);
                    return expression;
                }
                if (value->asInt() >= (2 * MAX_CONTAINER_SIZE)) {
                    error("%s: shifting of more than 63 bits with casting is too complex to "
                          "handle, consider simplifying the expression.", expression->srcInfo);
                    return expression;
                }
                if (value->asInt() >= MAX_CONTAINER_SIZE) {
                    // This is to cover for shifting over an entire container, for example:
                    // hdr.shift_32_to_64.res = (bit<64>)(hdr.shift_32_to_64.b << 40);
                    // as:
                    // hdr.shift_32_to_64.res = (hdr.shift_32_to_64.b << 8) ++ 32w0;
                    auto *shift_adj = new IR::Constant(value->asInt() - MAX_CONTAINER_SIZE);
                    return new IR::Concat(new IR::Shl(expression->srcInfo, shift->left, shift_adj),
                                          new IR::Constant(IR::Type_Bits::get(MAX_CONTAINER_SIZE),
                                                           0));
                } else {
                    // This is to cover for splilling part of the shift to top slice, for example:
                    // hdr.shift_32_to_64.res = (bit<64>)(hdr.shift_32_to_64.b << 4);
                    // as:
                    // hdr.shift_32_to_64.res = (hdr.shift_32_to_64.b >> 28) ++
                    //                          (hdr.shift_32_to_64.b << 4);
                    auto *shift_adj = new IR::Constant(MAX_CONTAINER_SIZE - value->asInt());
                    return new IR::Concat(new IR::Shr(expression->srcInfo, shift->left, shift_adj),
                                          shift);
                }
            } else if (!srcType->to<IR::Type_Bits>()->isSigned) {
                auto ctype = IR::Type_Bits::get(dstType->width_bits() - srcType->width_bits());
                return new IR::Concat(new IR::Constant(ctype, 0), expression->expr);
            } else {
                return new IR::BFN::SignExtend(expression->srcInfo, expression->destType,
                        expression->expr);
            }
        }
    }
    // cast from InfInt to bit<w> type is already handled in frontend.
    return expression;
}

const IR::Node* RewriteCastToReinterpretCast::preorder(IR::Cast* expression) {
    const IR::Type *srcType = expression->expr->type;
    const IR::Type *dstType = expression->destType;

    if (srcType->is<IR::Type_Boolean>() || dstType->is<IR::Type_Boolean>()) {
        auto retval = new IR::BFN::ReinterpretCast(expression->srcInfo,
                                                   expression->destType, expression->expr);
        typeMap->setType(retval, expression->destType);
        return retval;
    } else if (srcType->is<IR::Type_Bits>() && dstType->is<IR::Type_Bits>()) {
        if (srcType->width_bits() != dstType->width_bits())
            return expression;
        if (srcType->to<IR::Type_Bits>()->isSigned != dstType->to<IR::Type_Bits>()->isSigned) {
            auto retval = new IR::BFN::ReinterpretCast(expression->srcInfo,
                                                       expression->destType, expression->expr);
            typeMap->setType(retval, expression->destType);
            return retval; }
    }
    return expression;
}

const IR::Node* SimplifyRedundantCasts::preorder(IR::Cast* expression) {
    const IR::Type *srcType = expression->expr->type;
    const IR::Type *dstType = expression->destType;

    if (!srcType->is<IR::Type_Bits>() || !dstType->is<IR::Type_Bits>())
        return expression;

    if (srcType->width_bits() != dstType->width_bits())
        return expression;

    if (expression->expr->is<IR::Cast>()) {
        const IR::Cast *innerCast = expression->expr->to<IR::Cast>();
        if (srcType->to<IR::Type_Bits>()->isSigned == dstType->to<IR::Type_Bits>()->isSigned)
            return expression;
        if (dstType->width_bits() == innerCast->expr->type->width_bits())
            return innerCast->expr;
    } else {
        if (srcType->to<IR::Type_Bits>()->isSigned != dstType->to<IR::Type_Bits>()->isSigned)
            return expression;
        if (dstType->width_bits() == srcType->width_bits())
            return expression->expr;
    }

    return expression;
}

const IR::Node* SimplifyNestedCasts::preorder(IR::Cast* expression) {
    if (!expression->expr->is<IR::Cast>())
        return expression;

    const IR::Type *dstType = expression->destType;
    auto secondCast = expression->expr->to<IR::Cast>();

    // stop if not a nested cast
    if (!secondCast)
        return expression;

    // stop if more than two levels of nested casts.
    if (secondCast->expr->is<IR::Cast>()) {
        error("Expression %1% is too complex to handle, "
                "consider simplifying the nested casts.", expression);
        return expression; }

    const IR::Type *srcType = secondCast->destType;
    auto innerExpr = secondCast->expr;

    if (!srcType->is<IR::Type_Bits>() || !dstType->is<IR::Type_Bits>() ||
            !innerExpr->type->is<IR::Type_Bits>())
        return expression;

    // only handle (bit<a>)(bit<b>)(bit<c>)
    if (srcType->to<IR::Type_Bits>()->isSigned ||
        dstType->to<IR::Type_Bits>()->isSigned ||
        innerExpr->type->to<IR::Type_Bits>()->isSigned)
        return expression;

    // given a expression (bit<a>)(bit<b>)(md), where the type of md is
    // bit<c>, we need to handle the following four cases:
    // case 1: a > b > c, the expression should be simplified to (bit<a>)(md);
    // case 2: a > b < c, the expression should be simplified to (bit<a>)(slice(md, b-1, 0))
    //    we reject this expression for now.
    // case 3: a < b > c, the expression should be simplified to (bit<a>)(md);
    // case 4: a < b < c, the expression should be simplified to (bit<a>)(md);
    if (dstType->width_bits() <= srcType->width_bits()) {
        return new IR::Cast(dstType, innerExpr);
    } else if (dstType->width_bits() > srcType->width_bits() &&
               srcType->width_bits() > innerExpr->type->width_bits()) {
        return new IR::Cast(dstType, innerExpr);
    } else {
        error("Expression %1% is too complex to handle, "
                "consider simplifying the nested casts.", expression); }

    return expression;
}

const IR::Node* SimplifyOperationBinary::preorder(IR::Cast *expression) {
    if (!expression->expr->is<IR::Operation_Binary>())
        return expression;

    // cannot cast lhs and rhs to boolean on
    // a bitwise binary operation.
    if (expression->destType->is<IR::Type_Boolean>())
        return expression;

    auto binop = expression->expr->to<IR::Operation_Binary>();
    IR::Expression* left = nullptr;
    IR::Expression* right = nullptr;

    // do not simplify if any operand is not bit type
    if (!binop->left->type->is<IR::Type_Bits>() || !binop->right->type->is<IR::Type_Bits>())
        return expression;

    // do not simplify if any operand is signed bit type
    if (binop->left->type->to<IR::Type_Bits>()->isSigned ||
        binop->right->type->to<IR::Type_Bits>()->isSigned)
        return expression;

    if (expression->type->width_bits() > binop->type->width_bits()) {
        // widening cast
        if (binop->is<IR::Add>() || binop->is<IR::Sub>() || binop->is<IR::Mul>() ||
            binop->is<IR::AddSat>() || binop->is<IR::SubSat>()) {
            // would change carry/overflow/saturate behavior, so can't do it.
            return expression; }
    } else if (expression->type->width_bits() < binop->type->width_bits()) {
        // narrowing cast
        if (binop->is<IR::AddSat>() || binop->is<IR::SubSat>() ||
            binop->is<IR::Div>() || binop->is<IR::Mod>()) {
            // would change saturate or rounding behavior, so can't do it.
            return expression; } }

    if (binop->left->is<IR::Constant>()) {
        auto cst = binop->left->to<IR::Constant>();
        left = new IR::Constant(expression->destType, cst->value, cst->base);
    } else {
        left = new IR::Cast(expression->destType, binop->left); }

    if (binop->right->is<IR::Constant>()) {
        auto cst = binop->right->to<IR::Constant>();
        right = new IR::Constant(expression->destType, cst->value, cst->base);
    } else {
        right = new IR::Cast(expression->destType, binop->right); }

    if (auto op = expression->expr->to<IR::Add>())
        return new IR::Add(op->srcInfo, left, right);
    if (auto op = expression->expr->to<IR::Sub>())
        return new IR::Sub(op->srcInfo, left, right);
    if (auto op = expression->expr->to<IR::AddSat>())
        return new IR::AddSat(op->srcInfo, left, right);
    if (auto op = expression->expr->to<IR::SubSat>())
        return new IR::SubSat(op->srcInfo, left, right);
    if (auto op = expression->expr->to<IR::BAnd>())
        return new IR::BAnd(op->srcInfo, left, right);
    if (auto op = expression->expr->to<IR::BOr>())
        return new IR::BOr(op->srcInfo, left, right);
    if (auto op = expression->expr->to<IR::BXor>())
        return new IR::BXor(op->srcInfo, left, right);
    return expression;
}

namespace {

struct SlicePair {
    const IR::Expression* hi;
    const IR::Expression* lo;
};

struct Statements {
    safe_vector<const IR::Declaration_Variable*> decl;
    safe_vector<const IR::AssignmentStatement*> wr_tmp;
    safe_vector<const IR::AssignmentStatement*> rd_tmp;
};

bool nested_nonsliceable(const IR::Expression* expr);

SlicePair slice_value(const IR::Expression* value, const IR::Concat* concatOp) {
    // We have removed all nonsliceable sub-expressions (the check is a bit expensive).
    // BUG_CHECK(!nested_nonsliceable(value), "value expression not sliceable");
    int lsize = concatOp->left->type->width_bits();
    int rsize = concatOp->right->type->width_bits();
    return SlicePair{/* hi */ MakeSlice(value, rsize, rsize + lsize - 1),
                     /* lo */ MakeSlice(value, 0, rsize - 1)};
}

const IR::Expression* temp_var(const IR::Type* type, Statements& stmts) {
    static int tmpvar_created = 0;  // Shared by all pass instantiations.
    IR::ID id("$concat_to_slice" + std::to_string(tmpvar_created++));
    stmts.decl.push_back(new IR::Declaration_Variable(id, type));
    return new IR::PathExpression(type, new IR::Path(id));
}

const IR::Expression* assign_expr_to_temp(const IR::Expression* expr, Statements& stmts) {
    auto temp = temp_var(expr->type, stmts);
    stmts.wr_tmp.push_back(new IR::AssignmentStatement(expr->srcInfo, temp, expr));
    return temp;
}

const IR::Expression* assign_concat_to_temp(const IR::Concat* ccatOp, Statements& stmts) {
    auto temp = temp_var(ccatOp->type, stmts);
    auto tslice = slice_value(temp, ccatOp);
    stmts.wr_tmp.push_back(new IR::AssignmentStatement(ccatOp->srcInfo, tslice.hi, ccatOp->left));
    stmts.wr_tmp.push_back(new IR::AssignmentStatement(ccatOp->srcInfo, tslice.lo, ccatOp->right));
    return temp;
}

void assign_slice(const IR::Expression* lvalue, const IR::Expression* rvalue, Statements& stmts) {
    auto temp = assign_expr_to_temp(rvalue, stmts);
    // N.B. rd_tmp must be issued after all wr_tmp as lvalue & rvalue may be the same field (swap).
    stmts.rd_tmp.push_back(new IR::AssignmentStatement(lvalue->srcInfo, lvalue, temp));
}

const IR::Node* create_block(Statements& stmts, IR::Statement* stmt = nullptr) {
    BUG_CHECK(!stmts.wr_tmp.empty(), "Unexpected behaviour");
    auto* block = new IR::BlockStatement;
    for (const auto& decl : stmts.decl)
        block->components.push_back(decl);
    for (const auto& wr_tmp : stmts.wr_tmp)
        block->components.push_back(wr_tmp);
    for (const auto& rd_tmp : stmts.rd_tmp)
        block->components.push_back(rd_tmp);
    if (stmt)
        block->components.push_back(stmt);
    return block;
}

bool zero_extended(const IR::Concat* concatOp) {
    auto left = concatOp->left->to<IR::Constant>();
    return (left && left->asInt() == 0);
}

bool nonsliceable(const IR::Operation_Binary* binOp) {
    // These are sliceable by MakeSlice(), see bf-p4c\common\slice.cpp.
    bool sliceable = binOp->is<IR::Concat>() ||
                     binOp->is<IR::Constant>() ||
                     binOp->is<IR::BAnd>() ||
                     binOp->is<IR::BOr>() ||
                     binOp->is<IR::BXor>() ||
    // We also include comparison operations.
                     binOp->is<IR::Equ>() ||
                     binOp->is<IR::Neq>();
    return !sliceable;
}

bool nested_nonsliceable(const IR::Expression* expr) {
    if (auto sliceOp = expr->to<IR::Slice>())
        return nested_nonsliceable(sliceOp->e0);
    if (auto binOp = expr->to<IR::Operation_Binary>()) {
        if (nonsliceable(binOp))
            return true;
        return nested_nonsliceable(binOp->left) || nested_nonsliceable(binOp->right);
    }
    return false;
}

// returns nullptr if none found, else the first find.
const IR::Concat* find_nested_concat(const IR::Expression* expr) {
    // concats have already been removed from nonsliceables by remove_concat_from_nonsliceable().
    if (auto sliceOp = expr->to<IR::Slice>()) {  // IR::Mux clean.
        // BUG_CHECK(!find_nested_concat(sliceOp->e1) && !find_nested_concat(sliceOp->e2), "!!");
        return find_nested_concat(sliceOp->e0);
    }
    if (auto binOp = expr->to<IR::Operation_Binary>()) {
        if (nonsliceable(binOp))
            return nullptr;    // nonsliceable clean.
        if (auto concatOp = expr->to<IR::Concat>())
            return concatOp;
        if (auto concatOp = find_nested_concat(binOp->left))
            return concatOp;
        if (auto concatOp = find_nested_concat(binOp->right))
            return concatOp;
    }
    return nullptr;
}

const IR::Expression* remove_concat_from_nonsliceable(const IR::Expression* expr,
                                                      Statements& stmts,
                                                      bool in_nonsliceable = false);

// returns nullptr if nothing changed, else the new node.
const IR::Expression* remove_concat_from_nonsliceable(const IR::Operation_Binary* binOp,
                                                      Statements& stmts,
                                                      bool in_nonsliceable) {
    if (in_nonsliceable) {
        if (auto concatOp = binOp->to<IR::Concat>()) {
            if (!zero_extended(concatOp)) {
                // Break the complex expression down the best we can.
                // TODO Should we cut slightly higher viz include sliceable parents?
                if (nested_nonsliceable(concatOp))
                    return assign_expr_to_temp(concatOp, stmts);
                else
                    return assign_concat_to_temp(concatOp, stmts);
            }
            // else leave the zero extending Concat expressions in nonsliceable expressions.
       }
    } else {
        in_nonsliceable = nonsliceable(binOp);
    }

    auto newleft = remove_concat_from_nonsliceable(binOp->left, stmts, in_nonsliceable);
    auto newright = remove_concat_from_nonsliceable(binOp->right, stmts, in_nonsliceable);
    if (newleft || newright) {
        IR::Operation_Binary* clone = binOp->clone();
        if (newleft)
            clone->left = newleft;
        if (newright)
            clone->right = newright;
        return clone;
    }
    return nullptr;
}

// returns nullptr if nothing changed, else the new node.
const IR::Expression* remove_concat_from_nonsliceable(const IR::Operation_Ternary* ternOp,
                                                      Statements& stmts,
                                                      bool in_nonsliceable) {
    // IR::Mux needs e1 & e2 checking too.
    auto newe0 = remove_concat_from_nonsliceable(ternOp->e0, stmts, in_nonsliceable);
    auto newe1 = remove_concat_from_nonsliceable(ternOp->e1, stmts, in_nonsliceable);
    auto newe2 = remove_concat_from_nonsliceable(ternOp->e2, stmts, in_nonsliceable);
    if (newe0 || newe1 || newe2) {
        IR::Operation_Ternary* clone = ternOp->clone();
        if (newe0)
            clone->e0 = newe0;
        if (newe1)
            clone->e1 = newe1;
        if (newe2)
            clone->e2 = newe2;
        return clone;
    }
    return nullptr;
}

// returns nullptr if nothing changed, else the new node.
const IR::Expression* remove_concat_from_nonsliceable(const IR::Expression* expr,
                                                      Statements& stmts,
                                                      bool in_nonsliceable) {
    if (auto ternOp = expr->to<IR::Operation_Ternary>())
        return remove_concat_from_nonsliceable(ternOp, stmts, in_nonsliceable);
    if (auto binOp = expr->to<IR::Operation_Binary>())
        return remove_concat_from_nonsliceable(binOp, stmts, in_nonsliceable);
    return nullptr;
}

// returns nullptr if nothing changed, else the new node.
const IR::Expression* remove_nonsliceable_from_sliceable(const IR::Expression* expr,
                                                         Statements& stmts) {
    if (auto ternOp = expr->to<IR::Operation_Ternary>()) {
        if (auto sliceOp = expr->to<IR::Slice>()) {
            // We don't expect the ranges to contain nonsliceable expressions!
            // BUG_CHECK(!nested_nonsliceable(sliceOp->e1) &&
            //           !nested_nonsliceable(sliceOp->e2), "Is this legal?!?");
            return remove_nonsliceable_from_sliceable(sliceOp->e0, stmts);
        }
        return assign_expr_to_temp(ternOp, stmts);
    }

    if (auto binOp = expr->to<IR::Operation_Binary>()) {
        if (nonsliceable(binOp))
            return assign_expr_to_temp(binOp, stmts);
        auto newleft = remove_nonsliceable_from_sliceable(binOp->left, stmts);
        auto newright = remove_nonsliceable_from_sliceable(binOp->right, stmts);
        if (newleft || newright) {
            IR::Operation_Binary* clone = binOp->clone();
            if (newleft)
                clone->left = newleft;
            if (newright)
                clone->right = newright;
            return clone;
        }
    }
    return nullptr;
}

// Slice the entire assignment.
void slice_assignment(const IR::Expression* lvalue, const IR::Expression* rvalue,
                      Statements& stmts) {
    if (auto concatOp = find_nested_concat(rvalue)) {
        auto lvSlice = slice_value(lvalue, concatOp);
        auto rvSlice = slice_value(rvalue, concatOp);
        slice_assignment(lvSlice.hi, rvSlice.hi, stmts);
        slice_assignment(lvSlice.lo, rvSlice.lo, stmts);
    } else {
        assign_slice(lvalue, rvalue, stmts);
    }
}

}  // namespace

const IR::Node* RewriteConcatToSlices::preorder(IR::AssignmentStatement* stmt) {
    // The returned node will be re-visited.
    // N.B. A previously visited node may not require further processing.
    // TODO Use the visitor to traverse rather than doing it here.

    // First remove sliceable expressions out of sub-expressions that can't be sliced,
    // (unless they are zero extensions) as they can't be slice where they are.
    Statements stmts;
    if (auto newright = remove_concat_from_nonsliceable(stmt->right, stmts)) {
        stmt->right = newright;
        return create_block(stmts, stmt);
    }
    if (find_nested_concat(stmt->right)) {
        // Next remove sub-expressions that can't be sliced from expressions that need to be sliced.
        // N.B. a sliceable concat could be anywhere in the expression.
        if (auto newright = remove_nonsliceable_from_sliceable(stmt->right, stmts)) {
            stmt->right = newright;
            return create_block(stmts, stmt);
        }
        // Before we can slice the entire assignment, we need to handle boolean expresions.
        // Delay completion for postorder...
    }
    return stmt;
}
const IR::Node* RewriteConcatToSlices::postorder(IR::AssignmentStatement* stmt) {
    // ...any boolean expression have now been sliced, so we can continue.
    Statements stmts;
    if (find_nested_concat(stmt->right)) {
        // Now we can slice the entire assignment.
        slice_assignment(stmt->left, stmt->right, stmts);
        return create_block(stmts);
    }
    return stmt;
}

namespace {

// N.B. Currently we need to maintain deterministic order viz left & right sides stay left & right,
//      otherwise PHV placement will fail.
// To search both sides of the expression, we can't swap left & right, so we pass in an enum.
enum class Search {Left, Right};
enum class Comp {Equ, Neq};
const IR::Operation_Binary* replaceConcatComp(const IR::Operation_Binary* expr,
                                              Search search, Comp comp) {
    if (auto concatOp = find_nested_concat(search == Search::Left? expr->left : expr->right)) {
        auto lSlice = slice_value(expr->left, concatOp);
        auto rSlice = slice_value(expr->right, concatOp);
        if (comp == Comp::Equ) {
            return new IR::LAnd(
                expr->srcInfo,
                new IR::Equ(expr->srcInfo, lSlice.hi, rSlice.hi),
                new IR::Equ(expr->srcInfo, lSlice.lo, rSlice.lo));
        } else {
            return new IR::LOr(
                expr->srcInfo,
                new IR::Neq(expr->srcInfo, lSlice.hi, rSlice.hi),
                new IR::Neq(expr->srcInfo, lSlice.lo, rSlice.lo));
        }
    }
    return nullptr;
}

}  // namespace

const IR::Node* RewriteConcatToSlices::preorder(IR::IfStatement* stmt) {
    // The returned node will be re-visited.

    // See comment re RewriteConcatToSlices::preorder(IR::AssignmentStatement* stmt)
    // First remove sliceable expressions out of sub-expressions that can't be sliced,
    // (unless they are logical expressions) as they can't be slice where they are.
    Statements stmts;
    if (auto newcond = remove_concat_from_nonsliceable(stmt->condition, stmts)) {
        stmt->condition = newcond;
        return create_block(stmts, stmt);
    }
    // Next remove sub-expressions that can't be sliced.
    if (find_nested_concat(stmt->condition)) {
       if (auto newcond = remove_nonsliceable_from_sliceable(stmt->condition, stmts)) {
            stmt->condition = newcond;
            return create_block(stmts, stmt);
        }
    }
    // Now we can slice the stmt->condition. See below.
    return stmt;
}

const IR::Node* RewriteConcatToSlices::preorder(IR::Equ *equ) {
    // The returned node will be re-visited.
    if (auto newexpr = replaceConcatComp(equ, Search::Left, Comp::Equ))
        return newexpr;
    if (auto newexpr = replaceConcatComp(equ, Search::Right, Comp::Equ))
        return newexpr;
    return equ;
}

const IR::Node* RewriteConcatToSlices::preorder(IR::Neq *neq) {
    // The returned node will be re-visited.
    if (auto newexpr = replaceConcatComp(neq, Search::Left, Comp::Neq))
        return newexpr;
    if (auto newexpr = replaceConcatComp(neq, Search::Right, Comp::Neq))
        return newexpr;
    return neq;
}

}  // namespace BFN
