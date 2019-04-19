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

#include "localizeActions.h"
#include "frontends/p4/cloner.h"

namespace P4 {

namespace {

class ParamCloner : public ClonePathExpressions {
 public:
    ParamCloner() { setName("ParamCloner"); }
    const IR::Node* postorder(IR::Parameter* param) override {
        return new IR::Parameter(param->srcInfo, param->name, param->annotations,
                                 param->direction, param->type, param->defaultValue);
    }
    const IR::Node* postorder(IR::Declaration_Variable* decl) override {
        return new IR::Declaration_Variable(decl->srcInfo, decl->name,
                                            decl->annotations, decl->type, decl->initializer);
    }
};

}  // namespace

const IR::Node* TagGlobalActions::preorder(IR::P4Action* action) {
    if (findContext<IR::P4Control>() == nullptr) {
        auto annos = action->annotations;
        if (annos == nullptr)
            annos = IR::Annotations::empty;
        cstring name = cstring(".") + action->name;
        annos = annos->addAnnotationIfNew(IR::Annotation::nameAnnotation,
                                          new IR::StringLiteral(name));
        action->annotations = annos;
    }
    prune();
    return action;
}

bool FindGlobalActionUses::preorder(const IR::P4Action* action) {
    if (findContext<IR::P4Control>() == nullptr)
        globalActions.emplace(action);
    return false;
}

bool FindGlobalActionUses::preorder(const IR::PathExpression* path) {
    auto decl = refMap->getDeclaration(path->path, true);
    if (!decl->is<IR::P4Action>())
        return false;

    auto action = decl->to<IR::P4Action>();
    if (globalActions.find(action) == globalActions.end())
        return false;
    auto control = findContext<IR::P4Control>();
    if (control != nullptr) {
        if (repl->getReplacement(action, control) != nullptr)
            return false;
        auto newName = refMap->newName(action->name);
        ParamCloner cloner;
        auto replBody = cloner.clone<IR::BlockStatement>(action->body);
        auto params = cloner.clone<IR::ParameterList>(action->parameters);

        auto annos = action->annotations;
        if (annos == nullptr)
            annos = IR::Annotations::empty;
        annos->addAnnotationIfNew(IR::Annotation::nameAnnotation,
                                  new IR::StringLiteral(action->name));
        auto replacement = new IR::P4Action(action->srcInfo,
                    IR::ID(action->name.srcInfo, newName, action->name.originalName),
                    annos, params, replBody);
        repl->addReplacement(action, control, replacement);
    }
    return false;
}

const IR::Node* LocalizeActions::postorder(IR::P4Control* control) {
    auto actions = ::get(repl->repl, getOriginal<IR::P4Control>());
    if (actions == nullptr)
        return control;
    IR::IndexedVector<IR::Declaration> newDecls;
    for (auto pair : *actions) {
        auto toInsert = pair.second;
        LOG1("Adding " << dbp(toInsert));
        newDecls.push_back(toInsert);
    }

    newDecls.append(control->controlLocals);
    control->controlLocals = newDecls;
    return control;
}

const IR::Node* LocalizeActions::postorder(IR::PathExpression* expression) {
    auto control = findOrigCtxt<IR::P4Control>();
    if (control == nullptr)
        return expression;
    auto decl = refMap->getDeclaration(expression->path);
    if (!decl || !decl->is<IR::P4Action>())
        return expression;
    auto action = decl->to<IR::P4Action>();
    auto replacement = repl->getReplacement(action, control);
    if (replacement != nullptr) {
        LOG1("Rewriting " << dbp(expression) << " to " << dbp(replacement));
        expression = new IR::PathExpression(IR::ID(expression->srcInfo, replacement->name,
                                                   expression->path->name.originalName));
    }
    return expression;
}

bool FindRepeatedActionUses::preorder(const IR::PathExpression* expression) {
    auto decl = refMap->getDeclaration(expression->path, true);
    if (!decl->is<IR::P4Action>())
        return false;
    auto action = decl->to<IR::P4Action>();
    auto control = findContext<IR::P4Control>();
    if (control == nullptr)
        // not within a control; ignore.
        return false;

    const IR::Node* actionUser = findContext<IR::P4Table>();
    if (actionUser == nullptr)
        actionUser = findContext<IR::MethodCallExpression>();
    if (actionUser == nullptr)
        actionUser = findContext<IR::SwitchStatement>();
    BUG_CHECK(actionUser != nullptr,
              "%1%: action not within a table, method call or switch statement", expression);
    if (actionUser->is<IR::SwitchStatement>()) {
        auto swstat = actionUser->to<IR::SwitchStatement>();
        // We must figure out which table is being invoked; that is the user
        auto mem = swstat->expression->to<IR::Member>();
        CHECK_NULL(mem);
        BUG_CHECK(mem->member.name == IR::Type_Table::action_run,
                  "%1%: unexpected expression", mem);
        auto mce = mem->expr->to<IR::MethodCallExpression>();
        CHECK_NULL(mce);
        auto method = mce->method;
        BUG_CHECK(method->is<IR::Member>(), "Unexpected method %1%", method);
        auto methmem = method->to<IR::Member>();
        BUG_CHECK(methmem->expr->is<IR::PathExpression>(), "Unexpected table %1%", methmem->expr);
        auto pathe = methmem->expr->to<IR::PathExpression>();
        auto tbl = refMap->getDeclaration(pathe->path, true);
        BUG_CHECK(tbl->is<IR::P4Table>(), "%1%: expected a table", pathe);
        actionUser = tbl->to<IR::P4Table>();
    }

    LOG1(dbp(expression) << " used by " << dbp(actionUser));
    auto replacement = repl->getActionUser(action, actionUser);
    if (replacement == nullptr) {
        auto newName = refMap->newName(action->name);
        ParamCloner cloner;
        auto replBody = cloner.clone<IR::BlockStatement>(action->body);
        auto annos = action->annotations;
        auto params = cloner.clone<IR::ParameterList>(action->parameters);

        if (annos == nullptr)
            annos = IR::Annotations::empty;
        annos->addAnnotationIfNew(IR::Annotation::nameAnnotation,
                                  new IR::StringLiteral(action->name));
        replacement = new IR::P4Action(action->srcInfo,
                IR::ID(action->name.srcInfo, newName, action->name.originalName),
                annos, params, replBody);
        repl->createReplacement(action, actionUser, replacement);
    }
    repl->setRefReplacement(expression, replacement);
    return false;
}

const IR::Node* DuplicateActions::postorder(IR::P4Control* control) {
    bool changes = false;
    IR::IndexedVector<IR::Declaration> newDecls;
    for (auto d : control->controlLocals) {
        newDecls.push_back(d);
        if (d->is<IR::P4Action>()) {
            // The replacement are inserted in the same place
            // as the original.
            auto replacements = repl->toInsert[d->to<IR::P4Action>()];
            if (replacements != nullptr) {
                for (auto action : Values(*replacements)) {
                    LOG1("Adding " << dbp(action));
                    newDecls.push_back(action);
                    changes = true;
                }
            }
        }
    }

    if (changes)
        control->controlLocals = newDecls;
    return control;
}

const IR::Node* DuplicateActions::postorder(IR::PathExpression* expression) {
    auto replacement = ::get(repl->repl, getOriginal<IR::PathExpression>());
    if (replacement != nullptr) {
        LOG1("Rewriting " << dbp(expression) << " to " << dbp(replacement));
        expression = new IR::PathExpression(IR::ID(expression->srcInfo, replacement->name,
                                                   expression->path->name.originalName));
    }
    return expression;
}

}  // namespace P4
