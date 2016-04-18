#include "removeReturns.h"

namespace P4 {

const IR::Node* RemoveReturns::preorder(IR::P4Action* action) {
    prune();
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
    return result;
}

const IR::Node* RemoveReturns::preorder(IR::P4Control* control) {
    prune();
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
    return result;
}

const IR::Node* RemoveReturns::preorder(IR::ReturnStatement* statement) {
    if (removeReturns) {
        setReturned();
        auto left = new IR::PathExpression(returnVar);
        return new IR::AssignmentStatement(statement->srcInfo, left,
                                           new IR::BoolLiteral(Util::SourceInfo(), true));
    }
    return statement;
}

const IR::Node* RemoveReturns::preorder(IR::ExitStatement* statement) {
    if (!removeReturns) {
        setReturned();
        auto left = new IR::PathExpression(returnVar);
        return new IR::AssignmentStatement(statement->srcInfo, left,
                                           new IR::BoolLiteral(Util::SourceInfo(), true));
    }
    return statement;
}

const IR::Node* RemoveReturns::preorder(IR::BlockStatement* statement) {
    prune();
    push();
    auto body = new IR::Vector<IR::StatOrDecl>();
    auto currentBody = body;
    bool may = false;
    for (auto s : *statement->components) {
        push();
        visit(s);
        currentBody->push_back(s);
        bool m = mayHaveReturned();
        pop();
        if (m) {
            may = true;
            auto newBody = new IR::Vector<IR::StatOrDecl>();
            auto path = new IR::PathExpression(returnVar);
            auto condition = new IR::LNot(Util::SourceInfo(), path);
            auto newBlock = new IR::BlockStatement(Util::SourceInfo(), newBody);
            auto ifstat = new IR::IfStatement(Util::SourceInfo(), condition, newBlock, nullptr);
            body->push_back(ifstat);
            currentBody = newBody;
        }
    }
    pop();
    if (may)
        setReturned();
    return new IR::BlockStatement(statement->srcInfo, body);
}

const IR::Node* RemoveReturns::preorder(IR::IfStatement* statement) {
    prune();
    push();
    visit(statement->ifTrue);
    if (statement->ifTrue == nullptr)
        statement->ifTrue = new IR::EmptyStatement(Util::SourceInfo());
    bool may = mayHaveReturned();
    pop();
    if (statement->ifFalse != nullptr) {
        push();
        visit(statement->ifFalse);
        may = may | mayHaveReturned();
        pop();
    }
    if (may) setReturned();
    return statement;
}

const IR::Node* RemoveReturns::preorder(IR::SwitchStatement* statement) {
    prune();
    bool may = false;
    for (auto c : *statement->cases) {
        push();
        visit(c);
        may = may | mayHaveReturned();
        pop();
    }
    if (may) setReturned();
    return statement;
}

}  // namespace P4
