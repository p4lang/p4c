/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_P4C_MAU_TABLE_FLOW_GRAPH_H_
#define BF_P4C_MAU_TABLE_FLOW_GRAPH_H_

#include <map>
#include <optional>
#include <set>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/transitive_closure.hpp>

#include "bf-p4c/ir/control_flow_visitor.h"
#include "bf-p4c/lib/boost_graph.h"
#include "bf-p4c/mau/mau_visitor.h"

namespace boost {
enum vertex_table_t { vertex_table };
BOOST_INSTALL_PROPERTY(vertex, table);

enum edge_annotation_t { edge_annotation };
BOOST_INSTALL_PROPERTY(edge, annotation);
}  // namespace boost

using namespace P4;

/// Represents a control-flow graph between the tables in a program, reflecting the logical control
/// flow through the program.
struct FlowGraph {
    typedef boost::adjacency_list<boost::vecS, boost::vecS,
                                  boost::bidirectionalS,  // Directed edges.
                                  // Label vertices with tables.
                                  boost::property<boost::vertex_table_t, const IR::MAU::Table *>,
                                  // Label edges with control annotations.
                                  boost::property<boost::edge_annotation_t, cstring>>
        Graph;

    /// Custom BFS visitor for finding a shortest path between two nodes.
    class BFSPathFinder : public boost::default_bfs_visitor {
        /// Maps each visited node to its parent when the node is first discovered as the
        /// destination of an edge.
        std::map<typename Graph::vertex_descriptor, typename Graph::vertex_descriptor> *parent =
            nullptr;

        /// Used as an exception to finish the BFS early when we find our target.
        struct done {};

        /// The graph being searched.
        const Graph &graph;

        /// The node that we are looking for.
        typename Graph::vertex_descriptor dst;

     public:
        void examine_edge(typename Graph::edge_descriptor e, const Graph &g) {
            auto src = boost::source(e, g);
            auto dst = boost::target(e, g);

            if (!parent->count(dst)) (*parent)[dst] = src;
            if (dst == this->dst) throw done{};
        }

        explicit BFSPathFinder(const Graph &g) : graph(g) {}

        std::vector<typename Graph::vertex_descriptor> find_path(
            typename Graph::vertex_descriptor src, typename Graph::vertex_descriptor dst) {
            std::map<typename Graph::vertex_descriptor, typename Graph::vertex_descriptor> parent;
            this->parent = &parent;

            this->dst = dst;
            try {
                boost::breadth_first_search(graph, src, boost::visitor(*this));
            } catch (done const &) {
            }

            std::vector<typename Graph::vertex_descriptor> result;
            if (parent.count(dst)) {
                // Search succeeded. Build the path in reverse.
                auto cur = dst;
                result.emplace_back(cur);
                do {
                    cur = parent.at(cur);
                    result.emplace_back(cur);
                } while (cur != src);

                // Reverse the result so we get a forward path.
                for (unsigned i = 0; i < result.size() / 2; ++i) {
                    std::swap(result[i], result[result.size() - i - 1]);
                }
            }

            // Clean up for the next call.
            this->parent = nullptr;

            return result;
        }
    };

    /// The underlying Boost graph backing this FlowGraph.
    Graph g;

    /// The source node, representing the entry point (i.e., entry from the parser).
    typename Graph::vertex_descriptor v_source;

    /// The sink node, representing the exit point (i.e., entry to the deparser).
    typename Graph::vertex_descriptor v_sink;

    std::optional<gress_t> gress;

    mutable Reachability<Graph> reachability;

    /// Maps each table to its corresponding vertex ID in the Boost graph.
    ordered_map<const IR::MAU::Table *, int> tableToVertexIndex;

    /// All tables in this graph, excluding the nullptr table, used to represent the sink node.
    std::set<const IR::MAU::Table *> tables;

    /// The dominator set for each table in the graph. Lazily computed.
    mutable std::optional<std::map<const IR::MAU::Table *, std::set<const IR::MAU::Table *>>>
        dominators;

    // By default, emptyFlowGraph is set to true to indicate that there are no vertices in the
    // graph. Only when the first actual table is added to the flow graph is this member set to
    // false.
    bool emptyFlowGraph = true;

    /// @returns the control-flow annotation for the given edge.
    const cstring get_ctrl_dependency_info(typename Graph::edge_descriptor edge) const {
        return boost::get(boost::edge_annotation, g)[edge];
    }

    FlowGraph(void) : reachability(g), path_finder(g) {
        gress = std::nullopt;
        dominators = std::nullopt;
    }

    FlowGraph(FlowGraph &&other)
        : g(std::move(other.g)),
          gress(std::move(other.gress)),
          reachability(g),
          tableToVertexIndex(std::move(other.tableToVertexIndex)),
          tables(std::move(other.tables)),
          dominators(std::move(other.dominators)),
          emptyFlowGraph(std::move(other.emptyFlowGraph)),
          tableToVertex(std::move(other.tableToVertex)),
          path_finder(g) {}

    FlowGraph(const FlowGraph &other)
        : g(other.g),
          gress(other.gress),
          reachability(g),
          tableToVertexIndex(other.tableToVertexIndex),
          tables(other.tables),
          dominators(other.dominators),
          emptyFlowGraph(other.emptyFlowGraph),
          tableToVertex(other.tableToVertex),
          path_finder(g) {}

    /// Maps each table to its associated graph vertex.
    std::map<const IR::MAU::Table *, typename Graph::vertex_descriptor> tableToVertex;

    /// Reverse map of above
    std::map<typename Graph::vertex_descriptor, const IR::MAU::Table *> vertexToTable;

    /// Determines whether this graph is empty.
    bool is_empty() const {
        auto all_vertices = boost::vertices(g);
        if (++all_vertices.first == all_vertices.second) {
            return true;
        }
        return false;
    }

    /// Clears the state in this FlowGraph.
    void clear() {
        g.clear();
        gress = std::nullopt;
        reachability.clear();
        tableToVertexIndex.clear();
        tableToVertex.clear();
        vertexToTable.clear();
        tables.clear();
        emptyFlowGraph = true;
    }

    void add_sink_vertex() {
        v_sink = add_vertex(nullptr);
        reachability.setSink(v_sink);
    }

    /// @returns true iff there is a path in the flow graph from @p t1 to @p t2.
    /// Passing nullptr for @p t1 or @p t2 designates the sink node (consider it the deparser).
    /// Tables are not considered reachable from themselves unless they are part of a cycle
    /// in the graph.
    bool can_reach(const IR::MAU::Table *t1, const IR::MAU::Table *t2) const {
        if (t2 == nullptr) return true;
        if (t1 == nullptr) return false;
        const auto v1 = get_vertex(t1);
        const auto v2 = get_vertex(t2);
        return reachability.canReach(v1, v2);
    }

    /// @returns the dominator set of the given table. If the IR is well-formed (i.e., the flow
    /// graph is a DAG), then passing nullptr for @p table will produce the set of tables that are
    /// always executed.
    const std::set<const IR::MAU::Table *> get_dominators(const IR::MAU::Table *table) const;

    /// Determines whether the given table is executed on all paths.
    bool is_always_reached(const IR::MAU::Table *) const;

    /// Helper for find_path.
    BFSPathFinder path_finder;

    /// @returns a path from one table to another. If no path is found, an empty path is returned.
    /// If non-empty, the returned path will always contain at least two elements: the src and the
    /// dst.
    std::vector<const IR::MAU::Table *> find_path(const IR::MAU::Table *src,
                                                  const IR::MAU::Table *dst) {
        auto path = path_finder.find_path(get_vertex(src), get_vertex(dst));
        std::vector<const IR::MAU::Table *> result;
        for (auto node : path) {
            result.emplace_back(get_vertex(node));
        }

        return result;
    }

    /// @returns the table pointer corresponding to a vertex in the flow graph
    const IR::MAU::Table *get_vertex(typename Graph::vertex_descriptor v) const {
        return boost::get(boost::vertex_table, g)[v];
    }

    /// @returns the vertex in the flow graph corresponding to a Table object.
    typename Graph::vertex_descriptor get_vertex(const IR::MAU::Table *tbl) const {
        BUG_CHECK(tableToVertexIndex.count(tbl), "Table object not found for %1%",
                  tbl->externalName());
        return tableToVertexIndex.at(tbl);
    }

    /// @returns all tables in this graph.
    const std::set<const IR::MAU::Table *> get_tables() const { return tables; }

    /// @return the vertex associated with the given table, creating the vertex if one does not
    /// already exist.
    typename Graph::vertex_descriptor add_vertex(const IR::MAU::Table *table) {
        // Initialize gress if needed.
        if (table != nullptr && gress == std::nullopt) gress = table->gress;

        if (tableToVertex.count(table)) {
            return tableToVertex.at(table);
        }

        // Create new vertex.
        auto v = boost::add_vertex(table, g);
        tableToVertex[table] = v;
        vertexToTable[v] = table;
        if (table) tables.insert(table);

        // If the vertex being added corresponds to a real table (not the sink), then the flow
        // graph is no longer empty; set the emptyFlowGraph member accordingly.
        if (table != nullptr) emptyFlowGraph = false;

        return v;
    }

    /// Return an edge descriptor from the given src to the given dst, creating the edge if one
    /// doesn't already exist. The returned bool is true when this is a newly-created edge.
    std::pair<typename Graph::edge_descriptor, bool> add_edge(const IR::MAU::Table *src,
                                                              const IR::MAU::Table *dst,
                                                              const cstring edge_label) {
        typename Graph::vertex_descriptor src_v, dst_v;
        src_v = add_vertex(src);
        dst_v = add_vertex(dst);

        // Look for a pre-existing edge.
        typename Graph::out_edge_iterator out, end;
        for (boost::tie(out, end) = boost::out_edges(src_v, g); out != end; ++out) {
            if (boost::target(*out, g) == dst_v) return {*out, false};
        }

        // No pre-existing edge, so make one.
        auto maybe_new_e = boost::add_edge(src_v, dst_v, g);
        if (!maybe_new_e.second)
            // A vector-based adjacency_list (i.e. Graph) is a multigraph.
            // Inserting edges should always create new edges.
            BUG("Boost Graph Library failed to add edge.");
        boost::get(boost::edge_annotation, g)[maybe_new_e.first] = edge_label;
        return {maybe_new_e.first, true};
    }

    std::vector<const IR::MAU::Table *> topological_sort() const {
        std::vector<Graph::vertex_descriptor> internal_result;
        boost::topological_sort(g, std::back_inserter(internal_result));

        // Boost produces a reverse order with internal vertices. Reverse the list while converting
        // to tables.
        std::vector<const IR::MAU::Table *> result;
        for (auto it = internal_result.rbegin(); it != internal_result.rend(); ++it) {
            result.push_back(get_vertex(*it));
        }

        return result;
    }

    friend std::ostream &operator<<(std::ostream &, const FlowGraph &);

    struct DumpTableDetails {
        virtual void dump(std::ostream &, const IR::MAU::Table *) const {}
    };

    static std::string viz_node_name(const IR::MAU::Table *tbl);
    void dump_viz(std::ostream &out, const DumpTableDetails *details = nullptr);
};

/// Computes a table control-flow graph for the IR.
//
// FIXME(Jed): This currently only works when gateway conditions are represented as separate table
// objects. After table placement, gateways and match tables are fused into single table objects.
// This pass should be fixed at some point to support this fused representation.
//
// Here are some thoughts on how to this. We can leverage the call structure in
// IR::MAU::Table::visit_children; specifically, the calls to flow_clone, visit(Node, label),
// flow_merge_global_to, and flow_merge. This should give us enough information to track the set of
// tables that could have been the last one to execute before reaching the node being visited. With
// this, we should be able to build the flow graph. Effectively, we would piggyback on the existing
// infrastructure in visit_children for supporting ControlFlowVisitor. We don't actually want to
// write a ControlFlowVisitor here, however, since in its current form, ControlFlowVisitor will
// deadlock when there are cycles in the IR; FindFlowGraph is used in part to check that the IR is
// cycle-free.
class FindFlowGraph : public MauInspector {
 private:
    /// The computed flow graph.
    FlowGraph &fg;

    /**
     * The next table that will be executed after the current branch is done executing. For
     * example, in the following IR fragment, while the subtree rooted at t1 is visited, next_table
     * will be t7; while the subtree rooted at t2 is visited, next_table will be t3. This is
     * nullptr when there is no next table (i.e., if control flow would exit to the deparser).
     *
     *          [ t1  t7 ]
     *           /  \
     *    [t2  t3]  [t6]
     *    /  \
     * [t4]  [t5]
     */
    const IR::MAU::Table *next_table;

    /// Helper for determining whether next_table executes immediately after the given table, and
    /// the appropriate edge label to use.
    std::pair<bool, cstring> next_incomplete(const IR::MAU::Table *t);

    Visitor::profile_t init_apply(const IR::Node *node) override;
    bool preorder(const IR::MAU::TableSeq *) override;
    bool preorder(const IR::MAU::Table *) override;
    bool preorder(const IR::MAU::Action *) override;
    void end_apply() override;

 public:
    explicit FindFlowGraph(FlowGraph &out) : fg(out), next_table(nullptr) {
        // We want to re-visit table nodes here, since the table calls can have different
        // next_tables in different contexts.
        visitDagOnce = false;
    }
};

/// Computes a table control-flow graph for each gress in the IR.
class FindFlowGraphs : public MauInspector {
 private:
    /// The computed flow graphs.
    ordered_map<gress_t, FlowGraph> &flow_graphs;

    Visitor::profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::MAU::TableSeq *) override;
    bool preorder(const IR::BFN::Deparser *) override;

 public:
    explicit FindFlowGraphs(ordered_map<gress_t, FlowGraph> &out) : flow_graphs(out) {}
};

#endif /* BF_P4C_MAU_TABLE_FLOW_GRAPH_H_ */
