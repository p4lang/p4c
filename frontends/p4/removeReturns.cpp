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

const IR::Node* DoRemoveReturns::preorder(IR::P4Action* action) {
    HasExits he;
    (void)action->apply(he);
    if (!he.hasReturns) {
        // don't pollute the code unnecessarily
        prune();
        return action;
    }
    LOG3("Processing " << dbp(action));
    cstring var = refMap->newName(variableName);
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

const IR::Node* DoRemoveReturns::preorder(IR::P4Control* control) {
    visit(control->controlLocals, "controlLocals");

    HasExits he;
    (void)control->body->apply(he);
    if (!he.hasReturns) {
        // don't pollute the code unnecessarily
        prune();
        return control;
    }

    cstring var = refMap->newName(variableName);
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

const IR::Node* DoRemoveReturns::preorder(IR::ReturnStatement* statement) {
    set(Returns::Yes);
    auto left = new IR::PathExpression(returnVar);
    return new IR::AssignmentStatement(statement->srcInfo, left,
                                       new IR::BoolLiteral(true));
    return statement;
}

const IR::Node* DoRemoveReturns::preorder(IR::ExitStatement* statement) {
    set(Returns::Yes);  // exit implies return
    return statement;
}

const IR::Node* DoRemoveReturns::preorder(IR::BlockStatement* statement) {
    auto block = new IR::BlockStatement;
    auto currentBlock = block;
    Returns ret = Returns::No;
    for (auto s : statement->components) {
        push();
        visit(s);
        currentBlock->push_back(s);
        Returns r = hasReturned();
        pop();
        if (r == Returns::Yes) {
            ret = r;
            break;
        } else if (r == Returns::Maybe) {
            auto newBlock = new IR::BlockStatement;
            auto path = new IR::PathExpression(returnVar);
            auto condition = new IR::LNot(path);
            auto ifstat = new IR::IfStatement(condition, newBlock, nullptr);
            block->push_back(ifstat);
            currentBlock = newBlock;
            ret = r;
        }
    }
    if (!stack.empty())
        set(ret);
    prune();
    return block;
}

const IR::Node* DoRemoveReturns::preorder(IR::IfStatement* statement) {
    push();
    visit(statement->ifTrue);
    if (statement->ifTrue == nullptr)
        statement->ifTrue = new IR::EmptyStatement();
    auto rt = hasReturned();
    auto rf = Returns::No;
    pop();
    if (statement->ifFalse != nullptr) {
        push();
        visit(statement->ifFalse);
        rf = hasReturned();
        pop();
    }
    if (rt == Returns::Yes && rf == Returns::Yes)
        set(Returns::Yes);
    else if (rt == Returns::No && rf == Returns::No)
        set(Returns::No);
    else
        set(Returns::Maybe);
    prune();
    return statement;
}

const IR::Node* DoRemoveReturns::preorder(IR::SwitchStatement* statement) {
    auto r = Returns::No;
    push();
    parallel_visit(statement->cases, "cases", 1);
    if (hasReturned() != Returns::No)
        // this is conservative: we don't check if we cover all labels.
        r = Returns::Maybe;
    pop();
    set(r);
    prune();
    return statement;
}

}  // namespace P4
