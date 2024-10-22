/*
Copyright 2024 Cisco Systems, Inc.

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

#include "duplicateActionControlPlaneNameCheck.h"

#include "lib/log.h"

namespace P4 {

cstring DuplicateActionControlPlaneNameCheck::getName(const IR::IDeclaration *decl) {
    return decl->getName();
}

void DuplicateActionControlPlaneNameCheck::checkForDuplicateName(cstring name,
                                                                 const IR::Node *node) {
    bool foundDuplicate = false;
    auto *otherNode = node;
    auto [it, inserted] = actions.insert(std::pair(name, node));
    if (!inserted) {
        foundDuplicate = true;
        otherNode = (*it).second;
    }
    if (foundDuplicate) {
        error(ErrorType::ERR_DUPLICATE, "%1%: conflicting control plane name: %2%", node,
              otherNode);
    }
}

const IR::Node *DuplicateActionControlPlaneNameCheck::postorder(IR::P4Action *action) {
    // Actions declared at the top level that the developer did not
    // write a @name annotation for it, should be included in the
    // duplicate name check.  They do not yet have a @name annotation
    // by the time this pass executes, so this method will handle
    // such actions.
    if (findContext<IR::P4Control>() == nullptr) {
        auto annos = action->annotations;
        bool hasNameAnnotation = false;
        if (annos != nullptr) {
            auto nameAnno = annos->getSingle(IR::Annotation::nameAnnotation);
            if (nameAnno) {
                hasNameAnnotation = true;
            }
        }
        if (!hasNameAnnotation) {
            // name is what this top-level action's @name annotation
            // will be, after LocalizeAllActions pass adds one.
            cstring name = "." + action->name;
            checkForDuplicateName(name, action);
        }
    }
    return action;
}

const IR::Node *DuplicateActionControlPlaneNameCheck::postorder(IR::Annotation *annotation) {
    if (annotation->name != IR::Annotation::nameAnnotation) return annotation;
    // The node the annotation belongs to
    CHECK_NULL(getContext()->parent);
    auto *annotatedNode = getContext()->parent->node;
    CHECK_NULL(annotatedNode);
    // We are only interested in name annotations on actions here, as
    // the P4Runtime API control plane generation code already checks
    // for duplicate control plane names for other kinds of objects.
    if (!annotatedNode->is<IR::P4Action>()) {
        return annotation;
    }
    cstring name = annotation->getName();
    if (!name.startsWith(".")) {
        if (!stack.empty()) {
            name = absl::StrCat(absl::StrJoin(stack, ".",
                                              [](std::string *out, cstring s) {
                                                  absl::StrAppend(out, s.string_view());
                                              }),
                                ".", name.string_view());
        }
    }
    checkForDuplicateName(name, annotatedNode);
    return annotation;
}

}  // namespace P4
