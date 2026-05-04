// Copyright 2024 Cisco Systems, Inc.
// SPDX-FileCopyrightText: 2024 Cisco Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "duplicateActionControlPlaneNameCheck.h"

#include "lib/log.h"

namespace P4 {

cstring DuplicateActionControlPlaneNameCheck::getName(const IR::IDeclaration *decl) {
    return decl->getName();
}

void DuplicateActionControlPlaneNameCheck::checkForDuplicateName(cstring name,
                                                                 const IR::Node *node) {
    auto *otherNode = node;
    auto [it, inserted] = actions.emplace(name, node);
    if (!inserted) {
        otherNode = it->second;
        error(ErrorType::ERR_DUPLICATE, "%1%: conflicting control plane name: %2%", node,
              otherNode);
    }
}

const IR::Node *DuplicateActionControlPlaneNameCheck::postorder(IR::P4Action *action) {
    bool topLevel = stack.empty();
    auto nameAnno = action->getAnnotation(IR::Annotation::nameAnnotation);
    if (!nameAnno && topLevel) {
        // Actions declared at the top level that the developer did not
        // write a @name annotation for it, should be included in the
        // duplicate name check.  They do not yet have a @name annotation
        // by the time this pass executes, so this case will handle
        // such actions.
        //
        // name is what this top-level action's @name annotation
        // will be, after LocalizeAllActions pass adds one.
        cstring name = absl::StrCat(".", action->name);
        checkForDuplicateName(name, action);
    } else {
        const auto *e0 = nameAnno->getExpr(0);
        cstring name = e0->to<IR::StringLiteral>()->value;
        if (!name.startsWith(".")) {
            // Create an absolute hierarchical name (one beginning
            // with a "."), so we can compare it against any other
            // @name annotation value that begins with "." and is
            // equal.
            if (topLevel) {
                name = absl::StrCat(".", name);
            } else {
                name = absl::StrCat(".", absl::StrJoin(stack, "."), ".", name);
            }
        }
        checkForDuplicateName(name, action);
    }
    return action;
}

}  // namespace P4
