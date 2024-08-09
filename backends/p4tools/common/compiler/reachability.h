#ifndef COMMON_COMPILER_REACHABILITY_H_
#define COMMON_COMPILER_REACHABILITY_H_

#include <list>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/callGraph.h"
#include "ir/id.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"
#include "lib/cstring.h"
#include "lib/null.h"

namespace P4::P4Tools {

using DCGVertexType = const IR::Node *;
using DCGVertexTypeSet = std::unordered_set<DCGVertexType>;
using ReachabilityHashType = std::unordered_map<cstring, DCGVertexTypeSet>;

template <class T>
class ExtendedCallGraph : public P4::CallGraph<T> {
    friend class P4ProgramDCGCreator;
    ReachabilityHashType hash;

 public:
    explicit ExtendedCallGraph(std::string_view name) : P4::CallGraph<T>(name) {}
    const ReachabilityHashType &getHash() const { return hash; }
    /// Function adds current vertex to a hash which allows to get access
    /// for vertexes from string in DCG.
    /// If name is empty then it doesn't hash it.
    void addToHash(T vertex, const IR::ID &name) {
        if (name.name.size() == 0) {
            return;
        }
        auto i = hash.find(name.name);
        if (i == hash.end()) {
            std::unordered_set<DCGVertexType> s;
            s.insert(vertex);
            hash.emplace(name.name, s);
        } else {
            i->second.insert(vertex);
        }
        const auto *prev = name.name.findlast('.');
        if (prev != nullptr) {
            cstring newName = name.name.before(prev);
            i = hash.find(newName);
            if (i == hash.end()) {
                addToHash(vertex, (!newName.isNullOrEmpty() ? IR::ID(newName) : IR::ID()));
            }
        }
    }

    bool isReachable(T start, T element) const {
        CHECK_NULL(start);
        CHECK_NULL(element);
        std::set<T> work;
        std::set<T> visited;
        work.emplace(start);
        while (!work.empty()) {
            auto *node = *work.begin();
            if (node->equiv(*element)) {
                return true;
            }
            work.erase(node);
            if (visited.find(node) != visited.end()) {
                continue;
            }
            visited.emplace(node);
            auto edges = this->out_edges.find(node);
            if (edges == this->out_edges.end()) {
                continue;
            }
            for (auto c : *(edges->second)) {
                work.emplace(c);
            }
        }
        return false;
    }
};

using NodesCallGraph = ExtendedCallGraph<DCGVertexType>;

/// The main class for building control flow DCG.
class P4ProgramDCGCreator : public Inspector, private P4::ResolutionContext {
    NodesCallGraph *dcg;
    DCGVertexTypeSet prev;
    std::unordered_set<DCGVertexType> visited;
    const IR::P4Program *p4program;

 public:
    explicit P4ProgramDCGCreator(NodesCallGraph *dcg);

    bool preorder(const IR::Annotation *annotation) override;
    bool preorder(const IR::ConstructorCallExpression *callExpr) override;
    bool preorder(const IR::MethodCallExpression *call) override;
    bool preorder(const IR::MethodCallStatement *method) override;
    bool preorder(const IR::P4Action *action) override;
    bool preorder(const IR::P4Parser *parser) override;
    bool preorder(const IR::P4Table *table) override;
    bool preorder(const IR::ParserState *parserState) override;
    bool preorder(const IR::P4Control *control) override;
    bool preorder(const IR::P4Program *program) override;
    bool preorder(const IR::P4ValueSet *valueSet) override;
    bool preorder(const IR::SelectExpression *selectExpression) override;
    bool preorder(const IR::IfStatement *ifStatement) override;
    bool preorder(const IR::SwitchStatement *switchStatement) override;
    bool preorder(const IR::StatOrDecl *statOrDecl) override;

 protected:
    /// Function add edge to current @vertex in DCG.
    /// The edge is a pair (@prev, @vertex).
    void addEdge(DCGVertexType vertex, const IR::ID &vertexName = IR::ID());
};

/// The main data for reachability engine.
class ReachabilityEngineState {
    DCGVertexType prevNode = nullptr;
    std::list<DCGVertexType> state;

 public:
    /// Gets initial state.
    static ReachabilityEngineState *getInitial();
    /// Copies a state.
    ReachabilityEngineState *copy();
    /// Gets current state.
    std::list<DCGVertexType> getState();
    /// Sets current state.
    void setState(const std::list<DCGVertexType> &);
    /// Gets previous node.
    DCGVertexType getPrevNode();
    /// Sets previous node.
    void setPrevNode(DCGVertexType);
    /// Retuns true if state is empty.
    bool isEmpty();
    /// Clears state.
    void clear();
};

using ReachabilityResult = std::pair<bool, const IR::Expression *>;

/// The main class to support user input patterns.
/// This class allows to move step-by-step via user's input expression.
/// Syntax for the user's input expressions
/// <behavior> ::= <behavior> ; <behavior> |
///                <behavior> + <behavior> |
///                <name> | <forbidden name>
/// <name> ::= <p4c node name> ['(' <condition> ')']
/// <forbidden name> ::= ! <p4c node name>
/// <p4c node name> - the name of the MethodCallStatement, P4Action, P4Parser,
///                   P4Table, ParserState, P4Control, ParserValueSet, Declaration.
/// <p4c condition> - any conditions p4c in syntax, whcih should be returned by the
//                    Engine if corresponded <p4c node name> was reached.
class ReachabilityEngine {
    const NodesCallGraph &dcg;
    const ReachabilityHashType &hash;
    std::unordered_map<DCGVertexType, std::list<DCGVertexType>> userTransitions;
    std::unordered_map<DCGVertexType, const IR::Expression *> conditions;
    std::unordered_set<DCGVertexType> forbiddenVertexes;

 public:
    /// Default constructor, where dcg is a control flow graph builded by P4ProgramDCGCreator,
    /// reachabilityExpression is a user's pattern wrote in the syntax presented above,
    /// eliminateAnnotations is true if after detection of the annotations it should to store
    /// corresponding parent IR::Node in a  reachability engine state.
    ReachabilityEngine(const NodesCallGraph &dcg, std::string reachabilityExpression,
                       bool eliminateAnnotations = false);
    /// Moves the next statement in a engine state. It returns a pair where the first argument
    /// is a flag for the possibility of such a movement and the second argument is an condition
    /// which should be checked additionally. If engine state is reachable from current node
    /// then it returns true. If engine state contain such node then it returns additional condition
    /// in case if it was inputed by user and moves engine state to the next state.
    ReachabilityResult next(ReachabilityEngineState *, DCGVertexType);
    /// Returns current control flow graph.
    const NodesCallGraph &getDCG();

 protected:
    /// Translates current annotation into set of the parent nodes.
    void annotationToStatements(DCGVertexType node, std::unordered_set<DCGVertexType> &s);
    /// Adds transition to engine control flow graph.
    void addTransition(DCGVertexType, const std::unordered_set<DCGVertexType> &);
    /// Translates string into the corresponding nodes.
    std::unordered_set<DCGVertexType> getName(std::string name);
    /// Gets a user's condition from a node.
    const IR::Expression *getCondition(DCGVertexType);
    /// Adds user's condition the a node.
    const IR::Expression *addCondition(const IR::Expression *prev, DCGVertexType currentState);
    /// Translates a string representation into an IR::Expression.
    /// Not implemented yet.
    static const IR::Expression *stringToNode(std::string name);

 protected:
    /// Adds an edge to the current @vertex in DCG.
    /// The edge is a pair (@prev, @vertex).
    void addEdge(DCGVertexType vertex, IR::ID vertexName = IR::ID());
};

}  // namespace P4::P4Tools

#endif /* COMMON_COMPILER_REACHABILITY_H_ */
