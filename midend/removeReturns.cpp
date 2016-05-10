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
    body->append(*action->body->components);
    auto result = new IR::P4Action(action->srcInfo, action->name, action->annotations,
                                   action->parameters,
                                   new IR::BlockStatement(action->body->srcInfo, body));
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
    push();
    visit(statement->cases);
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
    ReferenceMap* refMap;
    TypeMap* typeMap;
    std::set<const IR::Node*>* callers;

 public:
    bool callsExit = false;
    CallsExit(ReferenceMap* refMap, TypeMap* typeMap, std::set<const IR::Node*>* callers) :
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

void RemoveExits::callExit(const IR::Node* node) {
    LOG3(node << " calls exit");
    callsExit.emplace(node);
}

const IR::Node* RemoveExits::preorder(IR::ExitStatement* statement) {
    set(Returns::Yes);
    auto left = new IR::PathExpression(returnVar);
    return new IR::AssignmentStatement(statement->srcInfo, left,
                                       new IR::BoolLiteral(Util::SourceInfo(), true));
}

const IR::Node* RemoveExits::preorder(IR::P4Table* table) {
    for (auto a : *table->getActionList()->actionList) {
        auto path = a->name;
        auto decl = refMap->getDeclaration(path->path, true);
        BUG_CHECK(decl->is<IR::P4Action>(), "%1% is not an action", decl);
        if (callsExit.find(decl->getNode()) != callsExit.end()) {
            callExit(getOriginal());
            callExit(table);
            break;
        }
    }
    return table;
}

const IR::Node* RemoveExits::preorder(IR::P4Action* action) {
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

const IR::Node* RemoveExits::preorder(IR::P4Control* control) {
    HasExits he;
    (void)control->apply(he);
    if (!he.hasExits) {
        // don't pollute the code unnecessarily
        prune();
        return control;
    }

    cstring var = refMap->newName(variableName);
    returnVar = IR::ID(var);
    visit(control->stateful);

    BUG_CHECK(stack.empty(), "Non-empty stack");
    push();
    visit(control->body);
    auto stateful = new IR::IndexedVector<IR::Declaration>();
    auto decl = new IR::Declaration_Variable(Util::SourceInfo(), returnVar,
                                             IR::Annotations::empty, IR::Type_Boolean::get(), nullptr);
    stateful->push_back(decl);
    stateful->append(*control->stateful);
    control->stateful = stateful;

    auto newbody = new IR::IndexedVector<IR::StatOrDecl>();
    auto left = new IR::PathExpression(returnVar);
    auto init = new IR::AssignmentStatement(Util::SourceInfo(), left,
                                            new IR::BoolLiteral(Util::SourceInfo(), false));
    newbody->push_back(init);
    newbody->append(*control->body->components);
    control->body = new IR::BlockStatement(control->body->srcInfo, newbody);

    pop();
    BUG_CHECK(stack.empty(), "Non-empty stack");
    prune();
    return control;
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

// if (t.apply.hit()) stat1;
// becomes
// if (t.apply().hit()) if (!hasExited) stat1;
const IR::Node* RemoveExits::preorder(IR::IfStatement* statement) {
    push();

    CallsExit ce(refMap, typeMap, &callsExit);
    (void)statement->condition->apply(ce);
    auto rcond = ce.callsExit ? Returns::Maybe : Returns::No;

    visit(statement->ifTrue);
    if (statement->ifTrue == nullptr)
        statement->ifTrue = new IR::EmptyStatement(Util::SourceInfo());
    if (ce.callsExit) {
        auto path = new IR::PathExpression(returnVar);
        auto condition = new IR::LNot(Util::SourceInfo(), path);
        auto newif = new IR::IfStatement(Util::SourceInfo(), condition, statement->ifTrue, nullptr);
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
            auto condition = new IR::LNot(Util::SourceInfo(), path);
            auto newif = new IR::IfStatement(Util::SourceInfo(), condition, statement->ifFalse, nullptr);
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

const IR::Node* RemoveExits::preorder(IR::SwitchStatement* statement) {
    auto r = Returns::No;
    CallsExit ce(refMap, typeMap, &callsExit);
    (void)statement->expression->apply(ce);

    IR::Vector<IR::SwitchCase> *cases = nullptr;
    if (ce.callsExit) {
        r = Returns::Maybe;
        cases = new IR::Vector<IR::SwitchCase>();
    }
    for (auto c : *statement->cases) {
        push();
        visit(c);
        if (hasReturned() != Returns::No)
            // this is conservative: we don't check if we cover all labels.
            r = Returns::Maybe;
        if (cases != nullptr) {
            IR::Statement* stat = nullptr;
            if (c->statement != nullptr) {
                auto path = new IR::PathExpression(returnVar);
                auto condition = new IR::LNot(Util::SourceInfo(), path);
                auto newif = new IR::IfStatement(Util::SourceInfo(), condition,
                                                 c->statement, nullptr);
                auto vec = new IR::IndexedVector<IR::StatOrDecl>();
                vec->push_back(newif);
                stat = new IR::BlockStatement(newif->srcInfo, vec);
            }
            auto swcase = new IR::SwitchCase(c->srcInfo, c->label, stat);
            cases->push_back(swcase);
        }
        pop();
    }
    set(r);
    prune();
    if (cases != nullptr)
        statement->cases = cases;
    return statement;
}

const IR::Node* RemoveExits::preorder(IR::AssignmentStatement* statement) {
    CallsExit ce(refMap, typeMap, &callsExit);
    (void)statement->apply(ce);
    if (ce.callsExit)
        set(Returns::Maybe);
    return statement;
}

const IR::Node* RemoveExits::preorder(IR::MethodCallStatement* statement) {
    CallsExit ce(refMap, typeMap, &callsExit);
    (void)statement->apply(ce);
    if (ce.callsExit)
        set(Returns::Maybe);
    return statement;
}

}  // namespace P4
