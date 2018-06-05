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

#include "uselessCasts.h"

namespace P4 {

const IR::Node*
RemoveUselessCasts::postorder(IR::Cast* cast) {
    auto orig = getOriginal<IR::Cast>();
    auto argType = typeMap->getType(orig->expr, true);
    auto castType = typeMap->getTypeType(orig->destType, true);
    if (TypeMap::equivalent(argType, castType)) {
        LOG3("Eliminating cast " << dbp(cast));
        return cast->expr;
    }
    return cast;
}

}  // namespace P4
