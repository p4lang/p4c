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

namespace P4 {

const IR::Node* DoMoveActionsToTables::postorder(IR::MethodCallStatement* statement) {
    auto mi = MethodInstance::resolve(statement, refMap, typeMap);
    if (!mi->is<ActionCall>())
        return statement;
    auto ac = mi->to<ActionCall>();

    auto action = ac->action;
    auto directionArgs = new IR::Vector<IR::Argument>();
    auto mc = statement->methodCall;

    // TODO: should use argument names
    auto it = action->parameters->parameters.begin();
    auto arg = mc->arguments->begin();
    for (; it != action->parameters->parameters.end(); ++it) {
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
    auto actinst = new IR::ActionListElement(statement->srcInfo, call);
    // Action list property
    auto actlist = new IR::ActionList({actinst});
    auto prop = new IR::Property(
        IR::ID(IR::TableProperties::actionsPropertyName, nullptr),
        actlist, false);
    // default action property
    auto otherArgs = new IR::Vector<IR::Argument>();
    for (; it != action->parameters->parameters.end(); ++it) {
        otherArgs->push_back(*arg);
        ++arg;
    }
    BUG_CHECK(arg == mc->arguments->end(), "%1%: mismatched arguments", mc);
    auto amce = new IR::MethodCallExpression(mc->srcInfo, mc->method, mc->typeArguments, otherArgs);
    auto defactval = new IR::ExpressionValue(amce);
    auto defprop = new IR::Property(
        IR::ID(IR::TableProperties::defaultActionPropertyName, nullptr),
        defactval, true);

    // List of table properties
    auto props = new IR::TableProperties({ prop, defprop });
    // Synthesize a new table
    cstring tblName = IR::ID(refMap->newName(cstring("tbl_") + ac->action->name.name), nullptr);

    auto annos = new IR::Annotations();
    annos->add(new IR::Annotation(IR::Annotation::hiddenAnnotation, {}));
    auto tbl = new IR::P4Table(tblName, annos, props);
    tables.push_back(tbl);

    // Table invocation statement
    auto tblpath = new IR::PathExpression(tblName);
    auto method = new IR::Member(tblpath, IR::IApply::applyMethodName);
    auto mce = new IR::MethodCallExpression(
        statement->srcInfo, method, new IR::Vector<IR::Type>(),
        new IR::Vector<IR::Argument>());
    auto stat = new IR::MethodCallStatement(mce->srcInfo, mce);
    return stat;
}

const IR::Node* DoMoveActionsToTables::postorder(IR::P4Control* control) {
    for (auto t : tables)
        control->controlLocals.push_back(t);
    return control;
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
    }
    return true;
}

const IR::Node* DoSynthesizeActions::preorder(IR::P4Control* control) {
    actions.clear();
    changes = false;
    if (policy != nullptr && !policy->convert(getContext(), control))
        prune();  // skip this one
    return control;
}

const IR::Node* DoSynthesizeActions::postorder(IR::P4Control* control) {
    for (auto a : actions)
        control->controlLocals.push_back(a);
    return control;
}

const IR::Node* DoSynthesizeActions::preorder(IR::BlockStatement* statement) {
    // Find a chain of statements to convert
    auto actbody = new IR::BlockStatement;  // build here new action
    auto left = new IR::BlockStatement(statement->annotations);  // leftover statements

    for (auto c : statement->components) {
        if (c->is<IR::AssignmentStatement>()) {
            if (mustMove(c->to<IR::AssignmentStatement>())) {
                if (policy && !actbody->components.empty() &&
                    !policy->can_combine(getContext(), actbody, c)) {
                    auto action = createAction(actbody);
                    left->push_back(action);
                    actbody = new IR::BlockStatement; }
                actbody->push_back(c);
                continue; }
        } else if (c->is<IR::MethodCallStatement>()) {
            if (mustMove(c->to<IR::MethodCallStatement>())) {
                if (policy && !actbody->components.empty() &&
                    !policy->can_combine(getContext(), actbody, c)) {
                    auto action = createAction(actbody);
                    left->push_back(action);
                    actbody = new IR::BlockStatement; }
                actbody->push_back(c);
                continue;
            }
        } else {
            // This may modify 'changes'
            visit(c);
        }

        if (!actbody->components.empty()) {
            auto action = createAction(actbody);
            left->push_back(action);
            actbody = new IR::BlockStatement;
        }
        left->push_back(c);
    }
    if (!actbody->components.empty()) {
        auto action = createAction(actbody);
        left->push_back(action);
    }
    prune();
    if (changes) {
        // Since we have only one 'changes' per P4Control, this may
        // be conservatively creating a new block when it hasn't changed.
        // But the result should be correct.
        return left;
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
        body = new IR::BlockStatement(toAdd->srcInfo, { toAdd });
    }

    auto annos = new IR::Annotations();
    annos->add(new IR::Annotation(IR::Annotation::hiddenAnnotation, {}));
    auto action = new IR::P4Action(name, annos, new IR::ParameterList(), body);
    actions.push_back(action);
    auto actpath = new IR::PathExpression(name);
    auto repl = new IR::MethodCallExpression(actpath);
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
