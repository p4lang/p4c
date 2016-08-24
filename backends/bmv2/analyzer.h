/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _BACKENDS_BMV2_ANALYZER_H_
#define _BACKENDS_BMV2_ANALYZER_H_

#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace BMV2 {

cstring nameFromAnnotation(const IR::Annotations* annotations, cstring defaultValue);

// This CFG is only good for BMV2, which only cares about some Nodes in the program
class CFG : public IHasDbPrint {
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
    // All local variables
    std::vector<const IR::Declaration_Variable*> variables;

    ProgramParts() {}
    void analyze(IR::ToplevelBlock* toplevel);
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_ANALYZER_H_ */
