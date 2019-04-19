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

#include "parserCallGraph.h"

namespace P4 {

bool ComputeParserCG::preorder(const IR::PathExpression* expression) {
    auto state = findContext<IR::ParserState>();
    if (state != nullptr) {
        auto decl = refMap->getDeclaration(expression->path);
        if (decl != nullptr && decl->is<IR::ParserState>())
            transitions->calls(state, decl->to<IR::ParserState>());
    }
    return false;
}

void ComputeParserCG::postorder(const IR::SelectExpression* expression) {
    // transition (..) { ... } may imply a transition to
    // "reject" - if none of the cases matches.
    for (auto c : expression->selectCases) {
        if (c->keyset->is<IR::DefaultExpression>())
            // If we have a default case this will always match
            return;
    }
    auto state = findContext<IR::ParserState>();
    auto parser = findContext<IR::P4Parser>();
    CHECK_NULL(state);
    CHECK_NULL(parser);
    auto reject = parser->getDeclByName(IR::ParserState::reject);
    CHECK_NULL(reject);
    transitions->calls(state, reject->to<IR::ParserState>());
}

}  // namespace P4
