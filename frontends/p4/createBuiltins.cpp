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

#include "createBuiltins.h"

#include <string>
#include <vector>

#include "frontends/common/model.h"
#include "frontends/p4/coreLibrary.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/enumerator.h"
#include "lib/error.h"
#include "lib/error_catalog.h"

namespace P4 {

const IR::Node *CreateBuiltins::preorder(IR::P4Program *program) {
    auto decls = program->getDeclsByName(P4::P4CoreLibrary::instance().noAction.str());
    auto vec = decls->toVector();
    if (vec->empty()) return program;
    if (vec->size() > 1) {
        ::error(ErrorType::ERR_MODEL, "Multiple declarations of %1%: %2% %3%",
                P4::P4CoreLibrary::instance().noAction.str(), vec->at(0), vec->at(1));
    }
    globalNoAction = vec->at(0);
    return program;
}

void CreateBuiltins::checkGlobalAction() {
    // Check some invariants expected from NoAction.
    // The check is done lazily, only if NoAction invocations are
    // inserted by the compiler.  This allows for example simple test
    // programs that do not really need NoAction to skip including
    // core.p4.
    static bool checked = false;
    if (checked) return;
    checked = true;
    if (globalNoAction == nullptr) {
        ::error(ErrorType::ERR_MODEL,
                "Declaration of the action %1% not found; did you include core.p4?",
                P4::P4CoreLibrary::instance().noAction.str());
        return;
    }
    if (auto action = globalNoAction->to<IR::P4Action>()) {
        if (!action->parameters->empty()) {
            ::error(ErrorType::ERR_MODEL,
                    "%1%: Expected an action with no parameters; did you include core.p4?",
                    globalNoAction);
            return;
        }
        if (!action->body->empty()) {
            ::error(ErrorType::ERR_MODEL,
                    "%1%: Expected an action with no body; did you include core.p4?",
                    globalNoAction);
            return;
        }
    } else {
        ::error(ErrorType::ERR_MODEL, "%1%: expected to be an action; did you include core.p4?",
                globalNoAction);
        return;
    }
}

const IR::Node *CreateBuiltins::postorder(IR::P4Parser *parser) {
    parser->states.push_back(new IR::ParserState(IR::ParserState::accept, nullptr));
    parser->states.push_back(new IR::ParserState(IR::ParserState::reject, nullptr));
    return parser;
}

const IR::Node *CreateBuiltins::postorder(IR::ActionListElement *element) {
    // convert path expressions to method calls
    // actions = { a; b; }
    // becomes
    // action = { a(); b(); }
    if (element->expression->is<IR::PathExpression>())
        element->expression = new IR::MethodCallExpression(
            element->expression->srcInfo, element->expression, new IR::Vector<IR::Type>(),
            new IR::Vector<IR::Argument>());
    return element;
}

const IR::Node *CreateBuiltins::postorder(IR::ExpressionValue *expression) {
    // convert a default_action = a; into
    // default_action = a();
    auto prop = findContext<IR::Property>();
    if (prop != nullptr && prop->name == IR::TableProperties::defaultActionPropertyName &&
        expression->expression->is<IR::PathExpression>())
        expression->expression = new IR::MethodCallExpression(
            expression->expression->srcInfo, expression->expression, new IR::Vector<IR::Type>(),
            new IR::Vector<IR::Argument>());
    return expression;
}

const IR::Node *CreateBuiltins::postorder(IR::Entry *entry) {
    // convert a const table entry with action "a;" into "a();"
    if (entry->action->is<IR::PathExpression>())
        entry->action = new IR::MethodCallExpression(entry->action->srcInfo, entry->action,
                                                     new IR::Vector<IR::Type>(),
                                                     new IR::Vector<IR::Argument>());
    return entry;
}

const IR::Node *CreateBuiltins::postorder(IR::ParserState *state) {
    if (state->selectExpression == nullptr) {
        warn(ErrorType::WARN_PARSER_TRANSITION, "%1%: implicit transition to `reject'", state);
        state->selectExpression = new IR::PathExpression(IR::ParserState::reject);
    }
    return state;
}

const IR::Node *CreateBuiltins::postorder(IR::ActionList *actions) {
    if (!addNoAction) return actions;
    auto decl = actions->getDeclaration(P4::P4CoreLibrary::instance().noAction.str());
    if (decl != nullptr) return actions;
    checkGlobalAction();
    actions->push_back(new IR::ActionListElement(
        new IR::Annotations({new IR::Annotation(IR::Annotation::defaultOnlyAnnotation, {})}),
        new IR::MethodCallExpression(
            new IR::PathExpression(P4::P4CoreLibrary::instance().noAction.Id(actions->srcInfo)),
            new IR::Vector<IR::Type>(), new IR::Vector<IR::Argument>())));
    return actions;
}

const IR::Node *CreateBuiltins::preorder(IR::P4Table *table) {
    addNoAction = false;
    if (table->getDefaultAction() == nullptr) addNoAction = true;
    return table;
}

const IR::Node *CreateBuiltins::postorder(IR::Property *property) {
    // Spec: a table with an empty key is the same
    // as a table with no key.
    if (property->name != IR::TableProperties::keyPropertyName) return property;
    if (auto key = property->value->to<IR::Key>()) {
        if (key->keyElements.size() == 0) return nullptr;
    } else {
        ::error(ErrorType::ERR_INVALID, "%1%: must be a key", property->value);
        return nullptr;
    }
    return property;
}

const IR::Node *CreateBuiltins::postorder(IR::TableProperties *properties) {
    if (!addNoAction) return properties;
    checkGlobalAction();
    auto act =
        new IR::PathExpression(P4::P4CoreLibrary::instance().noAction.Id(properties->srcInfo));
    auto args = new IR::Vector<IR::Argument>();
    auto methodCall = new IR::MethodCallExpression(act, args);
    auto prop = new IR::Property(IR::ID(IR::TableProperties::defaultActionPropertyName),
                                 new IR::ExpressionValue(methodCall),
                                 /* isConstant = */ false);
    properties->properties.push_back(prop);
    return properties;
}

}  // namespace P4
