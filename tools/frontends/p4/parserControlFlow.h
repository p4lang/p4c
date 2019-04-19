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

#ifndef _FRONTENDS_P4_PARSERCONTROLFLOW_H_
#define _FRONTENDS_P4_PARSERCONTROLFLOW_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/simplify.h"

namespace P4 {

/** @brief Converts if statements in parsers into transitions.
 *
 * For example, this code snippet:
 * 
```
state s {
   statement1;
   statement2;
   if (exp)
      statement3;
   else
      statement4;
   statement5;
   transition selectExpression;
}
```
 * 
 * would be converted into the following four states:
 *
```
state s {
   statement1;
   statement2;
   transition select(exp) {
      true: s_true;
      false: s_false;
   }
}

state s_true {
   statement3;
   transition s_join;
}

state s_false {
   statement4;
   transition s_join;
}

state s_join {
   statement5;
   transition selectExpression;
}
```
 *
 * @pre Must be run after MoveDeclarations.  Requires an up-to-date
 * ReferenceMap.
 * 
 * @post No if statements remain in parsers.
 */

class DoRemoveParserControlFlow : public Transform {
    ReferenceMap* refMap;
 public:
    explicit DoRemoveParserControlFlow(ReferenceMap* refMap) : refMap(refMap)
    { CHECK_NULL(refMap); setName("DoRemoveParserControlFlow"); }
    const IR::Node* postorder(IR::ParserState* state) override;
    Visitor::profile_t init_apply(const IR::Node* node) override;
};


/// Iterates DoRemoveParserControlFlow and SimplifyControlFlow until
/// convergence.
class RemoveParserControlFlow : public PassRepeated {
 public:
    RemoveParserControlFlow(ReferenceMap* refMap, TypeMap* typeMap) : PassRepeated({}) {
        passes.emplace_back(new DoRemoveParserControlFlow(refMap));
        passes.emplace_back(new SimplifyControlFlow(refMap, typeMap));
        setName("RemoveParserControlFlow");
    }
};

}  // namespace P4

#endif  /* _FRONTENDS_P4_PARSERCONTROLFLOW_H_ */
