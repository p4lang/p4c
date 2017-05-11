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

#ifndef _MIDEND_REMOVELEFTSLICES_H_
#define _MIDEND_REMOVELEFTSLICES_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
 * This pass removes Slices on the lhs of an assignment
 *
 * \code{.cpp}
 * a[m:l] = e;  ->  a = (a & ~mask) | (((cast)e << l) & mask);
 * \endcode
 *
 * @pre none
 * @post no field slice operator in the lhs of assignment statement
 */
class DoRemoveLeftSlices : public Transform {
    P4::TypeMap* typeMap;
 public:
    explicit DoRemoveLeftSlices(P4::TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); setName("DoRemoveLeftSlices"); }
    const IR::Node* postorder(IR::AssignmentStatement* stat) override;
};

class RemoveLeftSlices : public PassManager {
 public:
    RemoveLeftSlices(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new P4::TypeChecking(refMap, typeMap));
        passes.push_back(new DoRemoveLeftSlices(typeMap));
        setName("RemoveLeftSlices");
    }
};

}  // namespace P4

#endif /* _MIDEND_REMOVELEFTSLICES_H_ */
