// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "backends/common/programStructure.h"

#include "lib/log.h"

namespace P4 {

void DiscoverStructure::postorder(const IR::ParameterList *paramList) {
    bool inAction = findContext<IR::P4Action>() != nullptr;
    unsigned index = 0;
    for (const auto *p : *paramList->getEnumerator()) {
        structure->index.emplace(p, index);
        if (!inAction) {
            structure->nonActionParameters.emplace(p);
        }
        index++;
    }
}

void DiscoverStructure::postorder(const IR::P4Action *action) {
    LOG2("discovered action " << action);
    const auto *control = findContext<IR::P4Control>();
    structure->actions.emplace(action, control);
}

void DiscoverStructure::postorder(const IR::Declaration_Variable *decl) {
    structure->variables.push_back(decl);
}

void DiscoverStructure::postorder(const IR::Type_Error *errors) {
    auto &map = structure->errorCodesMap;
    for (const auto *m : *errors->getDeclarations()) {
        BUG_CHECK(map.find(m) == map.end(), "Duplicate error");
        map[m] = map.size();
    }
}

void DiscoverStructure::postorder(const IR::Declaration_MatchKind *kind) {
    for (const auto *member : kind->members) {
        structure->match_kinds.insert(member->name);
    }
}

}  // namespace P4
