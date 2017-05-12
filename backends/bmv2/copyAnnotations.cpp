/*
Copyright 2013-present Barefoot Networks, Inc.

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

#include "copyAnnotations.h"

namespace BMV2 {

bool CopyAnnotations::preorder(const IR::PackageBlock* block) {
    auto iter = block->constantValue.begin();
    for (auto arch_iter = block->getConstructorParameters()->begin();
              arch_iter != block->getConstructorParameters()->end();
              arch_iter++, iter++) {
        auto param = *arch_iter;
        if (param->type->is<IR::Type_Specialized>()) {
            auto tp = param->type->to<IR::Type_Specialized>();
            auto decl = refMap->getDeclaration(tp->baseType->path, true);
            auto block = iter->second->to<IR::Block>();
            map->emplace(block, decl->to<IR::Type>());
        }
    }
    return false;
}

}  // namespace BMV2
