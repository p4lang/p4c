#include "midend/hsIndexSimplify.h"

#include <iostream>
#include <sstream>

namespace P4 {

void HSIndexFinder::postorder(const IR::ArrayIndex* curArrayIndex) {
    if (arrayIndex == nullptr && !curArrayIndex->right->is<IR::Constant>()) {
        arrayIndex = curArrayIndex;
        addNewVariable();
    }
}

void HSIndexFinder::addNewVariable() {
    BUG_CHECK(arrayIndex != nullptr, "Can't generate new name for empty ArrayIndex");
    // If index is an expression then create new variable.
    if (locals != nullptr && (arrayIndex->right->is<IR::Operation_Ternary>() ||
        arrayIndex->right->is<IR::Operation_Binary>() ||
        arrayIndex->right->is<IR::Operation_Unary>() ||
        arrayIndex->right->is<IR::PathExpression>())) {
        // Generate new local variable if needed.
        std::ostringstream ostr;
        ostr << arrayIndex->right;
        cstring indexString = ostr.str();
        if (arrayIndex->right->is<IR::PathExpression>()) {
            newVariable = arrayIndex->right->to<IR::PathExpression>();
        } else if (generatedVariables->count(indexString) == 0) {
            // Generate new temporary variable.
            auto type = typeMap->getTypeType(arrayIndex->right->type, true);
            std::string newName = std::string("hsiVar")+std::to_string(generatedVariables->size());
            auto name = refMap->newName(newName);
            auto decl = new IR::Declaration_Variable(name, type);
            locals->push_back(decl);
            typeMap->setType(decl, type);
            newVariable = new IR::PathExpression(arrayIndex->srcInfo, type, new IR::Path(name));
            generatedVariables->emplace(indexString, newVariable);
        } else {
            newVariable = generatedVariables->at(indexString);
        }
    }
}

const IR::Node* HSIndexTransform::postorder(IR::ArrayIndex* curArrayIndex) {
    if (hsIndexFinder.arrayIndex != nullptr &&
        curArrayIndex->equiv(*hsIndexFinder.arrayIndex)) {
        // Translating current array index.
        auto* newArrayIndex = hsIndexFinder.arrayIndex->clone();
        newArrayIndex->right = new IR::Constant(newArrayIndex->right->type, index);
        return newArrayIndex;
    }
    return curArrayIndex;
}

IR::Node* HSIndexSimplifier::eliminateArrayIndexes(HSIndexFinder& aiFinder,
                                                   IR::Statement* statement) {
    if (aiFinder.arrayIndex == nullptr) {
        return statement;
    }
    const IR::Statement* elseBody = nullptr;
    auto* curIf = statement->to<IR::IfStatement>();
    if (curIf != nullptr) {
        elseBody = curIf->ifFalse;
    }
    IR::IndexedVector<IR::StatOrDecl> newComponents;
    if (aiFinder.newVariable != nullptr) {
        if (!aiFinder.newVariable->equiv(*aiFinder.arrayIndex->right)) {
            newComponents.push_back(new IR::AssignmentStatement(aiFinder.arrayIndex->srcInfo,
            aiFinder.newVariable, aiFinder.arrayIndex->right));
        }
    }
    IR::IfStatement* result = nullptr;
    IR::IfStatement* curResult = nullptr;
    IR::IfStatement* newIf = nullptr;
    const auto* typeStack = aiFinder.arrayIndex->left->type->checkedTo<IR::Type_Stack>();
    size_t sz = typeStack->getSize();
    for (size_t i = 0; i < sz; i++) {
        HSIndexTransform aiRewriter(aiFinder, i);
        auto* newStatement = statement->apply(aiRewriter)->to<IR::Statement>();
        auto* newIndex = new IR::Constant(aiFinder.arrayIndex->right->type, i);
        auto* newCondition = new IR::Equ(aiFinder.newVariable, newIndex);
        if (curIf != nullptr) {
            newIf = newStatement->to<IR::IfStatement>()->clone();
            newIf->condition = new IR::LAnd(newCondition, newIf->condition);
        } else {
            newIf = new IR::IfStatement(newCondition, newStatement, nullptr);
        }
        if (result == nullptr) {
            result = newIf;
            curResult = newIf;
        } else {
            curResult->ifFalse = newIf;
            curResult = newIf;
        }
    }
    if (locals != nullptr) {
        // Add case for out of bound.
        HSIndexTransform aiRewriter(aiFinder, sz - 1);
        auto* newStatement = statement->apply(aiRewriter)->to<IR::Statement>();
        auto* newIndex = new IR::Constant(aiFinder.arrayIndex->right->type, sz - 1);
        auto* newCondition = new IR::Geq(aiFinder.newVariable, newIndex);
        if (curIf != nullptr) {
            newIf = newStatement->to<IR::IfStatement>()->clone();
            newIf->condition = new IR::LAnd(newCondition, newIf->condition);
        } else {
            newIf = new IR::IfStatement(newCondition, newStatement, nullptr);
        }
        cstring typeString = aiFinder.arrayIndex->type->node_type_name();
        const IR::PathExpression* pathExpr = nullptr;
        if (generatedVariables->count(typeString) == 0) {
            // Add assigment of undefined header.
            auto type = typeMap->getTypeType(aiFinder.arrayIndex->type, true);
            std::string newName = std::string("hsVar")+std::to_string(locals->size());
            auto name = refMap->newName(newName);
            auto decl = new IR::Declaration_Variable(name, type);
            locals->push_back(decl);
            typeMap->setType(decl, type);
            pathExpr =
                new IR::PathExpression(aiFinder.arrayIndex->srcInfo, type, new IR::Path(name));
            generatedVariables->emplace(typeString, pathExpr);
        } else {
            pathExpr = generatedVariables->at(typeString);
        }
        auto* newArrayIndex = aiFinder.arrayIndex->clone();
        newArrayIndex->right = newIndex;
        newIf->ifFalse = elseBody;
        IR::IndexedVector<IR::StatOrDecl> overflowComponents;
        overflowComponents.push_back(new IR::AssignmentStatement(aiFinder.arrayIndex->srcInfo,
                                    newArrayIndex, pathExpr));
        overflowComponents.push_back(newIf);
        // Set result.
        curResult->ifFalse = new IR::BlockStatement(overflowComponents);
    } else {
        newIf->ifFalse = elseBody;
    }
    newComponents.push_back(result);
    return new IR::BlockStatement(newComponents);
}

IR::Node* HSIndexSimplifier::preorder(IR::AssignmentStatement* assignmentStatement) {
    HSIndexFinder aiFinder(locals, refMap, typeMap, generatedVariables);
    assignmentStatement->apply(aiFinder);
    return eliminateArrayIndexes(aiFinder, assignmentStatement);
}

IR::Node* HSIndexSimplifier::preorder(IR::P4Control* control) {
    auto* newControl = control->clone();
    IR::IndexedVector<IR::Declaration> newControlLocals;
    HSIndexSimplifier hsSimplifier(refMap, typeMap, &newControlLocals,
        generatedVariables);
    newControl->body = newControl->body->apply(hsSimplifier)->to<IR::BlockStatement>();
    for (auto* declaration : control->controlLocals) {
        if (declaration->is<IR::P4Action>()) {
            newControlLocals.push_back(declaration->apply(hsSimplifier)->to<IR::Declaration>());
        } else {
            newControlLocals.push_back(declaration);
        }
    }
    newControl->controlLocals = newControlLocals;
    return newControl;
}

IR::Node* HSIndexSimplifier::preorder(IR::P4Parser* parser) {
    prune();
    return parser;
}

IR::Node* HSIndexSimplifier::preorder(IR::BlockStatement* blockStatement) {
    generatedVariablesMap blockGeneratedVariables;
    HSIndexFinder aiFinder(locals, refMap, typeMap, &blockGeneratedVariables);
    blockStatement->apply(aiFinder);
    if (aiFinder.arrayIndex == nullptr) {
        return blockStatement;
    }
    HSIndexSimplifier hsSimplifier(refMap, typeMap, locals, &blockGeneratedVariables);
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

IR::Node* HSIndexSimplifier::preorder(IR::IfStatement* ifStatement) {
    HSIndexFinder aiFinder(locals, refMap, typeMap, generatedVariables);
    ifStatement->condition->apply(aiFinder);
    return eliminateArrayIndexes(aiFinder, ifStatement);
}

IR::Node* HSIndexSimplifier::preorder(IR::MethodCallStatement* methodCallStatement) {
    HSIndexFinder aiFinder(locals, refMap, typeMap, generatedVariables);
    methodCallStatement->apply(aiFinder);
    return eliminateArrayIndexes(aiFinder, methodCallStatement);
}

IR::Node* HSIndexSimplifier::preorder(IR::SwitchStatement* switchStatement) {
    HSIndexFinder aiFinder(locals, refMap, typeMap, generatedVariables);
    switchStatement->expression->apply(aiFinder);
    return eliminateArrayIndexes(aiFinder, switchStatement);
}

}  // namespace P4
