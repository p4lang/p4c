// Copyright 2017 VMware, Inc.
// SPDX-FileCopyrightText: 2017 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "hierarchicalNames.h"

#include "lib/log.h"

namespace P4 {

void HierarchicalNames::postorder(IR::Annotation *annotation) {
    if (annotation->name != IR::Annotation::nameAnnotation) return;

    cstring name = annotation->getName();
    if (name.startsWith(".")) return;

    if (stack.empty()) return;

    std::string newName = absl::StrCat(absl::StrJoin(stack, "."), ".", name);
    LOG2("Changing " << name << " to " << newName);

    annotation->body = IR::Vector<IR::Expression>(new IR::StringLiteral(cstring(newName)));
}

}  // namespace P4
