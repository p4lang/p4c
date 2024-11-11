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
