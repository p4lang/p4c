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

#include "unreachableStates.h"

namespace P4 {

Transform::profile_t RemoveUnreachableStates::init_apply(const IR::Node* node) {
    BUG_CHECK(node->is<IR::P4Parser>(), "%1%: expected a P4Parser", node);
    auto parser = node->to<IR::P4Parser>();
    auto start = parser->getDeclByName(IR::ParserState::start);
    if (start == nullptr) {
        ::error("%1%: parser does not have a `start' state", parser);
    } else {
        transitions->reachable(start->to<IR::ParserState>(), reachable);
        LOG1("Parser " << node << " has " << reachable.size() << " reachable states");
    }
    return Transform::init_apply(node);
}

const IR::Node* RemoveUnreachableStates::preorder(IR::ParserState* state) {
    if (state->name == IR::ParserState::start ||
        state->name == IR::ParserState::reject ||
        state->name == IR::ParserState::accept)
        return state;
    auto orig = getOriginal<IR::ParserState>();
    if (reachable.find(orig) == reachable.end()) {
        LOG1("Removing unreachable state " << state);
        return nullptr;
    }
    return state;
}

const IR::Node* UnreachableParserStates::preorder(IR::P4Parser* parser) {
    CG transitions("transitions");
    ScanParser scanner(refMap, &transitions);
    RemoveUnreachableStates remove(&transitions);

    (void)parser->apply(scanner);
    return parser->apply(remove);
}

}  // namespace P4
