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

/**
 * \defgroup ElimCasts BFN::ElimCasts
 * \ingroup midend
 * \brief Set of passes that simplify the complex expressions with
 *        multiple casts into simpler expressions.
 * 
 * Open issues, the following expressions are not yet handled.
 *  1. (bit<64>)(h.h.a << h.h.b)
 *  2. cast in register action apply functions
 *  3. ++ operator in method call.
 * 
 * This pass will simplify the complex expression with multiple casts
 * into simpler expression with at most one cast, e.g.:
 *
 *     int<8> i8 = 8s1;
 *     bit<16> h1 = (bit<16>)(bit<8>)i8;
 *
 * will be translated to:
 *
 *     int<8> i8 = 8s1;
 *     bit<8> b8 = (bit<8>)i8;  // RHS is represented with IR::BFN::ReinterpretCast
 *     bit<16> h1 = 8w0++b8;
 *
 * This pass does not simplify key elements. These are processed in the P4::SimplifyKey
 * pass, which transforms them in order to be simplified in the BFN::ElimCasts pass.
 */
#ifndef BF_P4C_MIDEND_ELIM_CAST_H_
#define BF_P4C_MIDEND_ELIM_CAST_H_

#include "ir/ir.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/strengthReduction.h"
#include "bf-p4c/midend/type_checker.h"

namespace BFN {

/**
 * \ingroup ElimCasts
 * \brief Auxiliary transformer to avoid processing IR::KeyElement nodes.
 * 
 * When this should become too complex, use a custom policy instead,
 * the same way it is used in P4::KeyIsSimple.
 */
class IgnoreKeyElementTransform : public Transform {
 public:
    const IR::Node *preorder(IR::KeyElement *n) final override { prune(); return n; }
};

/** 
 * \ingroup ElimCasts
 */
class EliminateWidthCasts : public IgnoreKeyElementTransform {
 public:
    static constexpr int MAX_CONTAINER_SIZE = 32;

    EliminateWidthCasts() { }
    const IR::Node* preorder(IR::Cast* cast) override;

#if 0
    // Caveat(hanw): do not simplify cast in apply functions.
    // stateful_alu implementation must handle '++'
    // before we can eliminate cast.
    const IR::Node* preorder(IR::Function* func) override {
        prune();
        return func; }
#endif

    // Caveat(hanw): we do not simplify the cast in a selection expression,
    // because we do not support '++' operator with a constant lhs.
    // Further, simplifying the selection expression has impact on
    // pvs control plan API generation that needs to be discussed with
    // Steve and Yuan.
    const IR::Node* preorder(IR::SelectExpression *expr) override {
        prune();
        return expr;
    }
};

/**
 * \ingroup ElimCasts
 * \brief Pass that converts some of the IR::Casts to ReinterpretCasts.
 * 
 * This pass converts IR::Cast to IR::BFN::ReinterpretCast for the following
 * cases:
 *   - cast from bit<W> to int<W>
 *   - cast from int<W> to bit<W>
 *   - cast from bit<1> to bool
 *   - cast from bool to bit<1>
 *
 * IR::BFN::ReinterpretCast is handled in the backend.
 */
class RewriteCastToReinterpretCast : public IgnoreKeyElementTransform {
    P4::TypeMap* typeMap;

 public:
    explicit RewriteCastToReinterpretCast(P4::TypeMap* typeMap) : typeMap(typeMap) {}
    const IR::Node* preorder(IR::Cast* expression) override;
};

/**
 * \ingroup ElimCasts
 * \brief Pass that removes some of the redundant casts.
 * 
 * This pass removes redundant casts in the following cases:
 *
 * - int<w> val = (int<w>)(bit<w>)(v); where the type of v is int<w>
 * - bit<w> val = (bit<w>)(int<w>)(v); where the type of v is bit<w>
 * - int<w> val = (int<w>)(v); where the type of v is int<w>
 * - bit<w> val = (bit<w>)(v); where the type of v is bit<w>
 *
 * This usually happens as the result of a copy-propagation.
 */
class SimplifyRedundantCasts : public IgnoreKeyElementTransform {
 public:
    SimplifyRedundantCasts() {}
    const IR::Node* preorder(IR::Cast* expression) override;
};

/**
 * \ingroup ElimCasts
 * \brief Pass that removes nested casts when applicable.
 * 
 * This pass removes nested casts with the same signs, e.g.:
 *
 *     bit<10> md;
 *     bit<10> b = (bit<10>)(bit<32>)(md);
 *
 * which should be simplified to
 *
 *     bit<10> b = md;
 *
 * Caveat: we currently do not support more than two levels of casting.
 */
class SimplifyNestedCasts : public IgnoreKeyElementTransform {
 public:
    SimplifyNestedCasts() {}
    const IR::Node* preorder(IR::Cast* expression) override;
};

/**
 * \ingroup ElimCasts
 * \brief Pass that moves the cast on binary operation to each operand.
 * 
 * This transformation does not apply to all binary operations,
 * and it is not sound if the operands are int type. So it is a best effort.
 * It also does not handle (bool)(expr binop expr), where binop is bitwise operation.
 *
 * We only simplifies add/sub, addsat/subsat,
 * bitwise operations (and, or, xor) on unsigned bit types.
 */
class SimplifyOperationBinary : public IgnoreKeyElementTransform {
 public:
    SimplifyOperationBinary() {}
    const IR::Node* preorder(IR::Cast *expression) override;
};


/**
 * \ingroup ElimCasts
 * \brief Pass that replaces concat ++ operations with multiple operations
 *        on slices in the contexts.
 */
class RewriteConcatToSlices : public IgnoreKeyElementTransform {
 public:
    // do not simplify '++' in apply functions.
    const IR::Node* preorder(IR::Function* func) override {
        prune();
        return func; }

    // do not simplify '++' in deparser block.
    const IR::Node* preorder(IR::BFN::TnaDeparser* dprsr) override {
        prune();
        return dprsr; }

    const IR::BlockStatement *preorder(IR::BlockStatement *blk) override {
        if (blk->getAnnotation("in_hash"_cs)) prune();
        return blk;
    }
    const IR::Node* preorder(IR::AssignmentStatement* stmt) override;
    const IR::Node* postorder(IR::AssignmentStatement* stmt) override;
    const IR::Node* preorder(IR::IfStatement* stmt) override;
    const IR::Node* preorder(IR::Equ *eq) override;
    const IR::Node* preorder(IR::Neq *ne) override;
};

/**
 * \ingroup ElimCasts
 */
class StrengthReduction : public PassManager {
 public:
    StrengthReduction(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
        passes.push_back(new BFN::TypeChecking(refMap, typeMap, true));
        passes.push_back(new P4::DoStrengthReduction());
    }
};

/**
 * \ingroup ElimCasts
 * \brief Top level PassManager that simplifies complex expression with multiple casts
 *        into simpler expression with at most one cast.
 */
class ElimCasts : public PassManager {
 public:
    ElimCasts(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
        addPasses({
           new SimplifyOperationBinary(),
           new PassRepeated({
               new SimplifyNestedCasts(),
               new SimplifyRedundantCasts()}),
           new RewriteCastToReinterpretCast(typeMap),
           new EliminateWidthCasts(),
           // EliminateWidthCasts might change some of the Type_Bits
           // to different objects representing the same type =>
           // rerun typechecking with empty typemap to properly
           // unify those new types
           new P4::ClearTypeMap(typeMap),
           new BFN::TypeChecking(refMap, typeMap, true),
           // FIXME -- StrengthReduction has problems with complex nested things like
           // (0 ++ field << 1)[15:0] which causes problems for Rewrite
           // so we repeat until a fixed point is reached.
           new PassRepeated({new StrengthReduction(refMap, typeMap)}),
           new RewriteConcatToSlices(),
           new P4::ClearTypeMap(typeMap),
           new BFN::TypeChecking(refMap, typeMap, true),
           new P4::SimplifyControlFlow(typeMap),
           new P4::ClearTypeMap(typeMap),
           new BFN::TypeChecking(refMap, typeMap, true),
        });
    }
};

}  // namespace BFN

#endif  /* BF_P4C_MIDEND_ELIM_CAST_H_ */
