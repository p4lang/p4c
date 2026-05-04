// Copyright 2017 VMware, Inc.
// SPDX-FileCopyrightText: 2017 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "globals.h"

#include "extern.h"
#include "sharedActionSelectorCheck.h"

namespace P4::BMV2 {

bool ConvertGlobals::preorder(const IR::ExternBlock *block) {
    LOG2("Converting " << block);
    // This object will be lost, but we don't care about
    // global action profiles here; they are synthesized also
    // from each table that uses them.
    auto action_profiles = new Util::JsonArray();
    ctxt->action_profiles = action_profiles;
    ExternConverter::cvtExternInstance(ctxt, block->node->to<IR::Declaration>(),
                                       block->to<IR::ExternBlock>(), emitExterns);
    return false;
}

}  // namespace P4::BMV2
