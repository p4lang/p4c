// Copyright 2024 Marvell Technology, Inc.
// SPDX-FileCopyrightText: 2024 Marvell Technology, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "backends/common/portableProgramStructure.h"

namespace P4 {

using namespace P4::literals;

bool InspectPortableProgram::isHeaders(const IR::Type_StructLike *st) {
    bool result = false;
    for (auto f : st->fields) {
        if (f->type->is<IR::Type_Header>() || f->type->is<IR::Type_Array>()) {
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
