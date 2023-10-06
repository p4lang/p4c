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

#ifndef MIDEND_PARSERUNROLL_H_
#define MIDEND_PARSERUNROLL_H_

#include <unordered_map>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/callGraph.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "interpreter.h"
#include "ir/ir.h"

namespace P4 {

/// Name of out of bound state
const char outOfBoundsStateName[] = "stateOutOfBound";

//////////////////////////////////////////////
// The following are for a single parser

// Represents a variable for storing indexes values for a header stack.
//
// This is a thin wrapper around a 'const IR::Member*' to (1) enforce invariants on which forms of
// Members can represent state variables and (2) enable the use of StackVariable as map keys.
//
// A Member can represent a StackVariable exactly when its qualifying variable
// (IR::Member::expr) either is a PathExpression or can represent a StackVariable.
class StackVariable {
    friend class StackVariableHash;

 public:
    /// Determines whether @expr can represent a StateVariable.
    static bool repOk(const IR::Expression *expr);

    // Implements comparisons so that StateVariables can be used as map keys.
    bool operator==(const StackVariable &other) const;

 private:
    const IR::Expression *variable;

 public:
    /// Implicitly converts IR::Expression* to a StackVariable.
    StackVariable(const IR::Expression *expr);  // NOLINT(runtime/explicit)
};

/// Class with hash function for @a StackVariable.
class StackVariableHash {
 public:
    size_t operator()(const StackVariable &var) const;
};

typedef std::unordered_map<StackVariable, size_t, StackVariableHash> StackVariableMap;
typedef std::unordered_map<StackVariable, const IR::Expression *, StackVariableHash>
    StackVariableIndexMap;

/// Information produced for a parser state by the symbolic evaluator
struct ParserStateInfo {
    friend class ParserStateRewriter;
    cstring name;  // new state name
    const IR::P4Parser *parser;
    const IR::ParserState *state;        // original state this is produced from
    const ParserStateInfo *predecessor;  // how we got here in the symbolic evaluation
    ValueMap *before;
    ValueMap *after;
    IR::ParserState *newState;  // pointer to a new state
    size_t currentIndex;
    StackVariableMap statesIndexes;  // global map in state indexes
    // set of parsers' states names with are in current path.
    std::unordered_set<cstring> scenarioStates;
    std::unordered_set<cstring> scenarioHS;    // scenario header stack's operations
    StackVariableIndexMap substitutedIndexes;  // values of the evaluated indexes
    ParserStateInfo(cstring name, const IR::P4Parser *parser, const IR::ParserState *state,
                    const ParserStateInfo *predecessor, ValueMap *before, size_t index)
        : name(name),
          parser(parser),
          state(state),
          predecessor(predecessor),
          before(before),
          after(nullptr),
          newState(nullptr),
          currentIndex(index) {
        CHECK_NULL(parser);
        CHECK_NULL(state);
        CHECK_NULL(before);
        if (predecessor) {
            statesIndexes = predecessor->statesIndexes;
            scenarioHS = predecessor->scenarioHS;
            substitutedIndexes = predecessor->substitutedIndexes;
        }
    }
};

/// Information produced for a parser by the symbolic evaluator
class ParserInfo {
    friend class RewriteAllParsers;
    // for each original state a vector of states produced by unrolling
    std::map<cstring, std::vector<ParserStateInfo *> *> states;

 public:
    std::vector<ParserStateInfo *> *get(cstring origState) {
        std::vector<ParserStateInfo *> *vec;
        auto it = states.find(origState);
        if (it == states.end()) {
            vec = new std::vector<ParserStateInfo *>;
            states.emplace(origState, vec);
        } else {
            vec = it->second;
        }
        return vec;
    }
    void add(ParserStateInfo *si) {
        cstring origState = si->state->name.name;
        auto vec = get(origState);
        vec->push_back(si);
    }
    std::map<cstring, std::vector<ParserStateInfo *> *> &getStates() { return states; }
};

typedef CallGraph<const IR::ParserState *> StateCallGraph;

/// Information about a parser in the input program
class ParserStructure {
    friend class ParserStateRewriter;
    friend class ParserSymbolicInterpreter;
    friend class AnalyzeParser;
    std::map<cstring, const IR::ParserState *> stateMap;

 public:
    const IR::P4Parser *parser;
    const IR::ParserState *start;
    const ParserInfo *result;
    StateCallGraph *callGraph;
    std::map<cstring, std::set<cstring>> statesWithHeaderStacks;
    std::map<cstring, size_t> callsIndexes;  // map for curent calls of state insite current one
    void setParser(const IR::P4Parser *parser) {
        CHECK_NULL(parser);
        callGraph = new StateCallGraph(parser->name);
        this->parser = parser;
        start = nullptr;
    }
    void addState(const IR::ParserState *state) { stateMap.emplace(state->name, state); }
    const IR::ParserState *get(cstring state) const { return ::get(stateMap, state); }
    void calls(const IR::ParserState *caller, const IR::ParserState *callee) {
        callGraph->calls(caller, callee);
    }

    bool analyze(ReferenceMap *refMap, TypeMap *typeMap, bool unroll, bool &wasError);
    /// check reachability for usage of header stack
    bool reachableHSUsage(IR::ID id, const ParserStateInfo *state) const;

 protected:
    /// evaluates rechable states with HS operations for each path.
    void evaluateReachability();
    /// add HS name which is used in a current state.
    void addStateHSUsage(const IR::ParserState *state, const IR::Expression *expression);
};

class AnalyzeParser : public Inspector {
    const ReferenceMap *refMap;
    ParserStructure *current;
    const IR::ParserState *currentState;

 public:
    AnalyzeParser(const ReferenceMap *refMap, ParserStructure *current)
        : refMap(refMap), current(current), currentState(nullptr) {
        CHECK_NULL(refMap);
        CHECK_NULL(current);
        setName("AnalyzeParser");
        visitDagOnce = false;
    }

    bool preorder(const IR::P4Parser *parser) override {
        LOG2("Scanning " << parser);
        current->setParser(parser);
        return true;
    }
    bool preorder(const IR::ParserState *state) override;
    void postorder(const IR::ParserState *state) override;
    void postorder(const IR::ArrayIndex *array) override;
    void postorder(const IR::Member *member) override;
    void postorder(const IR::PathExpression *expression) override;
};

// Applied to a P4Parser object.
class ParserRewriter : public PassManager {
    ParserStructure current;
    friend class RewriteAllParsers;

 public:
    bool hasOutOfboundState;
    bool wasError;
    ParserRewriter(ReferenceMap *refMap, TypeMap *typeMap, bool unroll) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        wasError = false;
        setName("ParserRewriter");
        addPasses({
            new AnalyzeParser(refMap, &current),
            [this, refMap, typeMap, unroll](void) {
                hasOutOfboundState = current.analyze(refMap, typeMap, unroll, wasError);
            },
        });
    }
};

///////////////////////////////////////////////////////
// The following are applied to the entire program

class RewriteAllParsers : public Transform {
    ReferenceMap *refMap;
    TypeMap *typeMap;
    bool unroll;

 public:
    RewriteAllParsers(ReferenceMap *refMap, TypeMap *typeMap, bool unroll)
        : refMap(refMap), typeMap(typeMap), unroll(unroll) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        setName("RewriteAllParsers");
    }

    // start generation of a code
    const IR::Node *postorder(IR::P4Parser *parser) override {
        // making rewriting
        auto rewriter = new ParserRewriter(refMap, typeMap, unroll);
        rewriter->setCalledBy(this);
        parser->apply(*rewriter);
        if (rewriter->wasError) {
            return parser;
        }
        /// make a new parser
        BUG_CHECK(rewriter->current.result,
                  "No result was found after unrolling of the parser loop");
        IR::P4Parser *newParser = parser->clone();
        IR::IndexedVector<IR::ParserState> states = newParser->states;
        newParser->states.clear();
        if (rewriter->hasOutOfboundState) {
            // generating state with verify(false, error.StackOutOfBounds)
            IR::Vector<IR::Argument> *arguments = new IR::Vector<IR::Argument>();
            arguments->push_back(
                new IR::Argument(new IR::BoolLiteral(IR::Type::Boolean::get(), false)));
            arguments->push_back(new IR::Argument(
                new IR::Member(new IR::TypeNameExpression(new IR::Type_Name(IR::ID("error"))),
                               IR::ID("StackOutOfBounds"))));
            IR::IndexedVector<IR::StatOrDecl> components;
            IR::IndexedVector<IR::Parameter> parameters;
            parameters.push_back(
                new IR::Parameter(IR::ID("check"), IR::Direction::In, IR::Type::Boolean::get()));
            parameters.push_back(new IR::Parameter(IR::ID("toSignal"), IR::Direction::In,
                                                   new IR::Type_Name(IR::ID("error"))));
            components.push_back(new IR::MethodCallStatement(new IR::MethodCallExpression(
                IR::Type::Void::get(),
                new IR::PathExpression(
                    new IR::Type_Method(IR::Type::Void::get(), new IR::ParameterList(parameters),
                                        "*method"),
                    new IR::Path(IR::ID("verify"))),
                arguments)));
            auto *outOfBoundsState = new IR::ParserState(
                IR::ID(outOfBoundsStateName), components,
                new IR::PathExpression(new IR::Type_State(),
                                       new IR::Path(IR::ParserState::reject, false)));
            newParser->states.push_back(outOfBoundsState);
        }
        for (auto &i : rewriter->current.result->states) {
            for (auto &j : *i.second)
                if (j->newState) {
                    if (rewriter->hasOutOfboundState &&
                        j->newState->name.name == "stateOutOfBound") {
                        continue;
                    }
                    newParser->states.push_back(j->newState);
                }
        }
        // adding accept/reject
        newParser->states.push_back(new IR::ParserState(IR::ParserState::accept, nullptr));
        newParser->states.push_back(new IR::ParserState(IR::ParserState::reject, nullptr));
        return newParser;
    }
};

class ParsersUnroll : public PassManager {
 public:
    ParsersUnroll(bool unroll, ReferenceMap *refMap, TypeMap *typeMap) {
        // remove block statements
        passes.push_back(new SimplifyControlFlow(refMap, typeMap));
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new RewriteAllParsers(refMap, typeMap, unroll));
        setName("ParsersUnroll");
    }
};

}  // namespace P4

#endif /* MIDEND_PARSERUNROLL_H_ */
