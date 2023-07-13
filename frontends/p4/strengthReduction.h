/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef P4_STRENGTHREDUCTION_H_
#define P4_STRENGTHREDUCTION_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/sideEffects.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/** Implements a pass that replaces expensive arithmetic and boolean
 * operations with cheaper ones -- i.e., strength reduction
 *
 * Specifically, it provides:
 *
 * 1. A collection of helper methods that determine whether a given
 * expression is `0`, `1`, `true`, or `false`, or a power of `2`
 *
 * 2. A visitor that transforms arithmetic and boolean expressions
 *
 * @pre: None
 *
 * @post: Ensure that
 *   - most arithmetic and boolean expressions are simplified
 *   - division and modulus by `0`
 *
 */
class DoStrengthReduction final : public Transform {
    /// @returns `true` if @p expr is the constant `1`.
    bool isOne(const IR::Expression *expr) const;
    /// @returns `true` if @p expr is the constant `0`.
    bool isZero(const IR::Expression *expr) const;
    /// @returns `true` if @p expr is the constant `true`.
    bool isTrue(const IR::Expression *expr) const;
    /// @returns `true` if @p expr is the constant `false`.
    bool isFalse(const IR::Expression *expr) const;
    /// @returns `true` if @p expr is all ones.
    bool isAllOnes(const IR::Expression *expr) const;
    /// @returns the logarithm (base 2) of @p expr if it is positive
    /// and a power of `2` and `-1` otherwise.
    int isPowerOf2(const IR::Expression *expr) const;

    /// Used to determine conservatively if an expression
    /// has side-effects.  If we had a refMap or a typeMap
    /// we could use them here.
    bool hasSideEffects(const IR::Expression *expr) const {
        return SideEffects::check(expr, this, nullptr, nullptr);
    }

 public:
    DoStrengthReduction() {
        visitDagOnce = true;
        setName("StrengthReduction");
    }

    using Transform::postorder;

    const IR::Node *postorder(IR::Cmpl *expr) override;
    const IR::Node *postorder(IR::BAnd *expr) override;
    const IR::Node *postorder(IR::BOr *expr) override;
    const IR::Node *postorder(IR::Equ *expr) override;
    const IR::Node *postorder(IR::Neq *expr) override;
    const IR::Node *postorder(IR::BXor *expr) override;
    const IR::Node *postorder(IR::LAnd *expr) override;
    const IR::Node *postorder(IR::LOr *expr) override;
    const IR::Node *postorder(IR::LNot *expr) override;
    const IR::Node *postorder(IR::Sub *expr) override;
    const IR::Node *postorder(IR::Add *expr) override;
    const IR::Node *postorder(IR::UPlus *expr) override;
    const IR::Node *postorder(IR::Shl *expr) override;
    const IR::Node *postorder(IR::Shr *expr) override;
    const IR::Node *postorder(IR::Mul *expr) override;
    const IR::Node *postorder(IR::Div *expr) override;
    const IR::Node *postorder(IR::Mod *expr) override;
    const IR::Node *postorder(IR::Mux *expr) override;
    const IR::Node *postorder(IR::Slice *expr) override;
    const IR::Node *postorder(IR::Mask *expr) override;
    const IR::Node *postorder(IR::Range *expr) override;
    const IR::Node *postorder(IR::Concat *expr) override;
    const IR::Node *postorder(IR::ArrayIndex *expr) override;

    const IR::BlockStatement *preorder(IR::BlockStatement *bs) override {
        if (bs->annotations->getSingle("disable_optimization")) prune();
        return bs;
    }
};

class StrengthReduction : public PassManager {
 public:
    StrengthReduction(ReferenceMap *refMap, TypeMap *typeMap,
                      TypeChecking *typeChecking = nullptr) {
        if (typeMap != nullptr) {
            if (!typeChecking) typeChecking = new TypeChecking(refMap, typeMap, true);
            passes.push_back(typeChecking);
        }
        passes.push_back(new DoStrengthReduction());
    }
};

}  // namespace P4

#endif /* P4_STRENGTHREDUCTION_H_ */
