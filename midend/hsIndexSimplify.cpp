#include "midend/hsIndexSimplify.h"

#include <iostream>
#include <sstream>

#include "midend/simplifyKey.h"

namespace P4 {

void HSIndexFinder::postorder(const IR::ArrayIndex *curArrayIndex) {
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
                              arrayIndex->right->is<IR::MethodCallExpression>() ||
                              arrayIndex->right->is<IR::PathExpression>())) {
        // Generate new local variable if needed.
        std::ostringstream ostr;
        ostr << arrayIndex->right;
        cstring indexString = ostr.str();
        if (arrayIndex->right->is<IR::PathExpression>()) {
            newVariable = arrayIndex->right->to<IR::PathExpression>();
        } else if (generatedVariables->count(indexString) == 0) {
            // Generate new temporary variable.
            const auto *type = typeMap->getTypeType(arrayIndex->right->type, true);
            auto name = nameGen->newName("hsiVar");
            auto *decl = new IR::Declaration_Variable(name, type);
            locals->push_back(decl);
            typeMap->setType(decl, type);
            newVariable = new IR::PathExpression(arrayIndex->srcInfo, type, new IR::Path(name));
            generatedVariables->emplace(indexString, newVariable);
        } else {
            newVariable = generatedVariables->at(indexString);
        }
    }
}

const IR::Node *HSIndexTransform::postorder(IR::ArrayIndex *curArrayIndex) {
    if (hsIndexFinder.arrayIndex != nullptr && curArrayIndex->equiv(*hsIndexFinder.arrayIndex)) {
        // Translating current array index.
        auto *newArrayIndex = hsIndexFinder.arrayIndex->clone();
        newArrayIndex->right = new IR::Constant(newArrayIndex->right->type, index);
        return newArrayIndex;
    }
    return curArrayIndex;
}

int HSIndexContretizer::idCtr;

IR::Node *HSIndexContretizer::eliminateArrayIndexes(HSIndexFinder &aiFinder,
                                                    IR::Statement *statement,
                                                    const IR::Expression *expr) {
    if (aiFinder.arrayIndex == nullptr) {
        return statement;
    }
    const IR::Statement *elseBody = nullptr;
    auto *curIf = statement->to<IR::IfStatement>();
    if (curIf != nullptr) {
        elseBody = curIf->ifFalse;
    }
    IR::IndexedVector<IR::StatOrDecl> newComponents;
    if (aiFinder.newVariable != nullptr) {
        if (!aiFinder.newVariable->equiv(*aiFinder.arrayIndex->right)) {
            newComponents.push_back(new IR::AssignmentStatement(
                aiFinder.arrayIndex->srcInfo, aiFinder.newVariable, aiFinder.arrayIndex->right));
        }
    }
    IR::IfStatement *result = nullptr;
    IR::IfStatement *curResult = nullptr;
    IR::IfStatement *newIf = nullptr;
    const auto *typeStack = aiFinder.arrayIndex->left->type->checkedTo<IR::Type_Stack>();
    size_t sz = typeStack->getSize();
    if ((expansion += (sz - 1)) > maxExpansion) {
        if (expansion - sz < maxExpansion)
            error(ErrorType::ERR_OVERLIMIT, "%1%too much expansion from non-const array indexes",
                  statement->srcInfo);
        return statement;
    }
    LOG5("HSIndexContretizer(" << id << ") expand " << aiFinder.arrayIndex << " by " << sz
                               << ",  expansion = " << expansion);
    for (size_t i = 0; i < sz; i++) {
        HSIndexTransform aiRewriter(aiFinder, i);
        const auto *newStatement = statement->apply(aiRewriter)->to<IR::Statement>();
        auto *newIndex = new IR::Constant(aiFinder.arrayIndex->right->type, i);
        auto *newCondition = new IR::Equ(aiFinder.newVariable, newIndex);
        if (curIf != nullptr) {
            newIf = newStatement->checkedTo<IR::IfStatement>()->clone();
            newIf->condition = new IR::LAnd(newCondition, newIf->condition);
        } else {
            newIf = new IR::IfStatement(newCondition, newStatement, nullptr);
        }
        if (result == nullptr) {
            result = newIf;
        } else {
            curResult->ifFalse = newIf;
        }
        curResult = newIf;
    }
    if (expr != nullptr && locals != nullptr) {
        // Add case for write out of bound.
        cstring typeString = expr->type->toString();
        const IR::PathExpression *pathExpr = nullptr;
        if (generatedVariables->count(typeString) == 0) {
            // Add assignment of undefined header.
            auto name = nameGen->newName("hsVar");
            auto *decl = new IR::Declaration_Variable(name, expr->type);
            locals->push_back(decl);
            typeMap->setType(decl, expr->type);
            pathExpr = new IR::PathExpression(aiFinder.arrayIndex->srcInfo, expr->type,
                                              new IR::Path(name));
            generatedVariables->emplace(typeString, pathExpr);
        } else {
            pathExpr = generatedVariables->at(typeString);
        }
        IR::BaseAssignmentStatement *newStatement;
        if (auto *oldAssign = statement->to<IR::OpAssignmentStatement>()) {
            newStatement = oldAssign->clone();
            newStatement->srcInfo = aiFinder.arrayIndex->srcInfo;
            newStatement->left = expr;
            newStatement->right = pathExpr;
        } else {
            newStatement =
                new IR::AssignmentStatement(aiFinder.arrayIndex->srcInfo, expr, pathExpr);
        }

        auto *newCondition = new IR::Geq(
            aiFinder.newVariable, new IR::Constant(aiFinder.arrayIndex->right->type, sz - 1));
        newIf = new IR::IfStatement(newCondition, newStatement, nullptr);
        curResult->ifFalse = newIf;
    }
    newIf->ifFalse = elseBody;
    newComponents.push_back(result);
    return new IR::BlockStatement(newComponents);
}

IR::Node *HSIndexContretizer::preorder(IR::BaseAssignmentStatement *assignmentStatement) {
    HSIndexFinder aiFinder(locals, nameGen, typeMap, generatedVariables);
    assignmentStatement->left->apply(aiFinder);
    if (aiFinder.arrayIndex == nullptr) {
        assignmentStatement->right->apply(aiFinder);
        return eliminateArrayIndexes(aiFinder, assignmentStatement, assignmentStatement->left);
    }
    return eliminateArrayIndexes(aiFinder, assignmentStatement, nullptr);
}

/**
 * Policy that treats a key as ArrayIndex with non constant index.
 */
class IsNonConstantArrayIndex : public KeyIsSimple, public Inspector {
 protected:
    bool simple = false;

 public:
    IsNonConstantArrayIndex() { setName("IsNonConstantArrayIndex"); }

    void postorder(const IR::ArrayIndex *arrayIndex) override {
        if (simple) {
            simple = arrayIndex->right->is<IR::Constant>();
        }
    }
    profile_t init_apply(const IR::Node *root) override {
        simple = true;
        return Inspector::init_apply(root);
    }

    bool isSimple(const IR::Expression *expression, const Visitor::Context * /*ctx*/) override {
        (void)expression->apply(*this);
        return simple;
    }
};

IR::Node *HSIndexContretizer::preorder(IR::P4Control *control) {
    DoSimplifyKey keySimplifier(typeMap, new IsNonConstantArrayIndex());
    const auto *controlKeySimplified =
        control->apply(keySimplifier, getContext())->to<IR::P4Control>();
    auto *newControl = controlKeySimplified->clone();
    IR::IndexedVector<IR::Declaration> newControlLocals;
    GeneratedVariablesMap blockGeneratedVariables;
    HSIndexContretizer hsSimplifier(typeMap, maxExpansion, nameGen, &newControlLocals,
                                    &blockGeneratedVariables);
    newControl->body = newControl->body->apply(hsSimplifier)->to<IR::BlockStatement>();
    for (const auto *declaration : controlKeySimplified->controlLocals) {
        if (declaration->is<IR::P4Action>()) {
            newControlLocals.push_back(declaration->apply(hsSimplifier)->to<IR::Declaration>());
        } else {
            newControlLocals.push_back(declaration);
        }
    }
    newControl->controlLocals = newControlLocals;
    prune();
    return newControl;
}

IR::Node *HSIndexContretizer::preorder(IR::P4Parser *parser) {
    prune();
    return parser;
}

IR::Node *HSIndexContretizer::preorder(IR::BlockStatement *blockStatement) {
    HSIndexFinder aiFinder(locals, nameGen, typeMap, generatedVariables);
    blockStatement->apply(aiFinder);
    if (aiFinder.arrayIndex == nullptr) {
        return blockStatement;
    }
    HSIndexContretizer hsSimplifier(typeMap, maxExpansion, nameGen, locals, generatedVariables);
    hsSimplifier.expansion = expansion;
    auto *newBlock = blockStatement->clone();
    IR::IndexedVector<IR::StatOrDecl> newComponents;
    for (auto &component : blockStatement->components) {
        const auto *newComponent = component->apply(hsSimplifier)->to<IR::StatOrDecl>();
        if (const auto *newComponentBlock = newComponent->to<IR::BlockStatement>()) {
            for (const auto &blockComponent : newComponentBlock->components) {
                newComponents.push_back(blockComponent);
            }
        } else {
            newComponents.push_back(newComponent);
        }
    }
    newBlock->components = newComponents;
    expansion = hsSimplifier.expansion;
    return newBlock;
}

IR::Node *HSIndexContretizer::preorder(IR::IfStatement *ifStatement) {
    HSIndexFinder aiFinder(locals, nameGen, typeMap, generatedVariables);
    ifStatement->condition->apply(aiFinder);
    return eliminateArrayIndexes(aiFinder, ifStatement, nullptr);
}

IR::Node *HSIndexContretizer::preorder(IR::MethodCallStatement *methodCallStatement) {
    HSIndexFinder aiFinder(locals, nameGen, typeMap, generatedVariables);
    methodCallStatement->apply(aiFinder);
    // Here we mean that in/out parameter will be replaced by correspondent assignments.
    // In this case no need to consider assignment to undefined value.
    return eliminateArrayIndexes(aiFinder, methodCallStatement, nullptr);
}

IR::Node *HSIndexContretizer::preorder(IR::SwitchStatement *switchStatement) {
    HSIndexFinder aiFinder(locals, nameGen, typeMap, generatedVariables);
    switchStatement->expression->apply(aiFinder);
    return eliminateArrayIndexes(aiFinder, switchStatement, nullptr);
}

}  // namespace P4
