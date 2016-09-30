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

#include "parserControlFlow.h"

namespace P4 {

/* convert
state s {
   statement1;
   statement2;
   if (exp)
      statement3;
   else
      statement4;
   statement5;
   transition selectExpression;
}

into 4 states:

state s {
   statement1;
   statement2;
   transition select(exp) {
      true: s_true;
      false: s_false;
   }
}

state s_true {
   statement3;
   transition s_join;
}

state s_false {
   statement4;
   transition s_join;
}

state s_join {
   statement5;
   transition selectExpression;
}

*/

const IR::Node* DoRemoveParserControlFlow::postorder(IR::ParserState* state) {
    LOG1("Visiting " << dbp(state));
    // TODO: we keep annotations on the first state,
    // but this may be wrong for something like @atomic

    // Set of newly created states
    auto states = new IR::IndexedVector<IR::ParserState>();
    IR::ParserState* currentState = state;
    // components of the currentState
    auto currentComponents = new IR::IndexedVector<IR::StatOrDecl>();
    auto origComponents = state->components;
    auto origSelect = state->selectExpression;

    cstring joinName;  // non-empty if we split the state
    for (auto c : *origComponents) {
        if (c->is<IR::IfStatement>()) {
            LOG1("Converting " << c << " into states");

            states->push_back(currentState);
            auto ifstat = c->to<IR::IfStatement>();
            cstring joinName = refMap->newName(state->name.name + "_join");

            // s_true
            cstring trueName = refMap->newName(state->name.name + "_true");
            auto trueComponents = new IR::IndexedVector<IR::StatOrDecl>();
            trueComponents->push_back(ifstat->ifTrue);
            auto trueState = new IR::ParserState(
                Util::SourceInfo(), trueName, IR::Annotations::empty, trueComponents,
                new IR::PathExpression(IR::ID(joinName)));
            states->push_back(trueState);

            // s_false
            cstring falseName = joinName;
            if (ifstat->ifFalse != nullptr) {
                falseName = refMap->newName(state->name.name + "_false");
                auto falseComponents = new IR::IndexedVector<IR::StatOrDecl>();
                falseComponents->push_back(ifstat->ifFalse);
                auto falseState = new IR::ParserState(
                    Util::SourceInfo(), falseName, IR::Annotations::empty, falseComponents,
                    new IR::PathExpression(IR::ID(joinName)));
                states->push_back(falseState);
            }

            // left-over
            auto vec = new IR::Vector<IR::Expression>();
            vec->push_back(ifstat->condition);
            auto trueCase = new IR::SelectCase(
                Util::SourceInfo(), new IR::BoolLiteral(true),
                new IR::PathExpression(IR::ID(trueName)));
            auto falseCase = new IR::SelectCase(
                Util::SourceInfo(), new IR::BoolLiteral(false),
                new IR::PathExpression(IR::ID(falseName)));
            auto cases = new IR::Vector<IR::SelectCase>();
            cases->push_back(trueCase);
            cases->push_back(falseCase);
            currentState->selectExpression = new IR::SelectExpression(
                Util::SourceInfo(), new IR::ListExpression(vec), std::move(*cases));

            currentState->components = currentComponents;
            currentComponents = new IR::IndexedVector<IR::StatOrDecl>();
            currentState = new IR::ParserState(
                Util::SourceInfo(), joinName, IR::Annotations::empty,
                currentComponents, origSelect);  // may be overriten
        } else {
            currentComponents->push_back(c);
        }
    }

    if (states->empty())
        return state;
    states->push_back(currentState);
    return states;
}

Visitor::profile_t DoRemoveParserControlFlow::init_apply(const IR::Node* node) {
    LOG1("DoRemoveControlFlow");
    return Transform::init_apply(node);
}

}  // namespace P4
