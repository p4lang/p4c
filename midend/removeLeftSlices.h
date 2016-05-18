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
#include "frontends/common/typeMap.h"

namespace P4 {

// Remove Slices on the lhs of an assignment
class RemoveLeftSlices : public Transform {
    P4::TypeMap* typeMap;
 public:
    explicit RemoveLeftSlices(P4::TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); setName("RemoveLeftSlices"); }
    const IR::Node* postorder(IR::AssignmentStatement* stat) override;
};

}  // namespace P4

#endif /* _MIDEND_REMOVELEFTSLICES_H_ */
