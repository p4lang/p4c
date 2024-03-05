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

#ifndef COMMON_CONSTANTFOLDING_H_
#define COMMON_CONSTANTFOLDING_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"
#include "lib/big_int_util.h"

namespace P4 {

/// A policy for constant folding that allows customization of the folding.
/// Currently we only have hook for customizing IR::PathExpression, but more can be added.
/// Each hook takes a visitor and a node and is called from the visitor's preorder function on that
/// node. If the hook returns a non-null value, so will the preorder. Otherwise the preorder
/// continues with its normal processing. The hooks can be stateful, they are non-const member
/// functions.
class ConstantFoldingPolicy {
 public:
    /// The default hook does not modify anything.
    virtual const IR::Node *hook(Visitor &, IR::PathExpression *) { return nullptr; }
};

/** @brief statically evaluates many constant expressions.
 *
 * This pass can be invoked either with or without the `refMap` and
 * `typeMap`. When type information is not available, constant folding
 * is not performed for many IR nodes.
 *
 * @pre: `typeMap` is up-to-date if not `nullptr` and similarly for `refMap`
 *
 * @post: Ensures
 *
 *    - most expressions that can be statically shown to evaluate to a
 *      constant are replaced with the constant value.
 *
 *    - operations that involve constant InfInt operands are evaluated to
 *      an InfInt value
 *
 *    - if `typeMap` and `refMap` are not `nullptr` then
 *      `IR::Declaration_Constant` nodes are initialized with
 *      compile-time known constants.
 */
class DoConstantFolding : public Transform {
 protected:
    ConstantFoldingPolicy *policy;

    /// Used to resolve IR nodes to declarations.
    /// If `nullptr`, then `const` values cannot be resolved.
    const ReferenceMap *refMap;

    /// Used to resolve nodes to their types.
    /// If `nullptr`, then type information is not available.
    const TypeMap *typeMap;

    /// Set to `true` iff `typeMap` is not `nullptr`.
    bool typesKnown;

    /// If `true` then emit warnings
    bool warnings;

    /// Maps declaration constants to constant expressions
    std::map<const IR::Declaration_Constant *, const IR::Expression *> constants;
    // True if we are processing a left side of an assignment; we should not
    // we substituting constants there.
    bool assignmentTarget;

    /// @returns a constant equivalent to @p expr or `nullptr`
    const IR::Expression *getConstant(const IR::Expression *expr) const;

    /// Statically cast constant @p node to @p type represented in the specified @p base.
    const IR::Constant *cast(const IR::Constant *node, unsigned base,
                             const IR::Type_Bits *type) const;

    /// Statically evaluate binary operation @p e implemented by @p func.
    const IR::Node *binary(const IR::Operation_Binary *op,
                           std::function<big_int(big_int, big_int)> func, bool saturating = false);
    /// Statically evaluate comparison operation @p e.
    /// Note that this only handles the case where @p e represents `==` or `!=`.
    const IR::Node *compare(const IR::Operation_Binary *op);

    /// Statically evaluate shift operation @p e.
    const IR::Node *shift(const IR::Operation_Binary *op);

    /// Result type for @ref setContains.
    enum class Result { Yes, No, DontKnow };

    /** Statically evaluate case in select expression.
     *
     * @returns
     *    - Result::Yes
     *    - Result::No
     *    - Result::DontKnow
     *
     *  depending on whether @p constant is contained in @p keyset.
     */
    Result setContains(const IR::Expression *keySet, const IR::Expression *constant) const;

 public:
    DoConstantFolding(const ReferenceMap *refMap, const TypeMap *typeMap, bool warnings = true,
                      ConstantFoldingPolicy *policy = nullptr)
        : refMap(refMap), typeMap(typeMap), typesKnown(typeMap != nullptr), warnings(warnings) {
        if (policy) {
            this->policy = policy;
        } else {
            this->policy = new ConstantFoldingPolicy();
        }
        visitDagOnce = true;
        setName("DoConstantFolding");
        assignmentTarget = false;
    }

    const IR::Node *postorder(IR::Declaration_Constant *d) override;
    const IR::Node *postorder(IR::PathExpression *e) override;
    const IR::Node *postorder(IR::Cmpl *e) override;
    const IR::Node *postorder(IR::Neg *e) override;
    const IR::Node *postorder(IR::UPlus *e) override;
    const IR::Node *postorder(IR::LNot *e) override;
    const IR::Node *postorder(IR::LAnd *e) override;
    const IR::Node *postorder(IR::LOr *e) override;
    const IR::Node *postorder(IR::Slice *e) override;
    const IR::Node *postorder(IR::Add *e) override;
    const IR::Node *postorder(IR::AddSat *e) override;
    const IR::Node *postorder(IR::Sub *e) override;
    const IR::Node *postorder(IR::SubSat *e) override;
    const IR::Node *postorder(IR::Mul *e) override;
    const IR::Node *postorder(IR::Div *e) override;
    const IR::Node *postorder(IR::Mod *e) override;
    const IR::Node *postorder(IR::BXor *e) override;
    const IR::Node *postorder(IR::BAnd *e) override;
    const IR::Node *postorder(IR::BOr *e) override;
    const IR::Node *postorder(IR::Equ *e) override;
    const IR::Node *postorder(IR::Neq *e) override;
    const IR::Node *postorder(IR::Lss *e) override;
    const IR::Node *postorder(IR::Grt *e) override;
    const IR::Node *postorder(IR::Leq *e) override;
    const IR::Node *postorder(IR::Geq *e) override;
    const IR::Node *postorder(IR::Shl *e) override;
    const IR::Node *postorder(IR::Shr *e) override;
    const IR::Node *postorder(IR::Concat *e) override;
    const IR::Node *postorder(IR::Member *e) override;
    const IR::Node *postorder(IR::Cast *e) override;
    const IR::Node *postorder(IR::Mux *e) override;
    const IR::Node *postorder(IR::Type_Bits *type) override;
    const IR::Node *postorder(IR::Type_Varbits *type) override;
    const IR::Node *postorder(IR::SelectExpression *e) override;
    const IR::Node *postorder(IR::IfStatement *statement) override;
    const IR::Node *preorder(IR::AssignmentStatement *statement) override;
    const IR::Node *preorder(IR::ArrayIndex *e) override;
    const IR::BlockStatement *preorder(IR::BlockStatement *bs) override {
        if (bs->annotations->getSingle("disable_optimization")) prune();
        return bs;
    }
};

/** Optionally runs @ref TypeChecking if @p typeMap is not
 *  `nullptr`, and then runs @ref DoConstantFolding.
 * If policy is provided, it can modify behaviour of the constant folder.
 */
class ConstantFolding : public PassManager {
 public:
    ConstantFolding(ReferenceMap *refMap, TypeMap *typeMap, ConstantFoldingPolicy *policy)
        : ConstantFolding(refMap, typeMap, true, nullptr, policy) {}

    ConstantFolding(ReferenceMap *refMap, TypeMap *typeMap, bool warnings = true,
                    TypeChecking *typeChecking = nullptr, ConstantFoldingPolicy *policy = nullptr) {
        if (typeMap != nullptr) {
            if (!typeChecking) typeChecking = new TypeChecking(refMap, typeMap);
            passes.push_back(typeChecking);
        }
        passes.push_back(new DoConstantFolding(refMap, typeMap, warnings, policy));
        if (typeMap != nullptr) passes.push_back(new ClearTypeMap(typeMap));
        setName("ConstantFolding");
    }
};

}  // namespace P4

#endif /* COMMON_CONSTANTFOLDING_H_ */
