#ifndef _BACKENDS_BMV2_ANALYZER_H_
#define _BACKENDS_BMV2_ANALYZER_H_

#include "ir/ir.h"
#include "frontends/common/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/evaluator/blockMap.h"

namespace BMV2 {

cstring nameFromAnnotation(const IR::Annotations* annotations, cstring defaultValue);

// This CFG is only good for BMV2, which only cares about some Nodes in the program
class CFG {
 public:
    class Edge;

    class EdgeSet {
     public:
        std::set<CFG::Edge*> edges;

        EdgeSet() = default;
        explicit EdgeSet(CFG::Edge* edge) { edges.emplace(edge); }
        explicit EdgeSet(const EdgeSet* other) { mergeWith(other); }

        void mergeWith(const EdgeSet* other)
        { edges.insert(other->edges.begin(), other->edges.end()); }
        void dbprint(std::ostream& out) const;
        void emplace(CFG::Edge* edge) { edges.emplace(edge); }
        size_t size() const { return edges.size(); }
    };

    class Node {
     protected:
        friend class CFG;

        static unsigned crtId;
        EdgeSet         predecessors;
        explicit Node(cstring name) : id(crtId++), name(name) {}
        Node() : id(crtId++), name("node_" + Util::toString(id)) {}
        virtual ~Node() {}

     public:
        const unsigned id;
        const cstring  name;
        EdgeSet        successors;

        void dbprint(std::ostream& out) const;
        void addPredecessors(const EdgeSet* set) { predecessors.mergeWith(set); }
        template<typename T> bool is() const { return to<T>() != nullptr; }
        template<typename T> T* to() { return dynamic_cast<T*>(this); }
        template<typename T> const T* to() const { return dynamic_cast<const T*>(this); }
        void computeSuccessors();
    };

 public:
    class TableNode : public Node {
     public:
        const IR::P4Table* table;
        const IR::Expression*      invocation;
        explicit TableNode(const IR::P4Table* table, const IR::Expression* invocation) :
                Node(nameFromAnnotation(table->annotations, table->name)),
                table(table), invocation(invocation) {}
    };

    class IfNode : public Node {
     public:
        const IR::IfStatement* statement;
        explicit IfNode(const IR::IfStatement* statement) : statement(statement) {}
    };

    class DummyNode : public Node {
     public:
        explicit DummyNode(cstring name) : Node(name) {}
    };

 protected:
    enum class EdgeType {
        Unconditional,
        True,
        False,
        Label
    };

 public:
    class Edge {
     protected:
        EdgeType type;
        Edge(Node* node, EdgeType type, cstring label) : type(type), endpoint(node), label(label) {}

     public:
        Node*    endpoint;
        cstring  label;  // only present if type == Label

        explicit Edge(Node* node) : type(EdgeType::Unconditional), endpoint(node)
        { CHECK_NULL(node); }
        Edge(Node* node, bool b) :
                type(b ? EdgeType::True : EdgeType::False), endpoint(node)
        { CHECK_NULL(node); }
        Edge(Node* node, cstring label) :
                type(EdgeType::Label), endpoint(node), label(label)
        { CHECK_NULL(node); }
        void dbprint(std::ostream& out) const;
        Edge* clone(Node* node) const
        { return new Edge(node, type, label); }
        Node* getNode() { return endpoint; }
        bool  getBool() {
            BUG_CHECK(isBool(), "Edge is not Boolean");
            return type == EdgeType::True;
        }
        bool isBool() const { return type == EdgeType::True || type == EdgeType::False; }
        bool isUnconditional() const { return type == EdgeType::Unconditional; }
    };

 public:
    Node* entryPoint;
    Node* exitPoint;
    const IR::P4Control* container;
    std::set<Node*> allNodes;

    CFG() : entryPoint(nullptr), exitPoint(nullptr), container(nullptr) {}
    Node* makeNode(const IR::P4Table* table, const IR::Expression* invocation) {
        auto result = new TableNode(table, invocation);
        allNodes.emplace(result);
        return result;
    }
    Node* makeNode(const IR::IfStatement* statement) {
        auto result = new IfNode(statement);
        allNodes.emplace(result);
        return result;
    }
    Node* makeNode(cstring name) {
        auto result = new DummyNode(name);
        allNodes.emplace(result);
        return result;
    }
    void build(const IR::P4Control* cc,
               const P4::ReferenceMap* refMap, const P4::TypeMap* typeMap);
    void setEntry(Node* entry) {
        BUG_CHECK(entryPoint == nullptr, "Entry already set");
        entryPoint = entry;
    }
    void dbprint(std::ostream& out, Node* node, std::set<Node*> &done) const;  // helper
    void dbprint(std::ostream& out) const;
    void computeSuccessors()
    { for (auto n : allNodes) n->computeSuccessors(); }
};

// Represents global information about a P4 v1.2 program
class ProgramParts {
 public:
    // map action to parent
    std::map<const IR::P4Action*, const IR::P4Control*> actions;
    // Maps each Parameter of an action to its positional index.
    // Needed to generate code for actions.
    std::map<const IR::Parameter*, unsigned> index;
    // Parameters of controls/parsers
    std::set<const IR::Parameter*> nonActionParameters;
    // for each action its json id
    std::map<const IR::P4Action*, unsigned> ids;

    ProgramParts() {}
    void analyze(P4::BlockMap* blockMap);
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_ANALYZER_H_ */
