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
            // If index is an expression then create new variable.
            if (locals != nullptr && (curArrayIndex->right->is<IR::Operation_Ternary>() ||
                curArrayIndex->right->is<IR::Operation_Binary>() ||
                curArrayIndex->right->is<IR::Operation_Unary>())) {
                // Generate new temporary variable.
                auto type = typeMap->getTypeType(curArrayIndex->right->type, true);
                std::string newName = std::string("hsiVar")+std::to_string(locals->size());
                auto name = refMap->newName(newName);
                auto decl = new IR::Declaration_Variable(name, type);
                locals->push_back(decl);
                typeMap->setType(decl, type);
                auto* newArray = curArrayIndex->clone();
                auto* pathExpr =
                    new IR::PathExpression(curArrayIndex->srcInfo, type, new IR::Path(name));
                newArray->right = pathExpr;
                tmpArrayIndex = newArray;
            }
            arrayIndex = curArrayIndex;
        }
    } else {
        if (curArrayIndex->equiv(*arrayIndex)) {
            if (tmpArrayIndex != nullptr) {
                return tmpArrayIndex;
            }
            // Translating current array index.
            if (arrayIndex != nullptr) {
                auto* newArrayIndex = arrayIndex->clone();
                newArrayIndex->right = new IR::Constant(index);
                return newArrayIndex;
            }
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
    HSIndexFindOrTransform aiFinder(locals, refMap, typeMap);
    const IR::Node* updatedStatement = nullptr;
    const IR::Statement* elseBody = nullptr;
    auto* curIf = statement->to<IR::IfStatement>();
    if (curIf != nullptr) {
        elseBody = curIf->ifFalse;
        auto* newIf = curIf->clone();
        newIf->condition = curIf->condition->apply(aiFinder);
        updatedStatement = newIf;
    } else if (statement->is<IR::SwitchStatement>()) {
        auto* curSwitch = statement->to<IR::SwitchStatement>();
        updatedStatement = curSwitch->expression->apply(aiFinder);
    } else {
        updatedStatement = statement->apply(aiFinder);
    }
    if (aiFinder.arrayIndex == nullptr) {
        return statement;
    }
    IR::AssignmentStatement* assigment = nullptr;
    if (aiFinder.tmpArrayIndex != nullptr) {
        aiFinder.isFinder = false;
        updatedStatement = updatedStatement->apply(aiFinder)->to<IR::Statement>();
        assigment = new IR::AssignmentStatement(aiFinder.tmpArrayIndex->srcInfo,
            aiFinder.tmpArrayIndex->right, aiFinder.arrayIndex->right);
        aiFinder.arrayIndex = aiFinder.tmpArrayIndex;
        aiFinder.tmpArrayIndex = nullptr;
    }
    IR::IfStatement* result = nullptr;
    IR::IfStatement* curResult = nullptr;
    size_t sz = aiFinder.getArraySize();
    for (size_t i = 0; i < sz; i++) {
        HSIndexFindOrTransform aiRewriter(aiFinder.arrayIndex, i);
        IR::IfStatement* newIf = nullptr;
        auto* newStatement = updatedStatement->apply(aiRewriter)->to<IR::Statement>();
        auto* newIndex = aiFinder.arrayIndex->right->apply(aiRewriter);
        if (curIf != nullptr) {
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
    if (assigment == nullptr) {
        return result;
    } else {
        IR::IndexedVector<IR::StatOrDecl> newComponents;
        newComponents.push_back(assigment);
        newComponents.push_back(result);
        return new IR::BlockStatement(newComponents);
    }
}

IR::Node* HSIndexSimplifier::preorder(IR::AssignmentStatement* assignmentStatement) {
    return eliminateArrayIndexes(assignmentStatement);
}

IR::Node* HSIndexSimplifier::preorder(IR::P4Control* control) {
    auto* newControl = control->clone();
    HSIndexSimplifier hsSimplifier(refMap, typeMap, &newControl->controlLocals);
    newControl->body = newControl->body->apply(hsSimplifier)->to<IR::BlockStatement>();
    return newControl;
}

IR::Node* HSIndexSimplifier::preorder(IR::P4Parser* parser) {
    IR::IndexedVector<IR::Declaration> components;
    HSIndexFindOrTransform aiFinder(&components, refMap, typeMap);
    parser->apply(aiFinder);
    if (aiFinder.arrayIndex != nullptr)
        ::warning("ParsersUnroll class should be used for elimination of %1%", parser);
    // Ignore SelectExpression.
    return parser;
}

IR::Node* HSIndexSimplifier::preorder(IR::BlockStatement* blockStatement) {
    HSIndexSimplifier hsSimplifier(refMap, typeMap, locals);
    auto* newBlock = blockStatement->clone();
    IR::IndexedVector<IR::StatOrDecl> newComponents;
    for (auto& component : blockStatement->components) {
        const auto* newComponent = component->apply(hsSimplifier)->to<IR::StatOrDecl>();
        if (const auto* newComponentBlock = newComponent->to<IR::BlockStatement>()) {
            for (auto& blockComponent : newComponentBlock->components) {
                newComponents.push_back(blockComponent);
            }
        } else {
            newComponents.push_back(newComponent);
        }
    }
    newBlock->components = newComponents;
    return newBlock;
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
    return eliminateArrayIndexes(methodCallStatement);
}

IR::Node* HSIndexSimplifier::preorder(IR::SwitchStatement* switchStatement) {
    return eliminateArrayIndexes(switchStatement);
}

}  // namespace P4
