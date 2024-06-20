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

#ifndef BACKENDS_GRAPHS_GRAPHS_H_
#define BACKENDS_GRAPHS_GRAPHS_H_

#include "config.h"
#include "lib/cstring.h"

/// Shouldn't happen as cmake will not try to build this backend if the boost
/// graph headers couldn't be found.
#ifndef HAVE_LIBBOOST_GRAPH
#error "This backend requires the boost graph headers, which could not be found"
#endif

#include <map>
#include <optional>
#include <utility>  // std::pair
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphviz.hpp>

#include "frontends/p4/parserCallGraph.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

class ReferenceMap;
class TypeMap;

}  // namespace P4

namespace graphs {

using namespace P4::literals;

class EdgeTypeIface {
 public:
    virtual ~EdgeTypeIface() {}
    virtual cstring label() const = 0;
};

class EdgeUnconditional : public EdgeTypeIface {
 public:
    EdgeUnconditional() = default;
    cstring label() const override { return cstring::empty; };
};

class EdgeIf : public EdgeTypeIface {
 public:
    enum class Branch { TRUE, FALSE };
    explicit EdgeIf(Branch branch) : branch(branch) {}
    cstring label() const override {
        switch (branch) {
            case Branch::TRUE:
                return "TRUE"_cs;
            case Branch::FALSE:
                return "FALSE"_cs;
        }
        BUG("unreachable");
        return cstring::empty;
    };

 private:
    Branch branch;
};

class EdgeSwitch : public EdgeTypeIface {
 public:
    explicit EdgeSwitch(const IR::Expression *labelExpr) : labelExpr(labelExpr) {}
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
        KEY,
        ACTION,
        CONDITION,
        SWITCH,
        STATEMENTS,
        CONTROL,
        OTHER,
        STATE,
        EMPTY
    };
    struct Vertex {
        cstring name;
        VertexType type;
    };

    /// The boost graph support for graphviz subgraphs is not very intuitive. In
    /// particular the write_graphviz code assumes the existence of a lot of
    /// properties. See
    /// https://stackoverflow.com/questions/29312444/how-to-write-graphviz-subgraphs-with-boostwrite-graphviz
    /// for more information.
    using GraphvizAttributes = std::map<cstring, cstring>;
    using vertexProperties = boost::property<boost::vertex_attribute_t, GraphvizAttributes, Vertex>;
    using edgeProperties = boost::property<
        boost::edge_attribute_t, GraphvizAttributes,
        boost::property<boost::edge_name_t, cstring, boost::property<boost::edge_index_t, int>>>;
    using graphProperties = boost::property<
        boost::graph_name_t, std::string,
        boost::property<
            boost::graph_graph_attribute_t, GraphvizAttributes,
            boost::property<boost::graph_vertex_attribute_t, GraphvizAttributes,
                            boost::property<boost::graph_edge_attribute_t, GraphvizAttributes>>>>;
    using Graph_ = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                                         vertexProperties, edgeProperties, graphProperties>;
    using Graph = boost::subgraph<Graph_>;
    using vertex_t = boost::graph_traits<Graph>::vertex_descriptor;

    using Parents = std::vector<std::pair<vertex_t, EdgeTypeIface *>>;

    /// merge misc control statements (action calls, extern method calls,
    /// assignments) into a single vertex to reduce graph complexity
    std::optional<vertex_t> merge_other_statements_into_vertex();

    vertex_t add_vertex(const cstring &name, VertexType type);
    vertex_t add_and_connect_vertex(const cstring &name, VertexType type);
    void add_edge(const vertex_t &from, const vertex_t &to, const cstring &name);
    /// Used to connect subgraphs
    ///
    /// @param from Node from which edge will start
    /// @param to Node where edge will end
    /// @param name Used as edge label
    /// @param cluster_id ID of cluster, that will be connected to the previous cluster.
    void add_edge(const vertex_t &from, const vertex_t &to, const cstring &name,
                  unsigned cluster_id);

    class GraphAttributeSetter {
     public:
        void operator()(Graph &g) const {
            auto vertices = boost::vertices(g);
            for (auto &vit = vertices.first; vit != vertices.second; ++vit) {
                const auto &vinfo = g[*vit];
                auto attrs = boost::get(boost::vertex_attribute, g);
                attrs[*vit]["label"_cs] = vinfo.name;
                attrs[*vit]["style"_cs] = vertexTypeGetStyle(vinfo.type);
                attrs[*vit]["shape"_cs] = vertexTypeGetShape(vinfo.type);
                attrs[*vit]["margin"_cs] = vertexTypeGetMargin(vinfo.type);
            }
            auto edges = boost::edges(g);
            for (auto &eit = edges.first; eit != edges.second; ++eit) {
                auto attrs = boost::get(boost::edge_attribute, g);
                attrs[*eit]["label"_cs] = boost::get(boost::edge_name, g, *eit);
            }
        }

     private:
        static cstring vertexTypeGetShape(VertexType type) {
            switch (type) {
                case VertexType::TABLE:
                case VertexType::ACTION:
                    return "ellipse"_cs;
                default:
                    return "rectangle"_cs;
            }
            BUG("unreachable");
            return cstring::empty;
        }

        static cstring vertexTypeGetStyle(VertexType type) {
            switch (type) {
                case VertexType::CONTROL:
                    return "dashed"_cs;
                case VertexType::EMPTY:
                    return "invis"_cs;
                case VertexType::KEY:
                case VertexType::CONDITION:
                case VertexType::SWITCH:
                    return "rounded"_cs;
                default:
                    return "solid"_cs;
            }
            BUG("unreachable");
            return cstring::empty;
        }

        static cstring vertexTypeGetMargin(VertexType type) {
            switch (type) {
                default:
                    return cstring::empty;
            }
        }
    };  // end class GraphAttributeSetter

 protected:
    Graph *g{nullptr};
    vertex_t start_v{};
    vertex_t exit_v{};
    Parents parents{};
    std::vector<const IR::Statement *> statementsStack{};

 private:
    /// Limits string size in helper_sstream and resets it
    ///
    /// @param[out] sstream Stringstream where trimmed string is stored
    /// @param helper_sstream Contains string, which will be trimmed
    void limitStringSize(std::stringstream &sstream, std::stringstream &helper_sstream);
};

}  // namespace graphs

#endif /* BACKENDS_GRAPHS_GRAPHS_H_ */
