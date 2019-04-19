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

#ifndef _MIDEND_TABLEHIT_H_
#define _MIDEND_TABLEHIT_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
Convert
tmp = t.apply().hit
   into
if (t.apply().hit)
   tmp = true;
else
   tmp = false;
This may be needed by some back-ends which only support hit test in conditionals
*/
class DoTableHit : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;
 public:
    DoTableHit(ReferenceMap* refMap, TypeMap* typeMap):
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("DoTableHit"); }
    const IR::Node* postorder(IR::AssignmentStatement* statement) override;
};

class TableHit : public PassManager {
 public:
    TableHit(ReferenceMap* refMap, TypeMap* typeMap,
             TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoTableHit(refMap, typeMap));
        setName("TableHit");
    }
};

}  // namespace P4

#endif /* _MIDEND_TABLEHIT_H_ */
