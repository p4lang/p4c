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
#include "ir/ir.h"
#include "frontends/p4/coreLibrary.h"

namespace P4 {
void CreateBuiltins::postorder(IR::P4Parser* parser) {
    parser->states.push_back(new IR::ParserState(IR::ParserState::accept, nullptr));
    parser->states.push_back(new IR::ParserState(IR::ParserState::reject, nullptr));
}

void CreateBuiltins::postorder(IR::ActionListElement* element) {
    // convert path expressions to method calls
    // actions = { a; b; }
    // becomes
    // action = { a(); b(); }
    if (element->expression->is<IR::PathExpression>())
        element->expression = new IR::MethodCallExpression(
            element->expression->srcInfo, element->expression,
            new IR::Vector<IR::Type>(), new IR::Vector<IR::Argument>());
}

void CreateBuiltins::postorder(IR::ExpressionValue* expression) {
    // convert a default_action = a; into
    // default_action = a();
    auto prop = findContext<IR::Property>();
    if (prop != nullptr &&
        prop->name == IR::TableProperties::defaultActionPropertyName &&
        expression->expression->is<IR::PathExpression>())
        expression->expression = new IR::MethodCallExpression(
            expression->expression->srcInfo, expression->expression,
            new IR::Vector<IR::Type>(), new IR::Vector<IR::Argument>());
}

void CreateBuiltins::postorder(IR::ParserState* state) {
    if (state->selectExpression == nullptr) {
        warning(ErrorType::WARN_PARSER_TRANSITION, "%1%: implicit transition to `reject'", state);
        state->selectExpression = new IR::PathExpression(IR::ParserState::reject);
    }
}

void CreateBuiltins::postorder(IR::ActionList* actions) {
    if (!addNoAction)
        return;
    auto decl = actions->getDeclaration(P4::P4CoreLibrary::instance.noAction.str());
    if (decl != nullptr)
        return;
    actions->push_back(
        new IR::ActionListElement(
            new IR::Annotations({new IR::Annotation(IR::Annotation::defaultOnlyAnnotation, {})}),
            new IR::MethodCallExpression(
                new IR::PathExpression(P4::P4CoreLibrary::instance.noAction.Id(actions->srcInfo)),
                new IR::Vector<IR::Type>(), new IR::Vector<IR::Argument>())));
}

bool CreateBuiltins::preorder(IR::P4Table* table) {
    addNoAction = false;
    if (table->getDefaultAction() == nullptr)
        addNoAction = true;
    return true;
}

void CreateBuiltins::postorder(IR::TableProperties* properties) {
    if (!addNoAction)
        return;
    auto act = new IR::PathExpression(P4::P4CoreLibrary::instance.noAction.Id(properties->srcInfo));
    auto args = new IR::Vector<IR::Argument>();
    auto methodCall = new IR::MethodCallExpression(act, args);
    auto prop = new IR::Property(
        IR::ID(IR::TableProperties::defaultActionPropertyName),
        new IR::ExpressionValue(methodCall),
        /* isConstant = */ false);
    properties->properties.push_back(prop);
}

}  // namespace P4
