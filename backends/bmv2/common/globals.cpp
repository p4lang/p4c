/*
Copyright 2017 VMware, Inc.

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

#include "globals.h"

#include <ostream>

#include "extern.h"
#include "ir/node.h"
#include "lib/json.h"
#include "lib/log.h"

namespace BMV2 {

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

}  // namespace BMV2
