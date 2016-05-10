#include "removeReturns.h"
#include "frontends/p4/methodInstance.h"

namespace P4 {

namespace {
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

const IR::Node* RemoveReturns::preorder(IR::P4Action* action) {
    HasExits he;
    (void)action->apply(he);
    if (!he.hasReturns) {
        // don't pollute the code unnecessarily
        prune();
        return action;
    }
    cstring var = refMap->newName(variableName);
    returnVar = IR::ID(var);
    auto f = new IR::BoolLiteral(Util::SourceInfo(), false);
    auto decl = new IR::Declaration_Variable(Util::SourceInfo(), returnVar,
                                             IR::Annotations::empty, IR::Type_Boolean::get(), f);
    BUG_CHECK(stack.empty(), "Non-empty stack");
    push();
    visit(action->body);
    auto body = new IR::IndexedVector<IR::StatOrDecl>();
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
    HasExits he;
    (void)control->body->apply(he);
    if (!he.hasReturns) {
        // don't pollute the code unnecessarily
        prune();
        return control;
    }

    cstring var = refMap->newName(variableName);
    returnVar = IR::ID(var);
    auto f = new IR::BoolLiteral(Util::SourceInfo(), false);
    auto decl = new IR::Declaration_Variable(Util::SourceInfo(), returnVar,
                                             IR::Annotations::empty, IR::Type_Boolean::get(), f);
    BUG_CHECK(stack.empty(), "Non-empty stack");
    push();
    visit(control->body);
    auto bodyContents = new IR::IndexedVector<IR::StatOrDecl>();
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
    set(Returns::Yes);
    auto left = new IR::PathExpression(returnVar);
    return new IR::AssignmentStatement(statement->srcInfo, left,
                                       new IR::BoolLiteral(Util::SourceInfo(), true));
    return statement;
}

const IR::Node* RemoveReturns::preorder(IR::ExitStatement* statement) {
    set(Returns::Yes);  // exit implies return
    return statement;
}

const IR::Node* RemoveReturns::preorder(IR::BlockStatement* statement) {
    auto body = new IR::IndexedVector<IR::StatOrDecl>();
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
            auto newBody = new IR::IndexedVector<IR::StatOrDecl>();
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

////////////////////////////////////////////////////////////////

namespace {
class CallsExit : public Inspector {
    ReferenceMap* refMap;
    TypeMap* typeMap;
    std::set<const IR::Node*>* callers;

 public:
    bool callsExit = false;
    CallsExit(ReferenceMap* refMap, TypeMap* typeMap, std::set<const IR::Node*>* callers) :
            refMap(refMap), typeMap(typeMap), callers(callers) {}
    void postorder(const IR::MethodCallExpression* expression) override {
        auto mi = MethodInstance::resolve(expression, refMap, typeMap);
        if (!mi->isApply())
            return;
        auto am = mi->to<ApplyMethod>();
        CHECK_NULL(am->object);
        auto obj = am->object->getNode();
        if (callers->find(obj) != callers->end())
            callsExit = true;
    }
};
}  // namespace

const IR::Node* RemoveExits::preorder(IR::ExitStatement* statement) {
    set(Returns::Yes);
    auto action = findOrigCtxt<IR::P4Action>();
    if (action != nullptr) {
        LOG4(getOriginal() << " calls exit");
        callsExit.emplace(action);
    }
    return statement;
}

const IR::Node* RemoveExits::preorder(IR::P4Table* table) {
    for (auto a : *table->getActionList()->actionList) {
        auto path = a->name;
        auto decl = refMap->getDeclaration(path->path, true);
        BUG_CHECK(decl->is<IR::P4Action>(), "%1% is not an action", decl);
        if (callsExit.find(decl->getNode()) != callsExit.end()) {
            LOG4(getOriginal() << " calls exit");
            callsExit.emplace(getOriginal());
            break;
        }
    }
    return table;
}

const IR::Node* RemoveExits::preorder(IR::P4Action* action) {
    BUG_CHECK(stack.empty(), "Non-empty stack");
    push();
    visit(action->body);
    pop();
    BUG_CHECK(stack.empty(), "Non-empty stack");
    prune();
    return action;
}

const IR::Node* RemoveExits::preorder(IR::P4Control* control) {
    HasExits he;
    (void)control->body->apply(he);
    if (!he.hasReturns) {
        // don't pollute the code unnecessarily
        prune();
        return control;
    }

    cstring var = refMap->newName(variableName);
    returnVar = IR::ID(var);
    auto f = new IR::BoolLiteral(Util::SourceInfo(), false);
    auto decl = new IR::Declaration_Variable(Util::SourceInfo(), returnVar,
                                             IR::Annotations::empty, IR::Type_Boolean::get(), f);
    BUG_CHECK(stack.empty(), "Non-empty stack");
    push();
    visit(control->body);
    auto bodyContents = new IR::IndexedVector<IR::StatOrDecl>();
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

const IR::Node* RemoveExits::preorder(IR::ReturnStatement* statement) {
    set(Returns::Yes);
    auto left = new IR::PathExpression(returnVar);
    return new IR::AssignmentStatement(statement->srcInfo, left,
                                       new IR::BoolLiteral(Util::SourceInfo(), true));
    return statement;
}

const IR::Node* RemoveExits::preorder(IR::BlockStatement* statement) {
    auto body = new IR::IndexedVector<IR::StatOrDecl>();
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
            auto newBody = new IR::IndexedVector<IR::StatOrDecl>();
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

const IR::Node* RemoveExits::preorder(IR::IfStatement* statement) {
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

const IR::Node* RemoveExits::preorder(IR::SwitchStatement* statement) {
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

const IR::Node* RemoveExits::preorder(IR::AssignmentStatement* statement) {
    // TODO
    return statement;
}

const IR::Node* RemoveExits::preorder(IR::MethodCallStatement* statement) {
    // TODO
    return statement;
}

}  // namespace P4
