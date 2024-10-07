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

#include "duplicateHierarchicalNameCheck.h"

#include "lib/log.h"

namespace P4 {

cstring DuplicateHierarchicalNameCheck::getName(const IR::IDeclaration *decl) { return decl->getName(); }

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
    if (annotatedNodes.count(name)) {
        error(ErrorType::ERR_DUPLICATE, "%1%: " ERR_STR_DUPLICATED_NAME ": %2%",
            annotatedNode, annotatedNodes[name]);
    } else {
        annotatedNodes[name] = annotatedNode;
    }
    return annotation;
}

}  // namespace P4
