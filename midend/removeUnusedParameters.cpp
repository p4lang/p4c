// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "removeUnusedParameters.h"

#include <algorithm>

#include "lib/enumerator.h"

namespace P4 {

const IR::Node *RemoveUnusedActionParameters::postorder(IR::P4Action *action) {
    auto params = action->parameters;

    // If all of the parameters to this action are used, there's nothing to do.
    auto isUsed = [&](const IR::Parameter *p) { return refMap->isUsed(p); };
    if (std::all_of(params->begin(), params->end(), isUsed)) return action;

    // Some parameters are unused; filter them out.
    auto newParams = new IR::ParameterList;
    for (auto param : *params->getEnumerator()->where(isUsed)) newParams->push_back(param);

    action->parameters = newParams;
    return action;
}

}  // namespace P4
