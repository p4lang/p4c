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

#include "duplicateHierarchicalNameCheck.h"

#include "lib/log.h"

namespace P4 {

cstring DuplicateHierarchicalNameCheck::getName(const IR::IDeclaration *decl) {
    return decl->getName();
}

const IR::Node *DuplicateHierarchicalNameCheck::postorder(IR::Annotation *annotation) {
    if (annotation->name != IR::Annotation::nameAnnotation) return annotation;

    cstring name = annotation->getName();
    if (!name.startsWith(".")) {
        std::string newName = "";
        for (cstring s : stack) newName += s + ".";
        newName += name;
        name = newName;
    }
    // The node the annotation belongs to
    CHECK_NULL(getContext()->parent);
    auto *annotatedNode = getContext()->parent->node;
    CHECK_NULL(annotatedNode);
    // variable and struct declarations are fine if they have identical
    // name annotations, and such name annotations can be synthesized by
    // p4c before this pass.  Ignore them.
    if (annotatedNode->is<IR::Declaration_Variable>() ||
        annotatedNode->is<IR::Type_Struct>()) {
        return annotation;
    }
    bool foundDuplicate = false;
    auto *otherNode = annotatedNode;
    if (annotatedNode->is<IR::P4Table>()) {
        if (annotatedTables.count(name)) {
            foundDuplicate = true;
            otherNode = annotatedTables[name];
        } else {
            annotatedTables[name] = annotatedNode;
        }
    } else if (annotatedNode->is<IR::P4Action>()) {
        if (annotatedActions.count(name)) {
            foundDuplicate = true;
            otherNode = annotatedActions[name];
        } else {
            annotatedActions[name] = annotatedNode;
        }
    } else {
        if (annotatedOthers.count(name)) {
            foundDuplicate = true;
            otherNode = annotatedOthers[name];
        } else {
            annotatedOthers[name] = annotatedNode;
        }
    }
    if (foundDuplicate) {
        error(ErrorType::ERR_DUPLICATE, "%1%: " ERR_STR_DUPLICATED_NAME ": %2%", annotatedNode,
              otherNode);
    }
    return annotation;
}

}  // namespace P4
