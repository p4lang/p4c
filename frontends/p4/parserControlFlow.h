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

#ifndef FRONTENDS_P4_PARSERCONTROLFLOW_H_
#define FRONTENDS_P4_PARSERCONTROLFLOW_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/moveDeclarations.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/uniqueNames.h"
#include "ir/ir.h"

namespace P4C::P4 {

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
    MinimalNameGenerator nameGen;

 public:
    DoRemoveParserControlFlow() { setName("DoRemoveParserControlFlow"); }
    const IR::Node *postorder(IR::ParserState *state) override;
    Visitor::profile_t init_apply(const IR::Node *node) override;
};

/// Iterates DoRemoveParserControlFlow and SimplifyControlFlow until
/// convergence.
class RemoveParserControlFlow : public PassRepeated {
 public:
    explicit RemoveParserControlFlow(TypeMap *typeMap) : PassRepeated({}) {
        passes.emplace_back(new DoRemoveParserControlFlow());
        passes.emplace_back(new SimplifyControlFlow(typeMap));
        setName("RemoveParserControlFlow");
    }
};

/// Detect whether the program contains an 'if' statement in a parser
class IfInParser : public Inspector {
    bool *found;

 public:
    explicit IfInParser(bool *found) : found(found) {
        CHECK_NULL(found);
        setName("IfInParser");
    }
    void postorder(const IR::IfStatement *) override {
        if (findContext<IR::P4Parser>()) *found = true;
    }
};

/// This pass wraps RemoveParserControlFlow by establishing the preconditions
/// that need to be present before running it.
class RemoveParserIfs : public PassManager {
    bool found = false;

 public:
    explicit RemoveParserIfs(TypeMap *typeMap) {
        passes.push_back(new IfInParser(&found));
        passes.push_back(
            new PassIf([this] { return found; },
                       {                    // only do this if we found an 'if' in a parser
                        new UniqueNames(),  // Give each local declaration a unique internal name
                        new MoveDeclarations(true),  // Move all local declarations to the beginning
                        new RemoveParserControlFlow(typeMap)}));
    }
};

}  // namespace P4C::P4

#endif /* FRONTENDS_P4_PARSERCONTROLFLOW_H_ */
