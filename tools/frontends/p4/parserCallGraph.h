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

#ifndef _FRONTENDS_P4_PARSERCALLGRAPH_H_
#define _FRONTENDS_P4_PARSERCALLGRAPH_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/callGraph.h"

namespace P4 {

typedef CallGraph<const IR::ParserState*> ParserCallGraph;

/** @brief Builds a CallGraph of ParserState nodes.
  */
class ComputeParserCG : public Inspector {
    const ReferenceMap* refMap;
    ParserCallGraph* transitions;

 public:
    ComputeParserCG(const ReferenceMap* refMap, /* out */ParserCallGraph* transitions) :
            refMap(refMap), transitions(transitions) {
        CHECK_NULL(refMap); CHECK_NULL(transitions);
        setName("ComputeParserCG");
    }
    bool preorder(const IR::PathExpression* expression) override;
    void postorder(const IR::SelectExpression* expression) override;
};

}  // namespace P4

#endif /* _FRONTENDS_P4_PARSERCALLGRAPH_H_ */
