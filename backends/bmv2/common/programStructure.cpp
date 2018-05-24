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

#include "programStructure.h"

namespace BMV2 {

void DiscoverStructure::postorder(const IR::ParameterList *paramList) {
    bool inAction = findContext<IR::P4Action>() != nullptr;
    unsigned index = 0;
    for (auto p : *paramList->getEnumerator()) {
        structure->index.emplace(p, index);
        if (!inAction)
            structure->nonActionParameters.emplace(p);
        index++;
    }
}

void DiscoverStructure::postorder(const IR::P4Action *action) {
    LOG2("discovered action " << action);
    auto control = findContext<IR::P4Control>();
    structure->actions.emplace(action, control);
}

void DiscoverStructure::postorder(const IR::Declaration_Variable *decl) {
    structure->variables.push_back(decl);
}

void DiscoverStructure::postorder(const IR::Type_Error *errors) {
    auto &map = structure->errorCodesMap;
    for (auto m : *errors->getDeclarations()) {
        BUG_CHECK(map.find(m) == map.end(), "Duplicate error");
        map[m] = map.size();
    }
}

void DiscoverStructure::postorder(const IR::Declaration_MatchKind* kind) {
    for (auto member : kind->members) {
        structure->match_kinds.insert(member->name);
    }
}

}  // namespace BMV2
