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

namespace {
/**
This inspector detects whether an IR tree contains
'return' or 'exit' statements.
It sets a boolean flag for each of them.

It treats exceptionally Functions - it claims that
returns do not exist in Functions.
*/
class HasExits : public Inspector {
 public:
    bool hasExits;
    bool hasReturns;
    HasExits() : hasExits(false), hasReturns(false) {}

    bool preorder(const IR::Function*) override
    { return false; }
    void postorder(const IR::ExitStatement*) override
    { hasExits = true; }
    void postorder(const IR::ReturnStatement*) override
    { hasReturns = true; }
};
}  // namespace

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

////////////////////////////////////////////////////////////////

namespace {
class CallsExit : public Inspector {
    ReferenceMap*        refMap;
    TypeMap*             typeMap;
    std::set<const IR::Node*>* callers;

 public:
    bool callsExit = false;
    CallsExit(ReferenceMap* refMap, TypeMap* typeMap,
              std::set<const IR::Node*>* callers) :
            refMap(refMap), typeMap(typeMap), callers(callers) {}
    void postorder(const IR::MethodCallExpression* expression) override {
        auto mi = MethodInstance::resolve(expression, refMap, typeMap);
        if (mi->isApply()) {
            auto am = mi->to<ApplyMethod>();
            CHECK_NULL(am->object);
            auto obj = am->object->getNode();
            if (callers->find(obj) != callers->end())
                callsExit = true;
        } else if (mi->is<ActionCall>()) {
            auto ac = mi->to<ActionCall>();
            if (callers->find(ac->action) != callers->end())
                callsExit = true;
        }
    }
    void end_apply(const IR::Node* node) override {
        LOG3(node << (callsExit ? " calls " : " does not call ") << "exit");
    }
};
}  // namespace

void DoRemoveExits::callExit(const IR::Node* node) {
    LOG3(node << " calls exit");
    callsExit.emplace(node);
}

const IR::Node* DoRemoveExits::preorder(IR::ExitStatement* statement) {
    set(Returns::Yes);
    auto left = new IR::PathExpression(returnVar);
    return new IR::AssignmentStatement(statement->srcInfo, left,
                                       new IR::BoolLiteral(true));
}

const IR::Node* DoRemoveExits::preorder(IR::P4Table* table) {
    for (auto a : table->getActionList()->actionList) {
        auto path = a->getPath();
        auto decl = refMap->getDeclaration(path, true);
        BUG_CHECK(decl->is<IR::P4Action>(), "%1% is not an action", decl);
        if (callsExit.find(decl->getNode()) != callsExit.end()) {
            callExit(getOriginal());
            callExit(table);
            break;
        }
    }
    return table;
}

const IR::Node* DoRemoveExits::preorder(IR::P4Action* action) {
    LOG3("Visiting " << action);
    push();
    visit(action->body);
    auto r = hasReturned();
    if (r != Returns::No) {
        callExit(action);
        callExit(getOriginal());
    }
    pop();
    prune();
    return action;
}

const IR::Node* DoRemoveExits::preorder(IR::P4Control* control) {
    HasExits he;
    (void)control->apply(he);
    if (!he.hasExits) {
        // don't pollute the code unnecessarily
        prune();
        return control;
    }

    cstring var = refMap->newName(variableName);
    returnVar = IR::ID(var, nullptr);
    visit(control->controlLocals, "controlLocals");

    BUG_CHECK(stack.empty(), "Non-empty stack");
    push();
    visit(control->body);
    IR::IndexedVector<IR::Declaration> stateful;
    auto decl = new IR::Declaration_Variable(returnVar, IR::Type_Boolean::get(), nullptr);
    stateful.push_back(decl);
    stateful.append(control->controlLocals);
    control->controlLocals = stateful;

    IR::IndexedVector<IR::StatOrDecl> newbody;
    auto left = new IR::PathExpression(returnVar);
    auto init = new IR::AssignmentStatement(left, new IR::BoolLiteral(false));
    newbody.push_back(init);
    newbody.append(control->body->components);
    control->body = new IR::BlockStatement(
        control->body->srcInfo, control->body->annotations, newbody);

    pop();
    BUG_CHECK(stack.empty(), "Non-empty stack");
    prune();
    return control;
}

const IR::Node* DoRemoveExits::preorder(IR::BlockStatement* statement) {
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

// if (t.apply.hit()) stat1;
// becomes
// if (t.apply().hit()) if (!hasExited) stat1;
const IR::Node* DoRemoveExits::preorder(IR::IfStatement* statement) {
    push();

    CallsExit ce(refMap, typeMap, &callsExit);
    (void)statement->condition->apply(ce);
    auto rcond = ce.callsExit ? Returns::Maybe : Returns::No;

    visit(statement->ifTrue);
    if (statement->ifTrue == nullptr)
        statement->ifTrue = new IR::EmptyStatement();
    if (ce.callsExit) {
        auto path = new IR::PathExpression(returnVar);
        auto condition = new IR::LNot(path);
        auto newif = new IR::IfStatement(condition, statement->ifTrue, nullptr);
        statement->ifTrue = newif;
    }
    auto rt = hasReturned();
    auto rf = Returns::No;
    pop();
    if (statement->ifFalse != nullptr) {
        push();
        visit(statement->ifFalse);
        if (ce.callsExit && statement->ifFalse != nullptr) {
            auto path = new IR::PathExpression(returnVar);
            auto condition = new IR::LNot(path);
            auto newif = new IR::IfStatement(condition, statement->ifFalse, nullptr);
            statement->ifFalse = newif;
        }
        rf = hasReturned();
        pop();
    }
    if (rcond == Returns::Yes || (rt == Returns::Yes && rf == Returns::Yes))
        set(Returns::Yes);
    else if (rcond == Returns::No && rt == Returns::No && rf == Returns::No)
        set(Returns::No);
    else
        set(Returns::Maybe);
    prune();
    return statement;
}

const IR::Node* DoRemoveExits::preorder(IR::SwitchStatement* statement) {
    auto r = Returns::No;
    CallsExit ce(refMap, typeMap, &callsExit);
    (void)statement->expression->apply(ce);

    /* FIXME -- alter cases in place rather than allocating a new Vector */
    IR::Vector<IR::SwitchCase> *cases = nullptr;
    if (ce.callsExit) {
        r = Returns::Maybe;
        cases = new IR::Vector<IR::SwitchCase>();
    }
    for (auto &c : statement->cases) {
        push();
        visit(c);
        if (hasReturned() != Returns::No)
            // this is conservative: we don't check if we cover all labels.
            r = Returns::Maybe;
        if (cases != nullptr) {
            IR::Statement* stat = nullptr;
            if (c->statement != nullptr) {
                auto path = new IR::PathExpression(returnVar);
                auto condition = new IR::LNot(path);
                auto newif = new IR::IfStatement(condition, c->statement, nullptr);
                stat = new IR::BlockStatement(newif->srcInfo, {newif});
            }
            auto swcase = new IR::SwitchCase(c->srcInfo, c->label, stat);
            cases->push_back(swcase);
        }
        pop();
    }
    set(r);
    prune();
    if (cases != nullptr)
        statement->cases = std::move(*cases);
    return statement;
}

const IR::Node* DoRemoveExits::preorder(IR::AssignmentStatement* statement) {
    CallsExit ce(refMap, typeMap, &callsExit);
    (void)statement->apply(ce);
    if (ce.callsExit)
        set(Returns::Maybe);
    return statement;
}

const IR::Node* DoRemoveExits::preorder(IR::MethodCallStatement* statement) {
    CallsExit ce(refMap, typeMap, &callsExit);
    (void)statement->apply(ce);
    if (ce.callsExit)
        set(Returns::Maybe);
    return statement;
}

}  // namespace P4
