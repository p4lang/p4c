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

#include "redundantParsers.h"

namespace P4 {
bool FindRedundantParsers::preorder(const IR::P4Parser *parser) {
    for (const IR::ParserState *state : parser->states) {
        if (state->name != IR::ParserState::start) {
            continue;
        }
        const auto *pathExpr = state->selectExpression->to<IR::PathExpression>();
        if (!pathExpr ||
            pathExpr->path->name != IR::ParserState::accept ||
            !state->components.empty()) {
            continue;
        }
        LOG4("Found redundant parser " << parser->name);
        redundantParsers.insert(parser->type->name);
    }
    return false;
}

const IR::Node *EliminateSubparserCalls::postorder(IR::MethodCallStatement *mcs) {
    auto method = mcs->methodCall->method;
    auto *member = method->to<IR::Member>();
    if (!member || member->member != IR::IApply::applyMethodName) {
        return mcs;
    }
    auto type = typeMap->getType(member->expr);
    if (!type) {
        return mcs;
    }
    auto parser = type->to<IR::Type_Parser>();
    if (!parser) {
        return mcs;
    }
    if (redundantParsers.count(parser->name)) {
        LOG4("Removing apply call to redundant parser " << parser->getName()
             << ": " << *mcs);
        return nullptr;
    }
    return mcs;
}
}
