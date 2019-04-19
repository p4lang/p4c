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

#ifndef _FRONTENDS_P4_USELESSCASTS_H_
#define _FRONTENDS_P4_USELESSCASTS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
Removes casts where the input expression has the exact same type
as the cast type
*/
class RemoveUselessCasts : public Transform {
    const P4::TypeMap* typeMap;

 public:
    explicit RemoveUselessCasts(const P4::TypeMap* typeMap): typeMap(typeMap)
    { CHECK_NULL(typeMap); setName("RemoveUselessCasts"); }
    const IR::Node* postorder(IR::Cast* cast) override;
};

class UselessCasts : public PassManager {
 public:
    UselessCasts(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new RemoveUselessCasts(typeMap));
        setName("UselessCasts");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_USELESSCASTS_H_ */
