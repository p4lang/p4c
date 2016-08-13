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

#ifndef _MIDEND_UNREACHABLESTATES_H_
#define _MIDEND_UNREACHABLESTATES_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/callGraph.h"

namespace P4 {

typedef CallGraph<const IR::ParserState*> CG;

class ScanParser : public Inspector {
    const ReferenceMap* refMap;
    CG* transitions;
 public:
    ScanParser(const ReferenceMap* refMap, CG* transitions) :
            refMap(refMap), transitions(transitions) {
        CHECK_NULL(refMap); CHECK_NULL(transitions);
        setName("ScanParser");
    }
    bool preorder(const IR::PathExpression* expression) override {
        auto state = findContext<IR::ParserState>();
        if (state != nullptr) {
            auto decl = refMap->getDeclaration(expression->path);
            if (decl != nullptr && decl->is<IR::ParserState>())
                transitions->calls(state, decl->to<IR::ParserState>());
        }
        return false;
    }
};

class RemoveUnreachableStates : public Transform {
    const CG* transitions;
    std::set<const IR::ParserState*> reachable;
 public:
    RemoveUnreachableStates(const CG*transitions) :
            transitions(transitions)
    { CHECK_NULL(transitions); setName("RemoveUnreachableStates"); }
    Transform::profile_t init_apply(const IR::Node* node) override;
    const IR::Node* preorder(IR::ParserState* state) override;
};

class UnreachableParserStates : public Transform {
    ReferenceMap *refMap;
 public:
    UnreachableParserStates(ReferenceMap *refMap) : refMap(refMap) {
        CHECK_NULL(refMap);
        setName("UnreachableParserStates");
    }

    const IR::Node* preorder(IR::P4Parser* parser) override;
    const IR::Node* preorder(IR::P4Control* control) override
    { prune(); return control; }
};

}  // namespace P4

#endif /* _MIDEND_UNREACHABLESTATES_H_ */
