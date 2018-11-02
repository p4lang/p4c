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

#ifndef _MIDEND_EXPANDMIRROR_H_
#define _MIDEND_EXPANDMIRROR_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

 /* 
  * the 2nd argument to a mirror call can be a header instance. The
  * objective of this pass is to expand the header instance to a list
  * expression
 */
class ExpandMirror : public Modifier {
    ReferenceMap* refMap;
    TypeMap* typeMap;
 public:
    ExpandMirror(ReferenceMap* refMap, TypeMap* typeMap):
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); }
    void postorder(IR::MethodCallExpression* expr) override;
    void explode(const IR::Expression*, IR::Vector<IR::Expression>*);

};

}  // namespace P4

#endif /* _MIDEND_EXPANDMIRROR_H_ */
