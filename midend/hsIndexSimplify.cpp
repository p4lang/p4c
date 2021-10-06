#include "midend/hsIndexSimplify.h"

#include <iostream>
#include <sstream>

namespace P4 {

const IR::Node* HSIndexFindOrTransform::postorder(IR::ArrayIndex* curArrayIndex) {
    if (curArrayIndex->right->is<IR::Constant>()) {
        return curArrayIndex;
    }
    // Find the first occurrence of non-concrete array index.
    if (isFinder) {
        if (arrayIndex == nullptr && !curArrayIndex->right->is<IR::Constant>()) {
            arrayIndex = curArrayIndex;
        }
    } else {
        // Translating current array index.
        if (arrayIndex != nullptr && curArrayIndex->equiv(*arrayIndex)) {
            auto* newArrayIndex = arrayIndex->clone();
            newArrayIndex->right = new IR::Constant(index);
            return newArrayIndex;
        }
    }
    return curArrayIndex;
}

size_t HSIndexFindOrTransform::getArraySize() {
    const auto* typeStack = arrayIndex->left->type->checkedTo<IR::Type_Stack>();
    return typeStack->getSize();
}

IR::Node* HSIndexSimplifier::eliminateArrayIndexes(IR::Statement* statement) {
    // Check non-concrete array indexes.
    HSIndexFindOrTransform aiFinder;
    const IR::Statement* elseBody = nullptr;
    bool isIf = statement->is<IR::IfStatement>();
    if (isIf) {
        auto* curIf = statement->to<IR::IfStatement>();
        elseBody = curIf->ifFalse;
        curIf->condition->apply(aiFinder);
    } else if (statement->is<IR::SwitchStatement>()) {
        auto* curSwitch = statement->to<IR::SwitchStatement>();
        curSwitch->expression->apply(aiFinder);
    } else {
        statement->apply(aiFinder);
    }
    if (aiFinder.arrayIndex == nullptr) {
        // Remove concrete indexes.
        HSIndexFindOrTransform aiElim(nullptr, 0);
        return const_cast<IR::Node*>(statement->apply(aiElim));
    }

    IR::IfStatement* result = nullptr;
    IR::IfStatement* curResult = nullptr;
    size_t sz = aiFinder.getArraySize();
    for (size_t i = 0; i < sz; i++) {
        HSIndexFindOrTransform aiRewriter(aiFinder.arrayIndex, i);
        IR::IfStatement* newIf = nullptr;
        auto* newStatement = statement->apply(aiRewriter)->to<IR::Statement>();
        auto* newIndex = aiFinder.arrayIndex->right->apply(aiRewriter)->to<IR::Expression>();
        if (isIf) {
            newIf = newStatement->to<IR::IfStatement>()->clone();
            newIf->condition = new IR::LAnd(
                new IR::Equ(newIndex, new IR::Constant(aiFinder.arrayIndex->right->type, i)),
                newIf->condition);
        } else {
            newIf = new IR::IfStatement(
                new IR::Equ(newIndex, new IR::Constant(aiFinder.arrayIndex->right->type, i)),
                newStatement, nullptr);
        }
        if (result == nullptr) {
            result = newIf;
            curResult = newIf;
        } else {
            curResult->ifFalse = newIf;
            curResult = newIf;
        }
    }
    curResult->ifFalse = elseBody;
    return result;
}

IR::Node* HSIndexSimplifier::preorder(IR::AssignmentStatement* assignmentStatement) {
    return eliminateArrayIndexes(assignmentStatement);
}

IR::Node* HSIndexSimplifier::preorder(IR::ConstructorCallExpression* expr) {
    // Eliminate concrete indexes.
    auto* newExpr = expr->clone();
    auto* newArguments = new IR::Vector<IR::Argument>();
    HSIndexFindOrTransform aiRewriter(nullptr, 0);
    for (const auto* arg : *expr->arguments)
        newArguments->push_back(arg->apply(aiRewriter)->to<IR::Argument>());
    newExpr->arguments = newArguments;
    return newExpr;
}

IR::Node* HSIndexSimplifier::preorder(IR::IfStatement* ifStatement) {
    return eliminateArrayIndexes(ifStatement);
}

IR::Node* HSIndexSimplifier::preorder(IR::MethodCallStatement* methodCallStatement) {
    return eliminateArrayIndexes(ifStatement);
}

IR::Node* HSIndexSimplifier::preorder(IR::SelectExpression* selectExpression) {
    // Ignore SelectExpression.
    return selectExpression;
}

IR::Node* HSIndexSimplifier::preorder(IR::SwitchStatement* switchStatement) {
    return eliminateArrayIndexes(switchStatement);
}

}  // namespace P4
