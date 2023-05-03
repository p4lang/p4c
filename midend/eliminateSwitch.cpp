/*
Copyright 2020 VMware, Inc.

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

#include "eliminateSwitch.h"

#include "frontends/p4/coreLibrary.h"

namespace P4 {

const IR::Node *DoEliminateSwitch::postorder(IR::P4Program *program) {
    if (!exactNeeded) return program;
    for (auto *obj : program->objects) {
        if (auto *match_kind = obj->to<IR::Declaration_MatchKind>()) {
            if (match_kind->getDeclByName(P4CoreLibrary::instance().exactMatch.Id()))
                return program;
        }
    }
    ::error(ErrorType::ERR_NOT_FOUND,
            "Could not find declaration for 'match_kind.exact', which is needed to implement "
            "switch statements; did you include core.p4?");
    return program;
}

const IR::Node *DoEliminateSwitch::postorder(IR::P4Control *control) {
    for (auto a : toInsert) control->controlLocals.push_back(a);
    toInsert.clear();
    return control;
}

const IR::Node *DoEliminateSwitch::postorder(IR::SwitchStatement *statement) {
    if (findContext<IR::P4Action>()) {
        ::error("%1%: switch statements not supported in actions on this target", statement);
        return statement;
    }
    auto type = typeMap->getType(statement->expression);
    if (type->is<IR::Type_ActionEnum>())
        // Classic switch; no changes needed
        return statement;

    auto src = statement->srcInfo;
    IR::IndexedVector<IR::StatOrDecl> contents;
    cstring prefix = refMap->newName("switch");
    // Compute key for new switch statement
    cstring key = refMap->newName(prefix + "_key");
    auto decl = new IR::Declaration_Variable(key, type->getP4Type());
    toInsert.push_back(decl);
    auto assign =
        new IR::AssignmentStatement(src, new IR::PathExpression(key), statement->expression);
    contents.push_back(assign);

    auto hidden = new IR::Annotations();
    hidden->add(new IR::Annotation(IR::Annotation::hiddenAnnotation, {}));

    auto tableKeyEl =
        new IR::KeyElement(src, new IR::PathExpression(key),
                           new IR::PathExpression(P4CoreLibrary::instance().exactMatch.Id()));
    exactNeeded = true;
    IR::IndexedVector<IR::ActionListElement> actionsList;
    IR::IndexedVector<IR::Property> properties;
    IR::Vector<IR::SwitchCase> cases;
    IR::Vector<IR::Entry> entries;
    properties.push_back(new IR::Property(src, "key", new IR::Key({tableKeyEl}), false));
    IR::ID defaultAction = P4CoreLibrary::instance().noAction.Id();

    // Add Default switch Expression if not present
    bool isDefaultPresent = false;
    for (auto sc : statement->cases) {
        if (sc->label->is<IR::DefaultExpression>()) {
            isDefaultPresent = true;
            break;
        }
    }
    if (!isDefaultPresent) {
        statement->cases.push_back(new IR::SwitchCase(
            new IR::DefaultExpression(IR::Type_Dontcare::get()), new IR::BlockStatement));
    }
    // Create actions
    IR::Vector<IR::Expression> pendingLabels;  // switch labels with no statement
    for (auto sc : statement->cases) {
        auto scSrc = sc->srcInfo;
        pendingLabels.push_back(sc->label);
        if (sc->statement != nullptr) {
            cstring actionName = refMap->newName(prefix + "_case");
            auto action = new IR::P4Action(src, actionName, hidden, new IR::ParameterList(),
                                           new IR::BlockStatement());
            auto actionCall =
                new IR::MethodCallExpression(scSrc, new IR::PathExpression(actionName));
            toInsert.push_back(action);
            auto ale = new IR::ActionListElement(scSrc, actionCall);
            actionsList.push_back(ale);

            auto c = new IR::SwitchCase(scSrc, new IR::PathExpression(actionName), sc->statement);
            cases.push_back(c);
            if (sc->label->is<IR::DefaultExpression>()) {
                defaultAction = IR::ID(actionName);
            }

            for (auto lab : pendingLabels) {
                if (!lab->is<IR::DefaultExpression>()) {
                    auto entry =
                        new IR::Entry(scSrc, new IR::ListExpression({lab}), actionCall, false);
                    entries.push_back(entry);
                }
            }
            pendingLabels.clear();
        }
    }

    properties.push_back(new IR::Property(src, "actions", new IR::ActionList(actionsList), false));
    properties.push_back(new IR::Property(
        src, "default_action",
        new IR::ExpressionValue(
            src, new IR::MethodCallExpression(new IR::PathExpression(defaultAction))),
        true));
    properties.push_back(new IR::Property(src, "entries", new IR::EntriesList(entries), true));
    cstring tableName = refMap->newName(prefix + "_table");
    auto table = new IR::P4Table(src, tableName, hidden, new IR::TableProperties(properties));
    toInsert.push_back(table);

    auto apply = new IR::Member(
        src,
        new IR::MethodCallExpression(src, new IR::Member(src, new IR::PathExpression(tableName),
                                                         IR::IApply::applyMethodName)),
        IR::Type_Table::action_run);
    auto stat = new IR::SwitchStatement(src, apply, cases);
    contents.push_back(stat);
    return new IR::BlockStatement(src, contents);
}

}  // namespace P4
