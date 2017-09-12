/*
Copyright 2016 VMware, Inc.

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

#ifndef _MIDEND_COPYSTRUCTURES_H_
#define _MIDEND_COPYSTRUCTURES_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

/**
 * Convert assignments between structures to assignments between fields
 *
 * Examples:
 *   struct src {
 *     bit<32> hdr;
 *   }
 *   struct dst {
 *     bit<32> hdr;
 *   }
 *   action test() {
 *     dst = src;
 *   }
 *
 *   is replaced by
 *
 *   action test() {
 *     dst.hdr = src.hdr;
 *   }
 *
 *   Further, struct initialization is converted to assignment on struct fields
 *
 *   Note, header assignments are not converted in this pass.
 *
 * @pre none
 * @post
 *  - struct to struct copy is replaced by field assignment
 *  - struct initialization is replaced by field assignment
 *
 */
class DoCopyStructures : public Transform {
    TypeMap* typeMap;
    /* Specific targets may allow functions or methods to return structs.
     * Such methods will not be converted in this pass. Setting the
     * errorOnMethodCall flag will produce an error message if such a
     * method is encountered. */
    bool errorOnMethodCall;
 public:
    explicit DoCopyStructures(TypeMap* typeMap, bool errorOnMethodCall) :
            typeMap(typeMap), errorOnMethodCall(errorOnMethodCall)
    { CHECK_NULL(typeMap); setName("DoCopyStructures"); }
    const IR::Node* postorder(IR::AssignmentStatement* statement) override;
};

class CopyStructures : public PassRepeated {
 public:
    CopyStructures(ReferenceMap* refMap, TypeMap* typeMap,
                   bool errorOnMethodCall = true) :
            PassManager({}) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("CopyStructures");
        passes.emplace_back(new TypeChecking(refMap, typeMap));
        passes.emplace_back(new DoCopyStructures(typeMap, errorOnMethodCall));
    }
};

}  // namespace P4

#endif /* _MIDEND_COPYSTRUCTURES_H__ */
