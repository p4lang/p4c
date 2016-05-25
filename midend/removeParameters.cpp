/*
Copyright 2016 VMware, Inc.

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

#include "removeParameters.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/tableApply.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "midend/moveDeclarations.h"

namespace P4 {

namespace {
// Remove all arguments from any embedded MethodCallExpression
class RemoveMethodCallArguments : public Transform {
 public:
    RemoveMethodCallArguments() { setName("RemoveMethodCallArguments"); }
    const IR::Node* postorder(IR::MethodCallExpression* expression) override {
        expression->arguments = new IR::Vector<IR::Expression>();
        return expression;
    }
};
}  // namespace

HasTableApply::HasTableApply(const ReferenceMap* refMap, const TypeMap* typeMap) :
        refMap(refMap), typeMap(typeMap), table(nullptr), call(nullptr)
{ CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("HasTableApply"); }

void HasTableApply::postorder(const IR::MethodCallExpression* expression) {
    auto mi = MethodInstance::resolve(expression, refMap, typeMap);
    if (!mi->isApply()) return;
    auto am = mi->to<P4::ApplyMethod>();
    if (!am->object->is<IR::P4Table>()) return;
    BUG_CHECK(table == nullptr, "%1% and %2%: multiple table applications in one expression",
              table, am->object);
    table = am->object->to<IR::P4Table>();
    call = expression;
    LOG1("Invoked table is " << table);
}

const IR::Node* RemoveTableParameters::postorder(IR::P4Table* table) {
    if (table->parameters->size() == 0)
        return table;
    BUG_CHECK(getContext()->node->is<IR::IndexedVector<IR::Declaration>>(),
              "%1%: unexpected parent %2%", getOriginal(), getContext()->node);
    auto result = new IR::IndexedVector<IR::Declaration>();
    for (auto p : *table->parameters->parameters) {
        auto decl = new IR::Declaration_Variable(p->srcInfo, p->name,
                                                 p->annotations, p->type, nullptr);
        result->push_back(decl);
    }
    original.emplace(getOriginal<IR::P4Table>());
    table->parameters = new IR::ParameterList();
    LOG1("To replace " << table);
    result->push_back(table);
    return result;
}

void RemoveTableParameters::doParameters(
    const IR::ParameterList* params, const IR::Vector<IR::Expression>* args,
    IR::IndexedVector<IR::StatOrDecl>* result, bool in) {

    if (in) {
        auto it = args->begin();
        for (auto param : *params->parameters) {
            auto initializer = *it;
            if (param->direction == IR::Direction::In ||
                param->direction == IR::Direction::InOut) {
                auto path = new IR::PathExpression(param->name);
                auto stat = new IR::AssignmentStatement(Util::SourceInfo(), path, initializer);
                result->push_back(stat);
            }
            ++it;
        }
    } else {
        auto it = args->begin();
        for (auto param : *params->parameters) {
            auto left = *it;
            if (param->direction == IR::Direction::InOut ||
                param->direction == IR::Direction::Out) {
                auto right = new IR::PathExpression(param->name);
                auto copyout = new IR::AssignmentStatement(Util::SourceInfo(), left, right);
                result->push_back(copyout);
            }
            ++it;
        }
    }
}

const IR::Node* RemoveTableParameters::postorder(IR::MethodCallStatement* statement) {
    LOG1("Visiting " << getOriginal());
    HasTableApply hta(refMap, typeMap);
    statement->apply(hta);
    if (hta.table == nullptr)
        return statement;
    // The reference map still points to the original table,
    // which is how hta.table is obtained
    if (original.find(hta.table) == original.end())
        return statement;
    auto table = hta.table;

    LOG1("Found table invocation " << table);
    auto call = hta.call;
    CHECK_NULL(call);
    auto params = table->parameters;
    CHECK_NULL(params);

    LOG1("Replacing " << getOriginal());
    auto body = new IR::IndexedVector<IR::StatOrDecl>();
    // Evaluate in and inout parameters in order.
    auto args = call->arguments;
    doParameters(params, args, body, true);

    RemoveMethodCallArguments rmc;
    auto newStat = statement->apply(rmc)->to<IR::Statement>();
    body->push_back(newStat);

    // Copy back out and inout parameters
    doParameters(params, args, body, false);

    auto result = new IR::BlockStatement(statement->srcInfo, body);
    return result;
}

const IR::Node* RemoveTableParameters::postorder(IR::IfStatement* statement) {
    HasTableApply hta(refMap, typeMap);
    statement->condition->apply(hta);
    if (hta.table == nullptr)
        return statement;
    // The reference map still points to the original table,
    // which is how hta.table is obtained
    if (original.find(hta.table) == original.end())
        return statement;
    auto table = hta.table;

    LOG1("Found table invocation " << table);
    auto call = hta.call;
    CHECK_NULL(call);
    auto params = table->parameters;
    CHECK_NULL(params);

    LOG1("Replacing " << getOriginal());
    auto pre = new IR::IndexedVector<IR::StatOrDecl>();
    // Evaluate in and inout parameters in order.
    auto args = call->arguments;
    doParameters(params, args, pre, true);

    // we only expect such expressions
    auto tbl = TableApplySolver::isHit(statement->condition, refMap, typeMap);
    CHECK_NULL(tbl);

    RemoveMethodCallArguments rma;
    auto newCond = statement->condition->apply(rma)->to<IR::Expression>();
    statement->condition = newCond;
    pre->push_back(statement);

    // Copy back out and inout parameters in both if branches
    auto tpost = new IR::IndexedVector<IR::StatOrDecl>();
    doParameters(params, args, tpost, false);
    if (tpost->size() != 0) {
        auto trueBlock = new IR::BlockStatement(statement->ifTrue->srcInfo, tpost);
        tpost->push_back(statement->ifTrue);
        statement->ifTrue = trueBlock;

        // Create false branch if it does not exist
        auto fpost = new IR::IndexedVector<IR::StatOrDecl>();
        doParameters(params, args, fpost, false);
        auto falseBlock = new IR::BlockStatement(Util::SourceInfo(), fpost);
        if (statement->ifFalse)
            fpost->push_back(statement->ifFalse);
        statement->ifFalse = falseBlock;
    }

    auto result = new IR::BlockStatement(statement->srcInfo, pre);
    return result;
}

const IR::Node* RemoveTableParameters::postorder(IR::SwitchStatement* statement) {
    HasTableApply hta(refMap, typeMap);
    statement->expression->apply(hta);
    if (hta.table == nullptr)
        return statement;
    // The reference map still points to the original table,
    // which is how hta.table is obtained
    if (original.find(hta.table) == original.end())
        return statement;
    auto table = hta.table;

    LOG1("Found table invocation " << table);
    auto call = hta.call;
    CHECK_NULL(call);
    auto params = table->parameters;
    CHECK_NULL(params);

    LOG1("Replacing " << getOriginal());
    auto pre = new IR::IndexedVector<IR::StatOrDecl>();
    // Evaluate in and inout parameters in order.
    auto args = call->arguments;
    doParameters(params, args, pre, true);

    // we only expect such expressions
    auto tbl = TableApplySolver::isActionRun(statement->expression, refMap, typeMap);
    CHECK_NULL(tbl);

    RemoveMethodCallArguments rma;
    auto newCond = statement->expression->apply(rma)->to<IR::Expression>();
    statement->expression = newCond;
    pre->push_back(statement);

    bool anyOut = false;
    for (auto p : *params->parameters) {
        if (p->direction == IR::Direction::Out ||
            p->direction == IR::Direction::InOut) {
            anyOut = true;
            break;
        }
    }

    if (anyOut) {
        auto cases = new IR::Vector<IR::SwitchCase>();
        bool foundDefault = false;
        for (auto l : *statement->cases) {
            if (l->label->is<IR::DefaultExpression>())
                foundDefault = true;
            if (l->statement != nullptr) {
                auto post = new IR::IndexedVector<IR::StatOrDecl>();
                doParameters(params, args, post, false);
                post->push_back(l->statement);
                auto block = new IR::BlockStatement(l->srcInfo, post);
                auto swcase = new IR::SwitchCase(l->srcInfo, l->label, block);
                cases->push_back(swcase);
            }
        }
        if (!foundDefault) {
            // Must add a default label
            auto post = new IR::IndexedVector<IR::StatOrDecl>();
            doParameters(params, args, post, false);
            auto block = new IR::BlockStatement(Util::SourceInfo(), post);
            auto swcase = new IR::SwitchCase(Util::SourceInfo(),
                                             new IR::DefaultExpression(Util::SourceInfo()), block);
            cases->push_back(swcase);
        }

        statement->cases = cases;
    }
    auto result = new IR::BlockStatement(statement->srcInfo, pre);
    return result;
}

///////////////////////////////////////////////////////////////////////

void FindActionParameters::postorder(const IR::ActionListElement* element) {
    auto path = element->getPath();
    auto decl = refMap->getDeclaration(path, true);
    BUG_CHECK(decl->is<IR::P4Action>(), "%1%: not an action", element);
    BUG_CHECK(element->expression->is<IR::MethodCallExpression>(),
              "%1%: expected a method call", element->expression);
    invocations->bind(decl->to<IR::P4Action>(),
                      element->expression->to<IR::MethodCallExpression>(), false);
}

void FindActionParameters::postorder(const IR::MethodCallExpression* expression) {
    auto table = findContext<IR::P4Table>();
    if (table != nullptr)
        // Here we only care about direct action invocations
        return;
    auto mi = MethodInstance::resolve(expression, refMap, typeMap);
    if (mi->is<P4::ActionCall>()) {
        auto ac = mi->to<P4::ActionCall>();
        invocations->bind(ac->action, expression, true);
    }
}

const IR::Node* RemoveActionParameters::postorder(IR::P4Action* action) {
    LOG1("Visiting " << action);
    BUG_CHECK(getContext()->node->is<IR::IndexedVector<IR::Declaration>>(),
              "%1%: unexpected parent %2%", getOriginal(), getContext()->node);
    auto result = new IR::IndexedVector<IR::Declaration>();
    auto leftParams = new IR::IndexedVector<IR::Parameter>();
    auto initializers = new IR::IndexedVector<IR::StatOrDecl>();
    auto postamble = new IR::IndexedVector<IR::StatOrDecl>();
    auto invocation = invocations->get(getOriginal<IR::P4Action>());
    if (invocation == nullptr)
        return action;
    auto args = invocation->arguments;

    auto argit = args->begin();
    bool removeAll = invocations->removeAllParameters(getOriginal<IR::P4Action>());
    for (auto p : *action->parameters->parameters) {
        if (p->direction == IR::Direction::None && !removeAll) {
            leftParams->push_back(p);
        } else {
            auto decl = new IR::Declaration_Variable(p->srcInfo, p->name,
                                                     p->annotations, p->type, nullptr);
            result->push_back(decl);
            auto arg = *argit;
            ++argit;
            if (p->direction == IR::Direction::In ||
                p->direction == IR::Direction::InOut ||
                p->direction == IR::Direction::None) {
                auto left = new IR::PathExpression(p->name);
                auto assign = new IR::AssignmentStatement(arg->srcInfo, left, arg);
                initializers->push_back(assign);
            }
            if (p->direction == IR::Direction::Out ||
                p->direction == IR::Direction::InOut) {
                auto right = new IR::PathExpression(p->name);
                auto assign = new IR::AssignmentStatement(arg->srcInfo, arg, right);
                postamble->push_back(assign);
            }
        }
    }
    if (result->empty())
        return action;

    initializers->append(*action->body->components);
    initializers->append(*postamble);

    action->parameters = new IR::ParameterList(action->parameters->srcInfo, leftParams);
    action->body = new IR::BlockStatement(action->body->srcInfo, initializers);
    LOG1("To replace " << action);
    result->push_back(action);
    return result;
}

const IR::Node* RemoveActionParameters::postorder(IR::ActionListElement* element) {
    RemoveMethodCallArguments rmca;
    element->expression = element->expression->apply(rmca)->to<IR::Expression>();
    return element;
}

const IR::Node* RemoveActionParameters::postorder(IR::MethodCallExpression* expression) {
    if (invocations->isCall(getOriginal<IR::MethodCallExpression>())) {
        RemoveMethodCallArguments rmca;
        return expression->apply(rmca);
    }
    return expression;
}

//////////////////////////////////////////

RemoveParameters::RemoveParameters(ReferenceMap* refMap, TypeMap* typeMap, bool isv1) {
    setName("RemoveParameters");
    auto ai = new ActionInvocation();
    passes.emplace_back(new TypeChecking(refMap, typeMap, isv1));
    passes.emplace_back(new RemoveTableParameters(refMap, typeMap));
    // This is needed because of this case:
    // action a(inout x) { x = x + 1 }
    // bit<32> w;
    // table t() { actions = a(w); ... }
    // Without the MoveDeclarations the code would become
    // action a() { x = w; x = x + 1; w = x; } << w is not yet defined
    // bit<32> w;
    // table t() { actions = a(); ... }
    passes.emplace_back(new MoveDeclarations());
    passes.emplace_back(new TypeChecking(refMap, typeMap, isv1));
    passes.emplace_back(new FindActionParameters(refMap, typeMap, ai));
    passes.emplace_back(new RemoveActionParameters(ai));
}

}  // namespace P4
