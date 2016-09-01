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

#ifndef _MIDEND_PARSERUNROLL_H_
#define _MIDEND_PARSERUNROLL_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/callGraph.h"
#include "interpreter.h"

namespace P4 {

//////////////////////////////////////////////
// The following are for a single parser

// Information produced for a parser state by the symbolic evaluator
struct ParserStateInfo {
    const IR::P4Parser*    parser;
    const IR::ParserState* state;  // original state this is produced from
    const ParserStateInfo* predecessor;  // how we got here in the symbolic evaluation
    cstring                name;  // new state name
    ValueMap*              before;
    ValueMap*              after;

    ParserStateInfo(cstring name, const IR::P4Parser* parser, const IR::ParserState* state,
                    const ParserStateInfo* predecessor, ValueMap* before) :
            parser(parser), state(state), predecessor(predecessor),
            name(name), before(before), after(nullptr)
    { CHECK_NULL(parser); CHECK_NULL(state); CHECK_NULL(before); }
};

// Information produced for a parser by the symbolic evaluator
class ParserInfo {
    // for each original state a vector of states produced by unrolling
    std::map<cstring, std::vector<ParserStateInfo*>*> states;
 public:
    std::vector<ParserStateInfo*>* get(cstring origState) {
        std::vector<ParserStateInfo*> *vec;
        auto it = states.find(origState);
        if (it == states.end()) {
            vec = new std::vector<ParserStateInfo*>;
            states.emplace(origState, vec);
        } else {
            vec = it->second;
        }
        return vec;
    }
    void add(ParserStateInfo* si) {
        cstring origState = si->state->name.name;
        auto vec = get(origState);
        vec->push_back(si);
    }
};

typedef CallGraph<const IR::ParserState*> StateCallGraph;

// Information about a parser in the input program
class ParserStructure {
    std::map<cstring, const IR::ParserState*> stateMap;
    StateCallGraph* callGraph;
 public:
    const IR::P4Parser*    parser;
    const IR::ParserState* start;
    const ParserInfo*      result;
    void setParser(const IR::P4Parser* parser) {
        CHECK_NULL(parser);
        callGraph = new StateCallGraph(parser->name);
        this->parser = parser;
        start = nullptr;
    }
    void addState(const IR::ParserState* state)
    { stateMap.emplace(state->name, state); }
    const IR::ParserState* get(cstring state) const
    { return ::get(stateMap, state); }
    void calls(const IR::ParserState* caller, const IR::ParserState* callee)
    { callGraph->calls(caller, callee); }

    void analyze(ReferenceMap* refMap, TypeMap* typeMap, bool unroll);
};

class AnalyzeParser : public Inspector {
    const ReferenceMap* refMap;
    ParserStructure*    current;
 public:
    AnalyzeParser(const ReferenceMap* refMap, ParserStructure* current) :
            refMap(refMap), current(current) {
        CHECK_NULL(refMap); CHECK_NULL(current); setName("AnalyzeParser");
        visitDagOnce = false;
    }

    bool preorder(const IR::ParserState* state) override;
    void postorder(const IR::PathExpression* expression) override;
};

#if 0
class ParserUnroller : public Transform {
    ReferenceMap*    refMap;
    TypeMap*         typeMap;
    ParserStructure* parser;
 public:
    ParserUnroller(ReferenceMap* refMap, TypeMap* typeMap, ParserStructure* parser) :
            refMap(refMap), typeMap(typeMap), parser(parser) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(parser);
        setName("ParserUnroller");
        visitDagOnce = false;
    }
};
#endif

// Applied to a P4Parser object.
class ParserRewriter : public PassManager {
    ParserStructure  current;
 public:
    ParserRewriter(ReferenceMap* refMap, TypeMap* typeMap, bool unroll) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        passes.push_back(new AnalyzeParser(refMap, &current));
        passes.push_back(new VisitFunctor (
            [this, refMap, typeMap, unroll](const IR::Node* root) -> const IR::Node* {
                current.analyze(refMap, typeMap, unroll);
                return root;
            }));
#if 0
        if (unroll)
            passes.push_back(new ParserUnroller(refMap, typeMap, &current));
#endif
    }
    Visitor::profile_t init_apply(const IR::Node* node) override {
        LOG1("Scanning " << node);
        BUG_CHECK(node->is<IR::P4Parser>(), "%1%: expected a parser", node);
        current.parser = node->to<IR::P4Parser>();
        return Visitor::init_apply(node);
    }
};

///////////////////////////////////////////////////////
// The following are applied to the entire program

class RewriteAllParsers : public Transform {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    bool          unroll;
 public:
    RewriteAllParsers(ReferenceMap* refMap, TypeMap* typeMap, bool unroll) :
            refMap(refMap), typeMap(typeMap), unroll(unroll)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); }
    const IR::Node* postorder(IR::P4Parser* parser) override {
        ParserRewriter rewriter(refMap, typeMap, unroll);
        return parser->apply(rewriter);
    }
};

class ParsersUnroll : public PassManager {
 public:
    ParsersUnroll(bool unroll, ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new RewriteAllParsers(refMap, typeMap, unroll));
        setName("ParsersUnroll");
    }
};

}  // namespace P4

#endif /* _MIDEND_PARSERUNROLL_H_ */
