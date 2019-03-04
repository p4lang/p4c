/*
Copyright 2019 VMware, Inc.

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

#ifndef _MIDEND_ELIMINATEADVANCE_H_
#define _MIDEND_ELIMINATEADVANCE_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
 * Convert an packet.advance into an extract into an unused variable.

packet.advance(20);

 * becomes

header Tmp { bit<8 * 20> field; }

state s {
   Tmp tmp;
   packet.extract(tmp);
}

 * while

packet.advance(x);

 * becomes

header Var { varbit<8 * 32> field; }
Var var;

state s0 {
   transition select(x > 32) {
       false: full;
       true:  last;
   }
}

state full {
    packet.extract(var, 32);
    x = x - 32;
    transition s0;
 }

state last {
    packet.extract(var, x);
    transition done;
}
 */
class DoEliminateAdvance : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;
 public:
    DoEliminateAdvance(ReferenceMap* refMap, TypeMap* typeMap):
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("DoEliminateAdvance"); }
    const IR::Node* postorder(IR::MethodCallStatement* statement) override;
};

class EliminateAdvance : public PassManager {
 public:
    EliminateAdvance(ReferenceMap* refMap, TypeMap* typeMap) {
        setName("EliminateAdvance");
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DoEliminateAdvance(refMap, typeMap));
    }
};

}  // namespace P4

#endif /* _MIDEND_ELIMINATEADVANCE_H_ */
