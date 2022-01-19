/*
Copyright 2022 VMware, Inc.

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

#include "checkNameAnnotations.h"

namespace P4 {

bool CheckNameAnnotations::preorder(const IR::Annotation* anno) {
    if (anno->name != IR::Annotation::nameAnnotation)
        return false;
    auto parent = findContext<IR::IAnnotated>();
    auto name = anno->getSingleString();
    BUG_CHECK(parent != nullptr, "%1% parent of annotation not found", anno);
    if (name.isNullOrEmpty()) {
        ::error(ErrorType::ERR_INVALID, "Invalid content for @name annotation %1%", name);
    }
    auto it = annotated.find(name);
    if (it != annotated.end()) {
        ::warning(ErrorType::WARN_SHADOWING,
                  "%1% and %2% use the same name in a @name annotation %3%",
                  parent, it->second, anno);
    } else {
        annotated[name] = parent->getNode();
    }
    return false;
}

}  // namespace P4
