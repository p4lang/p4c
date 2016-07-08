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

// Information produced for a parser state by the symbolic evaluator
struct ParserStateInfo {
    const IR::P4Parser*    parser;
    const IR::ParserState* state;  // original state this is produced from
    const ParserStateInfo* predecessor;  // how we got here in the symbolic evaluation
    cstring                name;  // new state name
    ValueMap*              before;
    ValueMap*              after;
    std::map<cstring, cstring> successors;  // for each successor state the new successor

    ParserStateInfo(cstring name, const IR::P4Parser* parser, const IR::ParserState* state,
                    const ParserStateInfo* predecessor) :
            parser(parser), state(state), predecessor(predecessor),
            name(name), before(nullptr), after(nullptr) {}
    // return true if there are changes in the values
    bool mergeValues(ValueMap* b) {
        if (before == nullptr) {
            before = b;
            return true;
        } else {
            return before->merge(b);
        }
    }
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
    ParserStateInfo* getSingle(cstring origState) {
        auto vec = get(origState);
        if (vec->empty())
            return nullptr;
        BUG_CHECK(vec->size() == 1, "Too many states map to %1%", origState);
        return vec->at(0);
    }
};

// Information about a parser in the input program
class ParserStructure {
    std::map<cstring, const IR::ParserState*> stateMap;
 public:
    const IR::P4Parser*    parser;
    const IR::ParserState* start;
    explicit ParserStructure(const IR::P4Parser* parser) :
            parser(parser), start(nullptr)
    { CHECK_NULL(parser); }
    void addState(const IR::ParserState* state)
    { stateMap.emplace(state->name, state); }
    const IR::ParserState* get(cstring state) const
    { return ::get(stateMap, state); }

    ParserInfo* evaluate(ReferenceMap* refMap, TypeMap* typeMap, bool unroll);
};

class Parsers {
 public:
    std::map<const IR::P4Parser*, ParserStructure*> parsers;
    std::map<const IR::P4Parser*, ParserInfo*> evaluation;
};

// Builds the 'parsers' structure
class DiscoverParsers : public Inspector {
    ParserStructure*    current;
    Parsers*            parsers;
 public:
    DiscoverParsers(Parsers* parsers) :
            current(nullptr), parsers(parsers) {
        CHECK_NULL(parsers); setName("DiscoverParsers");
        visitDagOnce = false;
    }
    bool preorder(const IR::Type*) override { return false; }  // prune
    bool preorder(const IR::P4Parser* parser) override;
    void postorder(const IR::P4Parser* parser) override;
    bool preorder(const IR::ParserState* state) override;
};

// Performs symbolic evaluation over each parser.
// Should be performed after local variable declarations have been removed.
class EvaluateParsers : public Inspector {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    Parsers*      parsers;
    bool          unroll;  // if true unroll the graph into a tree
 public:
    EvaluateParsers(ReferenceMap* refMap, TypeMap* typeMap, Parsers* parsers, bool unroll) :
            refMap(refMap), typeMap(typeMap), parsers(parsers), unroll(unroll) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(parsers);
        setName("ParserEvaluator");
        visitDagOnce = false;
    }
    void postorder(const IR::P4Parser* parser) override;
    bool preorder(const IR::P4Parser*) override { return true; }  // needed to override the next
    bool preorder(const IR::Type*) override { return false; }
};

class ParserUnroll : public PassManager {
    Parsers parsers;
 public:
    ParserUnroll(ReferenceMap* refMap, TypeMap* typeMap, bool isv1) {
        passes.push_back(new TypeChecking(refMap, typeMap, false, isv1));
        passes.push_back(new DiscoverParsers(&parsers));
        passes.push_back(new EvaluateParsers(refMap, typeMap, &parsers, false));
        setName("ParserUnroll");
    }
};

}  // namespace P4

#endif /* _MIDEND_PARSERUNROLL_H_ */
