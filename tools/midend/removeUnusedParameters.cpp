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

#include "removeUnusedParameters.h"

#include <algorithm>
#include "lib/enumerator.h"

namespace P4 {

const IR::Node* RemoveUnusedActionParameters::postorder(IR::P4Action* action) {
    auto params = action->parameters;

    // If all of the parameters to this action are used, there's nothing to do.
    auto isUsed = [&](const IR::Parameter* p) { return refMap->isUsed(p); };
    if (std::all_of(params->begin(), params->end(), isUsed)) return action;

    // Some parameters are unused; filter them out.
    auto newParams = new IR::ParameterList;
    for (auto param : *params->getEnumerator()->where(isUsed))
        newParams->push_back(param);

    action->parameters = newParams;
    return action;
}

}  // namespace P4
