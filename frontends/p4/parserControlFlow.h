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

#ifndef _MIDEND_PARSERCONTROLFLOW_H_
#define _MIDEND_PARSERCONTROLFLOW_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/simplify.h"

namespace P4 {

// Converts if statements in parsers into transitions.
// This should be run after variables have been moved to the "top" of
// the parser.
class DoRemoveParserControlFlow : public Transform {
    ReferenceMap* refMap;
 public:
    explicit DoRemoveParserControlFlow(ReferenceMap* refMap) : refMap(refMap)
    { CHECK_NULL(refMap); setName("DoRemoveParserControlFlow"); }
    const IR::Node* postorder(IR::ParserState* state) override;
    Visitor::profile_t init_apply(const IR::Node* node) override;
};

class RemoveParserControlFlow : public PassRepeated {
 public:
    RemoveParserControlFlow(ReferenceMap* refMap, TypeMap* typeMap) : PassRepeated({}) {
        passes.emplace_back(new DoRemoveParserControlFlow(refMap));
        passes.emplace_back(new SimplifyControlFlow(refMap, typeMap));
        setName("RemoveParserControlFlow");
    }
};

}  // namespace P4

#endif /* _MIDEND_PARSERCONTROLFLOW_H_ */
