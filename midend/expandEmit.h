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

#ifndef _MIDEND_EXPANDEMIT_H_
#define _MIDEND_EXPANDEMIT_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
 * Convert an emit of a struct recursively into a sequence of emits of
 * all fields of the struct.  Convert an emit of a header stack into a
 * sequence of emits of all elements of the stack.
 * header H1 {}
 * header H2 {}
 * struct S {
 *    H1 h1;
 *    H2[2] h2;
 * }
 * S s;
 * packet.emit(s);
 *
 * becomes
 *
 * emit(s.h1);
 * emit(s.h2[0]);
 * emit(s.h2[1]);
 */
class DoExpandEmit : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;
 public:
    DoExpandEmit(ReferenceMap* refMap, TypeMap* typeMap):
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("DoExpandEmit"); }
    // return true if the expansion produced something "new"
    bool expandArg(const IR::Type* type, const IR::Argument* argument,
                   std::vector<const IR::Argument*> *result,
                   std::vector<const IR::Type*> *resultTypes);
    const IR::Node* postorder(IR::MethodCallStatement* statement) override;
};

class ExpandEmit : public PassManager {
 public:
    ExpandEmit(ReferenceMap* refMap, TypeMap* typeMap,
            TypeChecking* typeChecking = nullptr) {
        setName("ExpandEmit");
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoExpandEmit(refMap, typeMap));
    }
};

}  // namespace P4

#endif /* _MIDEND_EXPANDEMIT_H_ */
