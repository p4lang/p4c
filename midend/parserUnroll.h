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
#include "frontends/p4/createBuiltins.h"
#include "frontends/p4/callGraph.h"
#include "interpreter.h"

namespace P4 {

/// Name of out of bound state
const char outOfBoundsStateName[] = "stateOutOfBound";

//////////////////////////////////////////////
// The following are for a single parser

/// Information produced for a parser state by the symbolic evaluator
struct ParserStateInfo {
    friend class ParserStateRewriter;
    const IR::P4Parser*             parser;
    const IR::ParserState*          state;  // original state this is produced from
    IR::ParserState*                newState;  // pointer to a new state
    const ParserStateInfo*          predecessor;  // how we got here in the symbolic evaluation
    std::map<cstring,size_t>        statesIndexes;  // global map in state indexes
    cstring                         name;  // new state name
    ValueMap*                       before;
    ValueMap*                       after;
    std::unordered_set<cstring>     scenarioStates;  // set of scenario states.
    std::unordered_set<cstring>     scenarioHS;  // scenario header stack's operations
    size_t                          currentIndex;
    ParserStateInfo(cstring name, const IR::P4Parser* parser, const IR::ParserState* state,
                    const ParserStateInfo* predecessor, ValueMap* before, size_t index) :
            parser(parser), state(state), predecessor(predecessor),
            name(name), before(before), newState(nullptr), after(nullptr), currentIndex(index)
    { 
        CHECK_NULL(parser); CHECK_NULL(state); CHECK_NULL(before);
        if (predecessor){
            statesIndexes = predecessor->statesIndexes;
            scenarioHS = predecessor->scenarioHS;
        }
    }
};

/// Information produced for a parser by the symbolic evaluator
class ParserInfo {
    friend class RewriteAllParsers;
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
    std::map<cstring, std::vector<ParserStateInfo*>*>& getStates() {
        return states;
    }
};

typedef CallGraph<const IR::ParserState*> StateCallGraph;

/// Information about a parser in the input program
class ParserStructure {
    friend class ParserStateRewriter;
    friend class ParserSymbolicInterpreter;
    friend class AnalyzeParser;
    std::map<cstring, const IR::ParserState*> stateMap;

 public:
    const IR::P4Parser*    parser;
    const IR::ParserState* start;
    const ParserInfo*      result;
    StateCallGraph*        callGraph;
    std::map<cstring, std::set<cstring> > statesWithHeaderStacks;
    std::map<cstring,size_t>  callsIndexes;  // map for curent calls of state insite current one
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

    void analyze(ReferenceMap* refMap, TypeMap* typeMap, bool unroll, bool& hasOutOfboundState);
    /// check reachability for usage of header stack
    bool reachableHSUsage(IR::ID id, ParserStateInfo* state);
 protected:
    /// evaluates rechable states with HS operations for each path.
    void evaluateReachability();
    /// add HS name which is used in a current state.
    void addStateHSUsage(const IR::ParserState* state, const IR::Expression* expression);
};

class AnalyzeParser : public Inspector {
    const ReferenceMap* refMap;
    ParserStructure*    current;
    const IR::ParserState* currentState;
 public:
    AnalyzeParser(const ReferenceMap* refMap, ParserStructure* current) :
            refMap(refMap), current(current), currentState(nullptr) {
        CHECK_NULL(refMap); CHECK_NULL(current); setName("AnalyzeParser");
        visitDagOnce = false;
    }

    bool preorder(const IR::P4Parser* parser) override {
        LOG2("Scanning " << parser);
        current->setParser(parser);
        return true;
    }
    bool preorder(const IR::ParserState* state) override;
    void postorder(const IR::ParserState* state) override;
    void postorder(const IR::ArrayIndex* array) override;
    void postorder(const IR::Member* member) override;
    void postorder(const IR::PathExpression* expression) override;

};

#if 0
class ParserUnroller : public Transform {
    ReferenceMap*           refMap;
    TypeMap*                typeMap;
    ParserStructure*        parser;
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
    ParserStructure         current;
    friend class RewriteAllParsers;
 public:
    bool hasOutOfboundState;
    ParserRewriter(ReferenceMap* refMap,
                   TypeMap* typeMap, bool unroll) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        setName("ParserRewriter");
        addPasses({
            new AnalyzeParser(refMap, &current),
            [this, refMap, typeMap, unroll](void) {
                current.analyze(refMap, typeMap, unroll, hasOutOfboundState); },
#if 0
            unroll ? new ParserUnroller(refMap, typeMap, &current) : 0,
#endif
        });
    }
};

///////////////////////////////////////////////////////
// The following are applied to the entire program

class RewriteAllParsers : public Transform {
    ReferenceMap*           refMap;
    TypeMap*                typeMap;
    bool                    unroll;
    const IR::ParserState*  outOfBoundsState;
    const IR::Type_Declaration* stdMetadataType;
    const IR::Type_Declaration* error;
 public:
    RewriteAllParsers(ReferenceMap* refMap, TypeMap* typeMap, bool unroll) :
            refMap(refMap), typeMap(typeMap), unroll(unroll), outOfBoundsState(nullptr),
            stdMetadataType(nullptr), error(nullptr)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("RewriteAllParsers"); }

    // generate OutOfBound state
    const IR::Node* preorder(IR::P4Program *p4Program) override {
        // getting error variable
        const auto errorEnum = p4Program->getDeclsByName("error");
        BUG_CHECK(errorEnum, "Can't find error variable");
        const auto errorVector = errorEnum->toVector();
        BUG_CHECK(errorVector->size(), "Can't find error variable %1%", errorVector);
        const auto error0 = errorVector->at(0);
        BUG_CHECK(error0->is<IR::Type_Declaration>(), "Can't translate error to expression");
        error = error0->to<IR::Type_Declaration>();

        const auto stdMetadataTypeEnum = p4Program->getDeclsByName("standard_metadata_t");
        BUG_CHECK(stdMetadataTypeEnum, "Can't find standard_metadata_t type");
        const auto stdMetadataTypeVector = stdMetadataTypeEnum->toVector();
        BUG_CHECK(stdMetadataTypeVector->size(), "Can't find standard_metadata_t type");
        BUG_CHECK(stdMetadataTypeVector->at(0)->is<IR::Type_Declaration>(),
                  "Can't translate standard_metadata_t type");
        stdMetadataType = stdMetadataTypeVector->at(0)->to<IR::Type_Declaration>();

        return p4Program;
    }

    // start generation of a code
    const IR::Node* postorder(IR::P4Parser* parser) override {
        BUG_CHECK(stdMetadataType, "The standard_metadata_t type was not found");
        
        // getting name of standard_metadata_t parameter
        BUG_CHECK(parser->type, "Parser without type is not supported");
        BUG_CHECK(parser->type->applyParams, "Parser without parameters is not supported");
        const IR::Parameter* metaData = nullptr;
        for (auto& i : parser->type->applyParams->parameters) {
            if (!i->type->is<IR::Type_Name>())
                continue;
            if (i->type->to<IR::Type_Name>()->path->name != stdMetadataType->name)
                continue;
            metaData = i;
            break;
        }

        // generating of out of bounds state
        BUG_CHECK(metaData, "Can't find standard_metadata_t parameter in the parser: %1%", parser);
        BUG_CHECK(error, "Can't find error type");
        const IR::Member* right = new IR::Member(
            new IR::TypeNameExpression(new IR::Type_Name(error->name)), IR::ID("StackOutOfBounds"));

        const IR::Member* left = new IR::Member(new IR::PathExpression(stdMetadataType,
            new IR::Path(metaData->name.name, false)), IR::ID("parser_error"));

        IR::IndexedVector<IR::StatOrDecl> components;
        components.push_back(new IR::AssignmentStatement(left, right));

        const IR::Expression* selectExpression = new IR::PathExpression(new IR::Type_State(),
            new IR::Path("reject", false));
        
        outOfBoundsState = new IR::ParserState(IR::ID(outOfBoundsStateName), components, selectExpression);

        // making rewriting
        auto rewriter = new ParserRewriter(refMap, typeMap, unroll);
        parser->apply(*rewriter);
        /// make a new parser
        BUG_CHECK(rewriter->current.result, "No result was found after unrolling of the parser loop");
        BUG_CHECK(outOfBoundsState, "No StackOutOfBound state was builded");
        IR::P4Parser* newParser = parser->clone();
        IR::IndexedVector<IR::ParserState> states = newParser->states;
        newParser->states.clear();
        if (rewriter->hasOutOfboundState)
            newParser->states.push_back(outOfBoundsState);
        for (auto& i : rewriter->current.result->states) {
            for (auto& j : *i.second)
                if (j->newState)
                    newParser->states.push_back(j->newState);
        }
        return newParser;
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
