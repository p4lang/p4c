/*
Copyright 2021 VMware, Inc.

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

#ifndef _FRONTENDS_P4_KEYSIDEEFFECT_H_
#define _FRONTENDS_P4_KEYSIDEEFFECT_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {


class KeySideEffect : public PassManager {
 public:
    KeySideEffect(ReferenceMap* refMap, TypeMap* typeMap,
                  TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new KeySideEffect(refMap, typeMap));
        setName("KeySideEffect");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_KEYSIDEEFFECT_H_ */
