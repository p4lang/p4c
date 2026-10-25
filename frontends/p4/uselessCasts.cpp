// Copyright 2017 VMware, Inc.
// SPDX-FileCopyrightText: 2017 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "uselessCasts.h"

namespace P4 {

const IR::Node *RemoveUselessCasts::postorder(IR::Cast *cast) {
    auto orig = getOriginal<IR::Cast>();
    auto argType = typeMap->getType(orig->expr, true);
    auto castType = typeMap->getTypeType(orig->destType, true);
    if (typeMap->equivalent(argType, castType)) {
        LOG3("Eliminating cast " << dbp(cast));
        return cast->expr;
    }
    return cast;
}

}  // namespace P4
