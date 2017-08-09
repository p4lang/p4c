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

#ifndef _BACKENDS_GRAPHS_CONTROLS_H_
#define _BACKENDS_GRAPHS_CONTROLS_H_

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <boost/optional.hpp>

#include <utility>  // std::pair
#include <vector>

#include "ir/ir.h"
#include "ir/visitor.h"

namespace graphs {

class EdgeTypeIface {
 public:
    virtual ~EdgeTypeIface() { }
    virtual cstring label() const = 0;
};

class NameStack {
 public:
    NameStack() = default;
    void push_back(const cstring &name) {
        stack.push_back(name);
    }
    void pop_back() {
        stack.pop_back();
    }
    cstring get(const cstring &name) const {
        std::stringstream sstream;
        for (auto &n : stack) sstream << n << ".";
        sstream << name;
        return cstring(sstream);
    }
 private:
    std::vector<cstring> stack;
};

class ControlGraphs : public Inspector {
 public:
    enum class VertexType {
        TABLE,
        CONDITION,
        SWITCH,
        STATEMENTS,
        CONTROL,
        OTHER
    };
    struct Vertex {
        cstring name;
        VertexType type;
    };
    class VertexWriter;
    using edgeProperty = boost::property<boost::edge_name_t, cstring>;
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS,
        boost::directedS, Vertex, edgeProperty>;
    using vertex_t = boost::graph_traits<Graph>::vertex_descriptor;

    using Parents = std::vector<std::pair<vertex_t, EdgeTypeIface *> >;

    ControlGraphs(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, const cstring &graphsDir);

    // merge misc control statements (action calls, extern method calls,
    // assignments) into a single vertex to reduce graph complexity
    boost::optional<vertex_t> merge_other_statements_into_vertex();

    vertex_t add_vertex(const cstring &name, VertexType type);

    bool preorder(const IR::PackageBlock *block) override;
    bool preorder(const IR::ControlBlock *block) override;
    bool preorder(const IR::P4Control *cont) override;
    bool preorder(const IR::BlockStatement *statement) override;
    bool preorder(const IR::IfStatement *statement) override;
    bool preorder(const IR::SwitchStatement *statement) override;
    bool preorder(const IR::MethodCallStatement *statement) override;
    bool preorder(const IR::AssignmentStatement *statement) override;
    bool preorder(const IR::ReturnStatement *) override;
    bool preorder(const IR::ExitStatement *) override;
    bool preorder(const IR::P4Table *table) override;

    void writeGraphToFile(const Graph &g, const cstring &name);

 private:
    P4::ReferenceMap *refMap; P4::TypeMap *typeMap;
    const cstring graphsDir;
    Graph *g{nullptr};
    vertex_t root{};
    vertex_t exit_v{};
    Parents parents{};
    Parents return_parents{};
    std::vector<const IR::Statement *> statements_stack{};
    NameStack name_stack{};
    boost::optional<cstring> instance_name{};
};

}  // namespace graphs

#endif  // _BACKENDS_GRAPHS_CONTROLS_H_
