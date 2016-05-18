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

#ifndef _MIDEND_REMOVEPARAMETERS_H_
#define _MIDEND_REMOVEPARAMETERS_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/typeMap.h"

namespace P4 {

// Remove table parameters by transforming them into local variables.
// This should be run after table parameters have been given unique names.
// control c() {
//   table t(inout bit x) { ... }
//   apply { t.apply(y); }
// }
// becomes
// control c() {
//    bit x;
//    table t() { ... }
//    apply {
//       x = y;  // all in an inout parameters
//       t.apply();
//       y = x;  // all out and inout parameters
// }}
class RemoveTableParameters : public Transform {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    std::set<const IR::P4Table*> original;
 public:
    RemoveTableParameters(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("RemoveTableParameters"); }

    const IR::Node* postorder(IR::P4Table* table) override;
    // These should be all statements that may contain a table apply
    // after the program has been simplified
    const IR::Node* postorder(IR::MethodCallStatement* statement) override;
    const IR::Node* postorder(IR::IfStatement* statement) override;
    const IR::Node* postorder(IR::SwitchStatement* statement) override;
 protected:
    void doParameters(const IR::ParameterList* params,
                      const IR::Vector<IR::Expression>* args,
                      IR::IndexedVector<IR::StatOrDecl>* result,
                      bool in);
};

}  // namespace P4

#endif /* _MIDEND_REMOVEPARAMETERS_H_ */
