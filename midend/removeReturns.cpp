#include "removeReturns.h"

namespace P4 {

const IR::Node* RemoveReturns::preorder(IR::P4Action* action) {
    cstring var = refMap->newName("hasReturned");
    returnVar = IR::ID(var);
    auto f = new IR::BoolLiteral(Util::SourceInfo(), false);
    auto decl = new IR::Declaration_Variable(Util::SourceInfo(), returnVar,
                                             IR::Annotations::empty, IR::Type_Boolean::get(), f);
    BUG_CHECK(stack.empty(), "Non-empty stack");
    push();
    visit(action->body);
    auto body = new IR::Vector<IR::StatOrDecl>();
    body->push_back(decl);
    body->append(*action->body);
    auto result = new IR::P4Action(action->srcInfo, action->name, action->annotations,
                                   action->parameters, body);
    pop();
    BUG_CHECK(stack.empty(), "Non-empty stack");
    prune();
    return result;
}

const IR::Node* RemoveReturns::preorder(IR::P4Control* control) {
    cstring base = removeReturns ? "hasReturned" : "hasExited";
    cstring var = refMap->newName(base);
    returnVar = IR::ID(var);
    auto f = new IR::BoolLiteral(Util::SourceInfo(), false);
    auto decl = new IR::Declaration_Variable(Util::SourceInfo(), returnVar,
                                             IR::Annotations::empty, IR::Type_Boolean::get(), f);
    BUG_CHECK(stack.empty(), "Non-empty stack");
    push();
    visit(control->body);
    auto bodyContents = new IR::Vector<IR::StatOrDecl>();
    bodyContents->push_back(decl);
    bodyContents->append(*control->body->components);
    auto body = new IR::BlockStatement(control->body->srcInfo, bodyContents);
    auto result = new IR::P4Control(control->srcInfo, control->name, control->type,
                                    control->constructorParams, control->stateful, body);
    pop();
    BUG_CHECK(stack.empty(), "Non-empty stack");
    prune();
    return result;
}

const IR::Node* RemoveReturns::preorder(IR::ReturnStatement* statement) {
    if (removeReturns) {
        set(Returns::Yes);
        auto left = new IR::PathExpression(returnVar);
        return new IR::AssignmentStatement(statement->srcInfo, left,
                                           new IR::BoolLiteral(Util::SourceInfo(), true));
    }
    return statement;
}

const IR::Node* RemoveReturns::preorder(IR::ExitStatement* statement) {
    set(Returns::Yes);
    if (!removeReturns) {
        auto left = new IR::PathExpression(returnVar);
        return new IR::AssignmentStatement(statement->srcInfo, left,
                                           new IR::BoolLiteral(Util::SourceInfo(), true));
    }
    return statement;
}

const IR::Node* RemoveReturns::preorder(IR::BlockStatement* statement) {
    auto body = new IR::Vector<IR::StatOrDecl>();
    auto currentBody = body;
    Returns ret = Returns::No;
    for (auto s : *statement->components) {
        push();
        visit(s);
        currentBody->push_back(s);
        Returns r = hasReturned();
        pop();
        if (r == Returns::Yes) {
            ret = r;
            break;
        } else if (r == Returns::Maybe) {
            auto newBody = new IR::Vector<IR::StatOrDecl>();
            auto path = new IR::PathExpression(returnVar);
            auto condition = new IR::LNot(Util::SourceInfo(), path);
            auto newBlock = new IR::BlockStatement(Util::SourceInfo(), newBody);
            auto ifstat = new IR::IfStatement(Util::SourceInfo(), condition, newBlock, nullptr);
            body->push_back(ifstat);
            currentBody = newBody;
            ret = r;
        }
    }
    set(ret);
    prune();
    return new IR::BlockStatement(statement->srcInfo, body);
}

const IR::Node* RemoveReturns::preorder(IR::IfStatement* statement) {
    push();
    visit(statement->ifTrue);
    if (statement->ifTrue == nullptr)
        statement->ifTrue = new IR::EmptyStatement(Util::SourceInfo());
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

const IR::Node* RemoveReturns::preorder(IR::SwitchStatement* statement) {
    auto r = Returns::No;
    for (auto c : *statement->cases) {
        push();
        visit(c);
        if (hasReturned() != Returns::No)
            // this is conservative: we don't check if we cover all labels.
            r = Returns::Maybe;
        pop();
    }
    set(r);
    prune();
    return statement;
}

}  // namespace P4
