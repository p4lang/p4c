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

#include "removeReturns.h"

#include "frontends/p4/methodInstance.h"

namespace P4 {

bool MoveToElseAfterBranch::preorder(IR::BlockStatement *block) {
    movedToIfBranch = false;
    for (auto it = block->components.begin(); it != block->components.end();) {
        if (movedToIfBranch)
            it = block->components.erase(it);
        else
            visit(*it++);
    }
    movedToIfBranch = false;
    return false;
}

bool MoveToElseAfterBranch::moveFromParentTo(const IR::Statement *&child) {
    auto parent = getParent<IR::BlockStatement>();
    size_t next = getContext()->child_index + 1;
    if (!parent || next >= parent->components.size()) {
        // nothing to move from the parent, so nothing to do
        return false;
    }

    IR::BlockStatement *modified = nullptr;
    if (!child)
        modified = new IR::BlockStatement;
    else if (auto *t = child->to<IR::BlockStatement>())
        modified = t->clone();
    else if (child->is<IR::EmptyStatement>())
        modified = new IR::BlockStatement;
    else
        modified = new IR::BlockStatement({child});
    for (auto it = parent->components.begin() + next; it != parent->components.end(); ++it)
        modified->components.push_back(*it);
    child = modified;
    return true;
}

bool MoveToElseAfterBranch::preorder(IR::IfStatement *ifStmt) {
    hasJumped = false;
    bool movedCode = false;
    visit(ifStmt->ifTrue, "ifTrue", 1);
    bool jumpInIfTrue = hasJumped;
    if (hasJumped) movedCode = moveFromParentTo(ifStmt->ifFalse);
    hasJumped = false;
    visit(ifStmt->ifFalse, "ifFalse", 2);
    if (hasJumped && !jumpInIfTrue) {
        movedCode = moveFromParentTo(ifStmt->ifTrue);
        // need to revisit to visit the copied code; will autoamtically skip nodes we've
        // already visited in Modifier::apply_visiter as visitDagOnce == true
        visit(ifStmt->ifTrue, "ifTrue", 1);
    }
    hasJumped &= jumpInIfTrue;
    movedToIfBranch |= movedCode;
    return false;
}

bool MoveToElseAfterBranch::preorder(IR::SwitchStatement *swch) {
    // TBD: if there is exactly one case that falls through (all others end with a branch)
    // then we could move subsequent code into that case, as it done with 'if'
    bool canFallThrough = false;
    for (auto &c : swch->cases) {
        hasJumped = false;
        visit(c, "cases");
        canFallThrough |= !hasJumped;
    }
    hasJumped = !canFallThrough;
    return false;
}

void MoveToElseAfterBranch::postorder(IR::LoopStatement *) {
    // after a loop body is never unreachable
    hasJumped = false;
}

const IR::Node *DoRemoveReturns::preorder(IR::P4Action *action) {
    HasExits he;
    he.setCalledBy(this);
    (void)action->apply(he);
    if (!he.hasReturns) {
        // don't pollute the code unnecessarily
        prune();
        return action;
    }
    LOG3("Processing " << dbp(action));
    cstring var = nameGen.newName(variableName.string_view());
    returnVar = IR::ID(var, nullptr);
    auto f = new IR::BoolLiteral(false);
    auto decl = new IR::Declaration_Variable(returnVar, IR::Type_Boolean::get(), f);
    BUG_CHECK(stack.empty(), "Non-empty stack");
    push();
    visit(action->body);
    auto body = new IR::BlockStatement(action->body->srcInfo, action->body->annotations);
    body->push_back(decl);
    body->components.append(action->body->components);
    auto result = new IR::P4Action(action->srcInfo, action->name, action->annotations,
                                   action->parameters, body);
    pop();
    BUG_CHECK(stack.empty(), "Non-empty stack");
    prune();
    return result;
}

const IR::Node *DoRemoveReturns::preorder(IR::Function *function) {
    // We leave returns in abstract functions alone
    if (findContext<IR::Declaration_Instance>()) {
        prune();
        return function;
    }

    HasExits he;
    he.setCalledBy(this);
    (void)function->apply(he);
    if (!he.hasReturns) {
        // don't pollute the code unnecessarily
        prune();
        return function;
    }

    bool returnsVal =
        function->type->returnType != nullptr && !function->type->returnType->is<IR::Type_Void>();

    cstring var = nameGen.newName(variableName.string_view());
    returnVar = IR::ID(var, nullptr);
    IR::Declaration_Variable *retvalDecl = nullptr;
    if (returnsVal) {
        var = nameGen.newName(retValName.string_view());
        returnedValue = IR::ID(var, nullptr);
        retvalDecl = new IR::Declaration_Variable(returnedValue, function->type->returnType);
    }
    auto f = new IR::BoolLiteral(false);
    auto decl = new IR::Declaration_Variable(returnVar, IR::Type_Boolean::get(), f);
    BUG_CHECK(stack.empty(), "Non-empty stack");
    push();
    visit(function->body);
    auto body = new IR::BlockStatement(function->body->srcInfo, function->body->annotations);
    body->push_back(decl);
    if (retvalDecl != nullptr) body->push_back(retvalDecl);
    body->components.append(function->body->components);
    if (returnsVal) body->push_back(new IR::ReturnStatement(new IR::PathExpression(returnedValue)));
    auto result = new IR::Function(function->srcInfo, function->name, function->annotations,
                                   function->type, body);
    pop();
    BUG_CHECK(stack.empty(), "Non-empty stack");
    prune();
    return result;
}

const IR::Node *DoRemoveReturns::preorder(IR::P4Control *control) {
    visit(control->controlLocals, "controlLocals");

    HasExits he;
    he.setCalledBy(this);
    (void)control->body->apply(he);
    if (!he.hasReturns) {
        // don't pollute the code unnecessarily
        prune();
        return control;
    }

    cstring var = nameGen.newName(variableName.string_view());
    returnVar = IR::ID(var, nullptr);
    auto f = new IR::BoolLiteral(false);
    auto decl = new IR::Declaration_Variable(returnVar, IR::Type_Boolean::get(), f);
    BUG_CHECK(stack.empty(), "Non-empty stack");
    push();
    visit(control->body);
    auto body = new IR::BlockStatement(control->body->srcInfo, control->body->annotations);
    body->push_back(decl);
    body->components.append(control->body->components);
    auto result = new IR::P4Control(control->srcInfo, control->name, control->type,
                                    control->constructorParams, control->controlLocals, body);
    pop();
    BUG_CHECK(stack.empty(), "Non-empty stack");
    prune();
    return result;
}

const IR::Node *DoRemoveReturns::preorder(IR::ReturnStatement *statement) {
    set(TernaryBool::Yes);
    auto vec = new IR::IndexedVector<IR::StatOrDecl>();

    auto left = new IR::PathExpression(IR::Type::Boolean::get(), returnVar);
    vec->push_back(
        new IR::AssignmentStatement(statement->srcInfo, left, new IR::BoolLiteral(true)));
    if (statement->expression != nullptr) {
        left = new IR::PathExpression(statement->expression->type, returnedValue);
        vec->push_back(
            new IR::AssignmentStatement(statement->srcInfo, left, statement->expression));
    }
    if (findContext<IR::LoopStatement>()) vec->push_back(new IR::BreakStatement);
    return new IR::BlockStatement(*vec);
}

const IR::Node *DoRemoveReturns::preorder(IR::ExitStatement *statement) {
    set(TernaryBool::Yes);  // exit implies return
    return statement;
}

const IR::Node *DoRemoveReturns::preorder(IR::BlockStatement *statement) {
    auto block = new IR::BlockStatement;
    auto currentBlock = block;
    TernaryBool ret = TernaryBool::No;
    for (auto s : statement->components) {
        push();
        visit(s);
        currentBlock->push_back(s);
        TernaryBool r = hasReturned();
        pop();
        if (r == TernaryBool::Yes) {
            ret = r;
            break;
        } else if (r == TernaryBool::Maybe) {
            auto newBlock = new IR::BlockStatement;
            auto path = new IR::PathExpression(IR::Type::Boolean::get(), returnVar);
            auto condition = new IR::LNot(path);
            auto ifstat = new IR::IfStatement(condition, newBlock, nullptr);
            block->push_back(ifstat);
            currentBlock = newBlock;
            ret = r;
        }
    }
    if (!stack.empty()) set(ret);
    prune();
    return block;
}

const IR::Node *DoRemoveReturns::preorder(IR::IfStatement *statement) {
    push();
    visit(statement->ifTrue);
    if (statement->ifTrue == nullptr) statement->ifTrue = new IR::EmptyStatement();
    auto rt = hasReturned();
    auto rf = TernaryBool::No;
    pop();
    if (statement->ifFalse != nullptr) {
        push();
        visit(statement->ifFalse);
        rf = hasReturned();
        pop();
    }
    if (rt == TernaryBool::Yes && rf == TernaryBool::Yes)
        set(TernaryBool::Yes);
    else if (rt == TernaryBool::No && rf == TernaryBool::No)
        set(TernaryBool::No);
    else
        set(TernaryBool::Maybe);
    prune();
    return statement;
}

const IR::Node *DoRemoveReturns::preorder(IR::SwitchStatement *statement) {
    auto r = TernaryBool::No;
    push();
    for (auto &swCase : statement->cases) {
        push();
        visit(swCase);
        if (hasReturned() != TernaryBool::No)
            // this is conservative: we don't check if we cover all labels.
            r = TernaryBool::Maybe;
        pop();
    }
    pop();
    set(r);
    prune();
    return statement;
}

const IR::Node *DoRemoveReturns::postorder(IR::LoopStatement *loop) {
    // loop body might not (all) execute, so can't be certain if it returns
    if (hasReturned() == TernaryBool::Yes) set(TernaryBool::Maybe);

    // only need to add an extra check for nested loops
    if (!findContext<IR::LoopStatement>()) return loop;
    // only if the inner loop may have returned
    if (hasReturned() == TernaryBool::No) return loop;

    // break out of the outer loop if the inner loop returned
    auto rv = new IR::BlockStatement();
    rv->push_back(loop);
    rv->push_back(new IR::IfStatement(new IR::PathExpression(IR::Type::Boolean::get(), returnVar),
                                      new IR::BreakStatement(), nullptr));
    return rv;
}

Visitor::profile_t DoRemoveReturns::init_apply(const IR::Node *node) {
    auto rv = Transform::init_apply(node);
    node->apply(nameGen);

    return rv;
}

}  // namespace P4
