/*
Copyright 2024 Marvell Technology, Inc.

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

#include "backends/common/portableProgramStructure.h"

namespace P4 {

using namespace P4::literals;

bool InspectPortableProgram::isHeaders(const IR::Type_StructLike *st) {
    bool result = false;
    for (auto f : st->fields) {
        if (f->type->is<IR::Type_Header>() || f->type->is<IR::Type_Stack>()) {
            result = true;
        }
    }
    return result;
}

bool ParsePortableArchitecture::preorder(const IR::ToplevelBlock *block) {
    // Blocks are not in IR tree, use a custom visitor to traverse.
    for (auto it : block->constantValue) {
        if (it.second->is<IR::Block>()) visit(it.second->getNode());
    }
    return false;
}

}  // namespace P4
