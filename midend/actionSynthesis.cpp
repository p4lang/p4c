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

#include "actionSynthesis.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/fromv1.0/v1model.h"

namespace P4 {

const IR::Node* DoMoveActionsToTables::postorder(IR::MethodCallStatement* statement) {
    auto mi = MethodInstance::resolve(statement, refMap, typeMap);
    if (!mi->is<ActionCall>())
        return statement;
    auto ac = mi->to<ActionCall>();

    auto action = ac->action;
    auto directionArgs = new IR::Vector<IR::Expression>();
    auto mc = statement->methodCall;

    auto it = action->parameters->parameters->begin();
    auto arg = mc->arguments->begin();
    for (; it != action->parameters->parameters->end(); ++it) {
        auto p = *it;
        if (p->direction == IR::Direction::None)
            break;
        directionArgs->push_back(*arg);
        ++arg;
    }

    // Action invocation
    BUG_CHECK(ac->expr->method->is<IR::PathExpression>(),
              "%1%: Expected a PathExpression", ac->expr->method);
    auto actionPath = new IR::PathExpression(IR::ID(mc->srcInfo, ac->action->name));
    auto call = new IR::MethodCallExpression(mc->srcInfo, actionPath,
                                             new IR::Vector<IR::Type>(), directionArgs);
    auto actinst = new IR::ActionListElement(statement->srcInfo, IR::Annotations::empty, call);
    auto actions = new IR::IndexedVector<IR::ActionListElement>();
    actions->push_back(actinst);
    // Action list property
    auto actlist = new IR::ActionList(Util::SourceInfo(), actions);
    auto prop = new IR::Property(
        Util::SourceInfo(),
        IR::ID(IR::TableProperties::actionsPropertyName, nullptr),
        IR::Annotations::empty, actlist, false);
    // default action property
    auto otherArgs = new IR::Vector<IR::Expression>();
    for (; it != action->parameters->parameters->end(); ++it) {
        otherArgs->push_back(*arg);
        ++arg;
    }
    BUG_CHECK(arg == mc->arguments->end(), "%1%: mismatched arguments", mc);
    auto amce = new IR::MethodCallExpression(mc->srcInfo, mc->method, mc->typeArguments, otherArgs);
    auto defactval = new IR::ExpressionValue(Util::SourceInfo(), amce);
    auto defprop = new IR::Property(
        Util::SourceInfo(), IR::ID(IR::TableProperties::defaultActionPropertyName, nullptr),
        IR::Annotations::empty, defactval, true);

    // List of table properties
    auto nm = new IR::IndexedVector<IR::Property>();
    nm->push_back(prop);
    nm->push_back(defprop);
    auto props = new IR::TableProperties(Util::SourceInfo(), nm);
    // Synthesize a new table
    cstring tblName = IR::ID(refMap->newName(cstring("tbl_") + ac->action->name.name), nullptr);
    auto tbl = new IR::P4Table(Util::SourceInfo(), tblName, IR::Annotations::empty, props);
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

const IR::Node* DoMoveActionsToTables::postorder(IR::P4Control* control) {
    if (tables.empty())
        return control;
    auto nm = new IR::IndexedVector<IR::Declaration>(*control->controlLocals);
    for (auto t : tables)
        nm->push_back(t);
    auto result = new IR::P4Control(control->srcInfo, control->name,
                                    control->type, control->constructorParams,
                                    nm, control->body);
    return result;
}

/////////////////////////////////////////////////////////////////////

bool DoSynthesizeActions::mustMove(const IR::MethodCallStatement* statement) {
    auto mi = MethodInstance::resolve(statement, refMap, typeMap);
    if (mi->is<ActionCall>() || mi->is<ApplyMethod>())
        return false;
    return true;
}

bool DoSynthesizeActions::mustMove(const IR::AssignmentStatement *assign) {
    if (auto mc = assign->right->to<IR::MethodCallExpression>()) {
        auto mi = MethodInstance::resolve(mc, refMap, typeMap);
        if (!mi->is<ExternMethod>())
            return true;
        auto em = mi->to<ExternMethod>();
        auto &v1model = P4V1::V1Model::instance;
        if (em->originalExternType->name.name == v1model.ck16.name)
            return false;
    }
    return true;
}

const IR::Node* DoSynthesizeActions::preorder(IR::P4Control* control) {
    actions.clear();
    changes = false;
    if (policy != nullptr && !policy->convert(control))
        prune();  // skip this one
    return control;
}

const IR::Node* DoSynthesizeActions::postorder(IR::P4Control* control) {
    if (actions.empty())
        return control;
    auto nm = new IR::IndexedVector<IR::Declaration>(*control->controlLocals);
    for (auto a : actions)
        nm->push_back(a);
    auto result = new IR::P4Control(control->srcInfo, control->name,
                                    control->type, control->constructorParams,
                                    nm, control->body);
    return result;
}

const IR::Node* DoSynthesizeActions::preorder(IR::BlockStatement* statement) {
    // Find a chain of statements to convert
    auto actbody = new IR::IndexedVector<IR::StatOrDecl>();  // build here new action
    auto left = new IR::IndexedVector<IR::StatOrDecl>();  // leftover statements

    for (auto c : *statement->components) {
        if (c->is<IR::AssignmentStatement>()) {
            if (mustMove(c->to<IR::AssignmentStatement>())) {
                actbody->push_back(c);
                continue; }
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
            auto block = new IR::BlockStatement(
                Util::SourceInfo(), IR::Annotations::empty, actbody);
            auto action = createAction(block);
            left->push_back(action);
            actbody = new IR::IndexedVector<IR::StatOrDecl>();
        }
        left->push_back(c);
    }
    if (!actbody->empty()) {
        auto block = new IR::BlockStatement(
            Util::SourceInfo(), IR::Annotations::empty, actbody);
        auto action = createAction(block);
        left->push_back(action);
    }
    prune();
    if (changes) {
        // Since we have only one 'changes' per P4Control, this may
        // be conservatively creating a new block when it hasn't changed.
        // But the result should be correct.
        auto result = new IR::BlockStatement(Util::SourceInfo(), statement->annotations, left);
        return result;
    }
    return statement;
}

const IR::Statement* DoSynthesizeActions::createAction(const IR::Statement* toAdd) {
    changes = true;
    auto name = refMap->newName("act");
    const IR::BlockStatement* body;
    if (toAdd->is<IR::BlockStatement>()) {
        body = toAdd->to<IR::BlockStatement>();
    } else {
        auto b = new IR::IndexedVector<IR::StatOrDecl>();
        b->push_back(toAdd);
        body = new IR::BlockStatement(toAdd->srcInfo, IR::Annotations::empty, b);
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

const IR::Node* DoSynthesizeActions::preorder(IR::AssignmentStatement* statement) {
    if (mustMove(statement))
        return createAction(statement);
    return statement;
}

const IR::Node* DoSynthesizeActions::preorder(IR::MethodCallStatement* statement) {
    if (mustMove(statement))
        return createAction(statement);
    return statement;
}

}  // namespace P4
