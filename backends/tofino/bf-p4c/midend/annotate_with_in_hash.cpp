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

#include "bf-p4c/midend/annotate_with_in_hash.h"
#include "bf-p4c/midend/path_linearizer.h"
#include "bf-p4c/midend/type_categories.h"

namespace BFN {

/**
 * The function checks whether the combination of the key table presence and
 * default action is suitable for using a hash unit. It deals with the following errors:
 *
 * error: Cannot specify @p action as the default action, as it requires the hash distribution
 *        unit.
 *
 * error: The table @p table with no key cannot have the action `<action>`.  This action requires
 *        hash, which can only be done through the hit pathway.  However, because the table has
 *        multiple actions, the driver may need to change at runtime, and the driver can only
 *        currently program the miss pathway.  The solution may be to declare only one action
 *        on the table, and mark it as the default action.
 *
 * @param[in] control  Containing control block
 * @param[in] action   Containing action
 * @return Returns true if the combination of the key table presence and
 *         default action is suitable; false otherwise.
 */
bool DoAnnotateWithInHash::checkKeyDefaultAction(const IR::P4Control &control,
        const IR::P4Action &action) const {
    for (const auto *decl : control.getDeclarations()->toVector()) {
        if (const auto *table = decl->to<IR::P4Table>()) {
            const auto *actionList = table->getActionList();
            if (actionList == nullptr) continue;  // The table has no actions
            const auto *actionListElement = actionList->getDeclaration(action.getName());
            if (!actionListElement) continue;  // The table does not call the action
            const IR::Expression *defaultAction = table->getDefaultAction();
            cstring defaultActionName;
            if (auto mce = defaultAction->to<IR::MethodCallExpression>()) {
                defaultActionName = mce->method->toString();
            } else {
                defaultActionName = defaultAction->toString();
            }
            bool isDefaultAction = (defaultActionName == action.getName());
            bool hasKey = (table->getKey() != nullptr);
            if ((isDefaultAction && hasKey) || (!isDefaultAction && !hasKey)) {
                return false;
            }
        }
    }
    return true;
}

/**
 * The function checks whether the structure of the assignment statement is suitable
 * for using the \@in_hash annotation. The structure has to respect the fact that
 * only the first operand of an ALU unit can be sourced from the hash unit, which
 * is the case of the \@in_hash unit applied to zero-extended operand.
 * The suitable structure is the following:
 *
 *   `(0 ++ <op1>) <binop> <op2>`
 *
 * In case `<binop>` is commutative, also the following case is suitable:
 *
 *   `<op1> <binop> (0 ++ <op2>)`
 *
 * Only (saturating) addition and (saturating) subtraction operations are supported
 * since other binary operations are treated in a different way and do not suffer
 * from this issue.
 *
 * @param[in] assignment  The assignment statement being checked
 * @param[out] op         The operand that is outside of the concatenation operation
 * @param[out] opConcat   The operand that is inside of the concatenation operation
 * @return Returns true if the structure of the assignment statement is suitable
 *         for using the \@in_hash annotation.
 */
bool DoAnnotateWithInHash::checkAssignmentStructure(const IR::AssignmentStatement &assignment,
        const IR::Expression **op, const IR::Expression **opConcat) {
    auto *binary = assignment.right->to<IR::Operation_Binary>();
    if (!binary || (!binary->is<IR::Add>() && !binary->is<IR::Sub>()
            && !binary->is<IR::AddSat>() && !binary->is<IR::SubSat>())) {
        return false;
    }

    bool commutative = (binary->is<IR::Add>() || binary->is<IR::AddSat>());

    const IR::Expression *opLeft = nullptr;
    const IR::Expression *opRight = nullptr;
    const IR::Concat *concatLeft = nullptr;
    const IR::Concat *concatRight = nullptr;

    const auto &isPathExpression = [](const IR::Expression *expr) {
        return expr->is<IR::PathExpression>() || expr->is<IR::Member>() || expr->is<IR::Slice>();
    };

    if (isPathExpression(binary->left))
        opLeft = binary->left;
    if (isPathExpression(binary->right))
        opRight = binary->right;
    if (binary->left->is<IR::Concat>())
        concatLeft = binary->left->to<IR::Concat>();
    if (binary->right->is<IR::Concat>())
        concatRight = binary->right->to<IR::Concat>();

    const IR::Concat *concat = nullptr;

    // OK for both commutative and non-commutative binary operations
    // (0 ++ <op1>) <binop> <op2>
    if (concatLeft && opRight) {
        *op = opRight;
        concat = concatLeft;
    }

    // OK only for commutative binary operations
    // <op1> <binop> (0 ++ <op2>)
    if (commutative) {
        if (opLeft && concatRight) {
            *op = opLeft;
            concat = concatRight;
        }
    }

    if (concat && concat->left->is<IR::Constant>()
            && concat->left->to<IR::Constant>()->asInt() == 0
            && isPathExpression(concat->right)) {
        *opConcat = concat->right;
    }

    return op && concat && opConcat;
}

/**
 * The function checks whether the operand has suitable width so that it could
 * be used as an ALU operand.
 *
 * @param[in] op  The operand being checked.
 * @return Returns true if the operand has suitable width for an ALU unit;
 *         false otherwise.
 */
bool DoAnnotateWithInHash::checkAluSuitability(const IR::Expression &op) const {
    int opWidth = typeMap->getType(&op)->width_bits();
    return opWidth == 8 || opWidth == 16 || opWidth == 32;
}

/**
 * The function checks whether the operand is suitable for being put into a %PHV container.
 * This is done by checking whether it is contained in a header or is metadata or intrinsic
 * metadata.
 *
 * @param[in] op  The operand being checked.
 * @return Return true if the operand is suitable for being put into a %PHV container;
 *         false otherwise.
 */
bool DoAnnotateWithInHash::checkHeaderMetadataReference(const IR::Expression &op) const {
    BFN::PathLinearizer path;
    op.apply(path);
    BUG_CHECK(path.linearPath != std::nullopt, "Missing linear path expression");
    return isHeaderReference(*path.linearPath, typeMap)
        || isMetadataReference(*path.linearPath, typeMap);
}

/**
 * If a block statement is already annotated with \@in_hash or \@in_vliw,
 * do not annotate it again.
 */
const IR::Node *DoAnnotateWithInHash::preorder(IR::BlockStatement *b) {
    if (b->getAnnotation("in_hash"_cs) || b->getAnnotation("in_vliw"_cs)) {
        prune();
    }
    return b;
}

/**
 * It is useless to traverse expressions.
 */
const IR::Node *DoAnnotateWithInHash::preorder(IR::Expression *e) {
    prune();
    return e;
}

const IR::Node *DoAnnotateWithInHash::preorder(IR::AssignmentStatement *assignment) {
    /*
     * The @in_hash annotation is supported only in an action in a control block.
     */
    auto *control = findContext<IR::BFN::TnaControl>();
    auto *action = findContext<IR::P4Action>();
    if (!control || !action) {
        prune();
        return assignment;
    }

    /*
     * Do not annotate in a default action if the table has a key.
     * Do not annotate in a non-default action if the table does not have a key.
     */
    if (!checkKeyDefaultAction(*control, *action)) {
        LOG2("Not annotating assignment statement in action " << action->getName()
            << " in a table with inappropriate combination of default "
                "action and table key presence.");
        prune();
        return assignment;
    }

    const IR::Expression *op = nullptr;
    const IR::Expression *opConcat = nullptr;

    /*
     * Checks the structure of the assignment statement.
     */
    if (!checkAssignmentStructure(*assignment, &op, &opConcat)) {
        LOG2("Not annotating assignment statement in action " << action->getName()
            << " which does not have appropriate form.");
        prune();
        return assignment;
    }

    /*
     * Checks whether the operands have suitable width for an ALU unit.
     * If so, the assignment statement will not be annotated since we
     * do not know whether the hash unit has to be used.
     */
    if (checkAluSuitability(*op) && checkAluSuitability(*opConcat)) {
        LOG2("Not annotating assignment statement in action " << action->getName()
            << " which has operands of appropriate width.");
        prune();
        return assignment;
    }

    /*
     * Checks whether the operands will be stored in a %PHV container and thus will not be sent
     * via action bus, which is to be used for carrying data from the hash unit.
     */
    if (!checkHeaderMetadataReference(*opConcat)) {
        LOG2("Not annotating assignment statement in action " << action->getName()
            << " whose concatenated operand is not a header or metadata type.");
        prune();
        return assignment;
    }
    if (!checkHeaderMetadataReference(*op)) {
        LOG2("Not annotating assignment statement in action " << action->getName()
            << " whose operand is not a header or metadata type.");
        prune();
        return assignment;
    }

    /*
     * If all previous checks passed, let's annotate the assignment statement.
     */
    LOG1("Annotating assignment statement in action " << action->getName() << ":"
        << std::endl << "\t" << assignment);
    prune();  // The annotated assignment statement would be visited recursively.
    return new IR::BlockStatement(
        assignment->srcInfo,
        new IR::Annotations({new IR::Annotation(IR::ID("in_hash"), {})}),
        {assignment});
}

}  // namespace BFN
