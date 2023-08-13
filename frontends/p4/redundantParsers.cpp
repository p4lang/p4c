/*
Copyright 2022 Intel Corporation

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

#include <ostream>
#include <string>
#include <vector>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/methodInstance.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "lib/log.h"

namespace P4 {
bool FindRedundantParsers::preorder(const IR::P4Parser *parser) {
    for (const IR::ParserState *state : parser->states) {
        if (state->name != IR::ParserState::start) {
            continue;
        }
        const auto *pathExpr = state->selectExpression->to<IR::PathExpression>();
        if (!pathExpr || pathExpr->path->name != IR::ParserState::accept ||
            !state->components.empty()) {
            continue;
        }
        LOG4("Found redundant parser " << parser->name);
        redundantParsers.insert(parser);
    }
    return false;
}

const IR::Node *EliminateSubparserCalls::postorder(IR::MethodCallStatement *mcs) {
    auto mi = MethodInstance::resolve(mcs->methodCall, refMap, typeMap, true);
    if (!mi->isApply()) return mcs;

    auto apply = mi->to<ApplyMethod>()->applyObject;
    auto parser = apply->to<IR::Type_Parser>();
    if (!parser) return mcs;

    auto declInstance = mi->object->to<IR::Declaration_Instance>();
    if (!declInstance) return mcs;

    auto decl = refMap->getDeclaration(declInstance->type->to<IR::Type_Name>()->path);
    auto p4parser = decl->to<IR::P4Parser>();
    if (!p4parser || !redundantParsers.count(p4parser)) return mcs;

    LOG4("Removing apply call to redundant parser " << parser->getName() << ": " << *mcs);
    return nullptr;
}
}  // namespace P4
