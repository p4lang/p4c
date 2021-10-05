#include "midend/hsIndexSimplify.h"

#include <iostream>
#include <sstream>

namespace P4 {

const IR::Node* HSIndexFinder::postorder(IR::ArrayIndex* curArrayIndex) {
    if (curArrayIndex->right->is<IR::Constant>()) {
        return exprIndex2Member(curArrayIndex->type, curArrayIndex->left,
                                curArrayIndex->right->checkedTo<IR::Constant>());
    }
    // Finding the first occurrence of non-concrete array index.
    if (isFinder) {
        if (arrayIndex == nullptr && !curArrayIndex->right->is<IR::Constant>()) {
            arrayIndex = curArrayIndex;
        }
    } else {
        // Translating current array index.
        if (arrayIndex != nullptr && curArrayIndex->equiv(*arrayIndex)) {
            return exprIndex2Member(curArrayIndex->type, curArrayIndex->left,
                                    new IR::Constant(index));
        }
    }
    return curArrayIndex;
}

size_t HSIndexFinder::getArraySize() {
    const auto* typeStack = arrayIndex->left->type->checkedTo<IR::Type_Stack>();
    return typeStack->getSize();
}

const IR::ArrayIndex* HSIndexFinder::exprIndex2Member(const IR::Type* type,
                                                      const IR::Expression* expression,
                                                      const IR::Constant* constant) {
    return new IR::ArrayIndex(type, expression, constant);
}

IR::Node* HSIndexSimplifier::eliminateArrayIndexes(IR::Statement* statement) {
    // Checking non-concrete array indexes.
    HSIndexFinder aiFinder;
    const IR::Statement* elseBody = nullptr;
    bool isIf = false;
    if (isIf = statement->is<IR::IfStatement>()) {
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
        HSIndexFinder aiElim(nullptr, 0);
        return const_cast<IR::Node*>(statement->apply(aiElim));
    }
    
    IR::IfStatement* result = nullptr;
    IR::IfStatement* curResult = nullptr;
    size_t sz = aiFinder.getArraySize();
    for (size_t i = 0; i < sz; i++) {
        HSIndexFinder aiRewriter(aiFinder.arrayIndex, i);
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
    HSIndexFinder aiRewriter(nullptr, 0);
    for (const auto* arg : *expr->arguments)
        newArguments->push_back(arg->apply(aiRewriter)->to<IR::Argument>());
    newExpr->arguments = newArguments;
    return newExpr;
}

IR::Node* HSIndexSimplifier::preorder(IR::IfStatement* ifStatement) {
    return eliminateArrayIndexes(ifStatement);
}

IR::Node* HSIndexSimplifier::preorder(IR::MethodCallStatement* methodCallStatement) {
    auto* newMethodCall = methodCallStatement->clone();
    HSIndexFinder aiRewriter(nullptr, 0);
    newMethodCall->methodCall =
        newMethodCall->methodCall->apply(aiRewriter)->to<IR::MethodCallExpression>();
    return newMethodCall;
}

IR::Node* HSIndexSimplifier::preorder(IR::SelectExpression* selectExpression) {
    // All non-concrete indexes should be eliminated by ParserUnroll.
    HSIndexFinder aiElim(nullptr, 0);
    return const_cast<IR::Node*>(selectExpression->apply(aiElim)->to<IR::Node>());
}

IR::Node* HSIndexSimplifier::preorder(IR::SwitchStatement* switchStatement) {
    return eliminateArrayIndexes(switchStatement);
}

}  // namespace P4
