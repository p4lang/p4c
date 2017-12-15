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

#ifndef _BACKENDS_GRAPHS_GRAPHS_H_
#define _BACKENDS_GRAPHS_GRAPHS_H_

#include "config.h"

// Shouldn't happen as cmake will not try to build this backend if the boost
// graph headers couldn't be found.
#ifndef HAVE_LIBBOOST_GRAPH
#error "This backend requires the boost graph headers, which could not be found"
#endif

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>

#include <boost/optional.hpp>

#include <map>
#include <utility>  // std::pair
#include <vector>

#include "ir/ir.h"
#include "ir/visitor.h"
#include "frontends/p4/parserCallGraph.h"

namespace P4 {

class ReferenceMap;
class TypeMap;

}  // namespace P4

namespace graphs {

class EdgeTypeIface {
 public:
    virtual ~EdgeTypeIface() { }
    virtual cstring label() const = 0;
};

class EdgeUnconditional : public EdgeTypeIface {
 public:
    EdgeUnconditional() = default;
    cstring label() const override { return ""; };
};

class EdgeIf : public EdgeTypeIface {
 public:
    enum class Branch { TRUE, FALSE };
    explicit EdgeIf(Branch branch) : branch(branch) { }
    cstring label() const override {
        switch (branch) {
          case Branch::TRUE:
              return "TRUE";
          case Branch::FALSE:
              return "FALSE";
        }
        BUG("unreachable");
        return "";
    };

 private:
    Branch branch;
};

class EdgeSwitch : public EdgeTypeIface {
 public:
    explicit EdgeSwitch(const IR::Expression *labelExpr)
        : labelExpr(labelExpr) { }
    cstring label() const override {
        std::stringstream sstream;
        labelExpr->dbprint(sstream);
        return cstring(sstream);
    };

 private:
    const IR::Expression *labelExpr;
};

class Graphs : public Inspector {
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

    // The boost graph support for graphviz subgraphs is not very intuitive. In
    // particular the write_graphviz code assumes the existence of a lot of
    // properties. See
    // https://stackoverflow.com/questions/29312444/how-to-write-graphviz-subgraphs-with-boostwrite-graphviz
    // for more information.
    using GraphvizAttributes = std::map<cstring, cstring>;
    using vertexProperties =
        boost::property<boost::vertex_attribute_t, GraphvizAttributes,
        Vertex>;
    using edgeProperties =
        boost::property<boost::edge_attribute_t, GraphvizAttributes,
        boost::property<boost::edge_name_t, cstring,
        boost::property<boost::edge_index_t, int> > >;
    using graphProperties =
        boost::property<boost::graph_name_t, cstring,
        boost::property<boost::graph_graph_attribute_t, GraphvizAttributes,
        boost::property<boost::graph_vertex_attribute_t, GraphvizAttributes,
        boost::property<boost::graph_edge_attribute_t, GraphvizAttributes> > > >;
    using Graph_ = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                                         vertexProperties, edgeProperties,
                                         graphProperties>;
    using Graph = boost::subgraph<Graph_>;
    using vertex_t = boost::graph_traits<Graph>::vertex_descriptor;

    using Parents = std::vector<std::pair<vertex_t, EdgeTypeIface *> >;

    // merge misc control statements (action calls, extern method calls,
    // assignments) into a single vertex to reduce graph complexity
    boost::optional<vertex_t> merge_other_statements_into_vertex();

    vertex_t add_vertex(const cstring &name, VertexType type);
    vertex_t add_and_connect_vertex(const cstring &name, VertexType type);
    void add_edge(const vertex_t &from, const vertex_t &to, const cstring &name);

    class GraphAttributeSetter {
     public:
        void operator()(Graph &g) const {
            auto vertices = boost::vertices(g);
            for (auto vit = vertices.first; vit != vertices.second; ++vit) {
                const auto &vinfo = g[*vit];
                auto attrs = boost::get(boost::vertex_attribute, g);
                attrs[*vit]["label"] = vinfo.name;
                attrs[*vit]["style"] = vertexTypeGetStyle(vinfo.type);
                attrs[*vit]["shape"] = vertexTypeGetShape(vinfo.type);
                attrs[*vit]["margin"] = vertexTypeGetMargin(vinfo.type);
            }
            auto edges = boost::edges(g);
            for (auto eit = edges.first; eit != edges.second; ++eit) {
                auto attrs = boost::get(boost::edge_attribute, g);
                attrs[*eit]["label"] = boost::get(boost::edge_name, g, *eit);
            }
        }

     private:
        static cstring vertexTypeGetShape(VertexType type) {
            switch (type) {
            case VertexType::TABLE:
                return "ellipse";
            default:
                return "rectangle";
            }
            BUG("unreachable");
            return "";
        }

        static cstring vertexTypeGetStyle(VertexType type) {
            switch (type) {
            case VertexType::CONTROL:
                return "dashed";
            default:
                return "solid";
            }
            BUG("unreachable");
            return "";
        }

        static cstring vertexTypeGetMargin(VertexType type) {
            switch (type) {
            default:
                return "";
            }
        }
    };  // end class GraphAttributeSetter

 protected:
    Graph *g{nullptr};
    vertex_t start_v{};
    vertex_t exit_v{};
    Parents parents{};
    std::vector<const IR::Statement *> statementsStack{};
};

}  // namespace graphs

#endif  // _BACKENDS_GRAPHS_GRAPHS_H_
