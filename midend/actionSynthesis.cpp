#include "actionSynthesis.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/coreLibrary.h"

namespace P4 {

const IR::Node* MoveActionsToTables::postorder(IR::MethodCallStatement* statement) {
    auto mi = MethodInstance::resolve(statement, refMap, typeMap);
    if (!mi->is<ActionCall>())
        return statement;
    auto ac = mi->to<ActionCall>();

    // Action invocation
    BUG_CHECK(ac->expr->method->is<IR::PathExpression>(),
              "%1%: Expected a PathExpression", ac->expr->method);
    auto actionpath = new IR::PathExpression(ac->action->name);
    auto actinst = new IR::ActionListElement(statement->srcInfo,
                                             IR::Annotations::empty,
                                             actionpath, nullptr);
    auto actions = new IR::Vector<IR::ActionListElement>();
    actions->push_back(actinst);
    // Action list property
    auto actlist = new IR::ActionList(Util::SourceInfo(), actions);
    auto prop = new IR::TableProperty(
        Util::SourceInfo(),
        IR::ID(IR::TableProperties::actionsPropertyName),
        IR::Annotations::empty, actlist, false);
    // default action property
    auto defactval = new IR::ExpressionValue(Util::SourceInfo(), statement->methodCall);
    auto defprop = new IR::TableProperty(
        Util::SourceInfo(), IR::ID(IR::TableProperties::defaultActionPropertyName),
        IR::Annotations::empty, defactval, true);

    // List of table properties
    auto nm = new IR::NameMap<IR::TableProperty, ordered_map>();
    nm->addUnique(prop->name, prop);
    nm->addUnique(defprop->name, defprop);
    auto props = new IR::TableProperties(Util::SourceInfo(), std::move(*nm));
    // Synthesize a new table
    cstring tblName = IR::ID(refMap->newName(cstring("tbl_") + ac->action->name.name));
    auto tbl = new IR::P4Table(Util::SourceInfo(), tblName, IR::Annotations::empty,
                               new IR::ParameterList(), props);
    tables.push_back(tbl);

    // Table invocation statement
    auto tblpath = new IR::PathExpression(tblName);
    auto method = new IR::Member(Util::SourceInfo(), tblpath, IR::IApply::applyMethodName);
    auto mce = new IR::MethodCallExpression(
        statement->srcInfo, method, new IR::Vector<IR::Type>(),
        new IR::Vector<IR::Expression>());
    auto stat = new IR::MethodCallStatement(mce->srcInfo, mce);
    return stat;
}

const IR::Node* MoveActionsToTables::postorder(IR::P4Control* control) {
    if (tables.empty())
        return control;
    auto nm = new IR::NameMap<IR::Declaration, ordered_map>(control->stateful);
    for (auto t : tables)
        nm->addUnique(t->name, t);
    auto result = new IR::P4Control(control->srcInfo, control->name,
                                    control->type, control->constructorParams,
                                    std::move(*nm), control->body);
    return result;
}

/////////////////////////////////////////////////////////////////////

bool SynthesizeActions::mustMove(const IR::MethodCallStatement* statement) {
    auto mi = MethodInstance::resolve(statement, refMap, typeMap);
    if (mi->is<ActionCall>() || mi->is<ApplyMethod>())
        return false;
    if (!moveEmits) {
        if (!mi->is<ExternMethod>())
            return true;
        auto em = mi->to<ExternMethod>();
        auto corelib = P4::P4CoreLibrary::instance;
        if (em->type->name.name == corelib.packetOut.name &&
            em->method->name.name == corelib.packetOut.emit.name)
            return false;
    }
    return true;
}

const IR::Node* SynthesizeActions::postorder(IR::P4Control* control) {
    if (actions.empty())
        return control;
    auto nm = new IR::NameMap<IR::Declaration, ordered_map>(control->stateful);
    for (auto a : actions)
        nm->addUnique(a->name, a);
    auto result = new IR::P4Control(control->srcInfo, control->name,
                                    control->type, control->constructorParams,
                                    std::move(*nm), control->body);
    return result;
}

const IR::Node* SynthesizeActions::preorder(IR::BlockStatement* statement) {
    // Find a chain of statements to convert
    auto actbody = new IR::Vector<IR::StatOrDecl>();  // build here new action
    auto left = new IR::Vector<IR::StatOrDecl>();  // leftover statements

    for (auto c : *statement->components) {
        if (c->is<IR::AssignmentStatement>()) {
            actbody->push_back(c);
            continue;
        } else if (c->is<IR::MethodCallStatement>()) {
            if (mustMove(c->to<IR::MethodCallStatement>())) {
                actbody->push_back(c);
                continue;
            }
        } else {
            // This may modify 'changes'
            visit(c);
        }

        if (!actbody->empty()) {
            auto block = new IR::BlockStatement(Util::SourceInfo(), actbody);
            auto action = createAction(block);
            left->push_back(action);
            actbody = new IR::Vector<IR::StatOrDecl>();
        }
        left->push_back(c);
    }
    if (!actbody->empty())  {
        auto block = new IR::BlockStatement(Util::SourceInfo(), actbody);
        auto action = createAction(block);
        left->push_back(action);
    }
    prune();
    if (changes) {
        // Since we have only one 'changes' per P4Control, this may
        // be conservatively creating a new block when it hasn't changed.
        // But the result should be correct.
        auto result = new IR::BlockStatement(Util::SourceInfo(), left);
        return result;
    }
    return statement;
}

const IR::Statement* SynthesizeActions::createAction(const IR::Statement* toAdd) {
    changes = true;
    auto name = refMap->newName("act");
    const IR::Vector<IR::StatOrDecl>* body;
    if (toAdd->is<IR::BlockStatement>()) {
        body = toAdd->to<IR::BlockStatement>()->components;
    } else {
        auto b = new IR::Vector<IR::StatOrDecl>();
        b->push_back(toAdd);
        body = b;
    }
    auto action = new IR::P4Action(Util::SourceInfo(), name,
                                   IR::Annotations::empty,
                                   new IR::ParameterList(), body);
    actions.push_back(action);
    auto actpath = new IR::PathExpression(name);
    auto repl = new IR::MethodCallExpression(Util::SourceInfo(), actpath,
                                             new IR::Vector<IR::Type>(),
                                             new IR::Vector<IR::Expression>());
    auto result = new IR::MethodCallStatement(repl->srcInfo, repl);
    return result;
}

const IR::Node* SynthesizeActions::preorder(IR::AssignmentStatement* statement) {
    return createAction(statement);
}

const IR::Node* SynthesizeActions::preorder(IR::MethodCallStatement* statement) {
    if (mustMove(statement))
        return createAction(statement);
    return statement;
}


}  // namespace P4
