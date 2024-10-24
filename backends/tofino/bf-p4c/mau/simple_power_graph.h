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

#ifndef BF_P4C_MAU_SIMPLE_POWER_GRAPH_H_
#define BF_P4C_MAU_SIMPLE_POWER_GRAPH_H_

#include <cstring>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <vector>

#include "bf-p4c/mau/mau_power.h"

namespace MauPower {
class Node;
class Edge;

using ::operator<<;

struct NodeAndEdgeSet {
    std::set<const Node *> nodes;
    std::set<const Edge *> edges;
};

/**
 * An edge denotes a connection from a parent to at least one child.
 * More than one child exists in the case of a local execute, global execute,
 * or long branch start.
 */
class Edge {
 public:
    const size_t id_;     // edge id (used for dot output)
    const cstring name_;  // debug name of edge
    cstring to_string() const { return name_; }
    bool is_equivalent(const Edge *other) const;

    /**
     * The edge color denotes what color to display an edge for dot output.
     * Only multi-line edges (one-to-many connections) will be colors other
     * than black.
     */
    std::string get_edge_color() const;

    explicit Edge(size_t id, cstring name, std::vector<Node *> &child_nodes)
        : id_(id), name_(name) {
        for (auto n : child_nodes) {
            child_nodes_.push_back(n);
        }
    }
    // Encapsulation of what next Node(s) (logical table(s)) this edge connects to.
    std::vector<Node *> child_nodes_;
    void dbprint(std::ostream &out, NodeAndEdgeSet *seen) const;
    void dbprint(std::ostream &out) const {
        NodeAndEdgeSet seen;
        dbprint(out, &seen);
    }
};

/**
 * A Node represents a single logical table (isolated to a single MAU stage).
 * Nodes have outgoing edges, that denote the control flow from this node to
 * child nodes.
 */
class Node {
 public:
    const UniqueId unique_id_;
    const int id_;                   // node id (used for dot output)
    std::vector<Edge *> out_edges_;  // collection of outbound edges

    explicit Node(UniqueId uniq_id, int id) : unique_id_(uniq_id), id_(id) {}
    ~Node() {
        for (auto e : out_edges_) delete e;
    }
    Node(const Node &n) : unique_id_(n.unique_id_), id_(n.id_) {  // copy
        for (auto e : n.out_edges_) add_edge(e);
    }
    Node &operator=(const Node &n) = delete;   // copy assignment
    Node(const Node &&n) = delete;             // move
    Node &operator=(const Node &&n) = delete;  // move assignment

    /**
     * Create an outgoing edge from this node.
     * @param edge_name A debug name to give the edge, to be used in dot display.
     * @param child_nodes A vector of Node pointers that indicate what this
     *                    Node is followed by.
     */
    void create_and_add_edge(cstring edge_name, std::vector<Node *> &child_nodes);
    bool is_equivalent(const Node *other) const;
    cstring to_string() const { return "" + unique_id_.build_name(); }
    void dbprint(std::ostream &out, NodeAndEdgeSet *seen) const;
    void dbprint(std::ostream &out) const {
        NodeAndEdgeSet seen;
        dbprint(out, &seen);
    }

 private:
    void add_edge(Edge *e);
};

/**
 * This class represents the final placed MAU table control flow graph for
 * a given thread of execution: ingress, egress, or ghost.
 * It consists of Nodes that represent logical tables and Edges that represent
 * the flow from one logical table to one or more subsequent logical tables.
 * For Tofino2 and beyond, an Edge is allowed to be a one-to-many relationship,
 * since Tofino2+ allows initiation of multiple parallel threads of execution
 * via local_exec, global_exec, and long branching.
 */
class SimplePowerGraph : public IHasDbPrint {
    std::map<UniqueId, Node *> nodes_ = {};
    std::map<UniqueId, std::set<UniqueId>> pred_ = {};

 public:
    const cstring name_;  // graph name, e.g. ingress or egress

    explicit SimplePowerGraph(cstring gress) : name_(gress), running_id_(1) {
        UniqueId root_uid;
        root_uid.name = "$root"_cs;
        root_ = new Node(root_uid, 0);
        nodes_.emplace(root_uid, root_);
    }
    Node *get_root() const { return root_; }
    /**
     * Connects a parent Node to one or more child nodes via an Edge.
     * This function creates Node and Edge objects, if they do not already exist.
     * @param parent The unique representation of a parent logical table.
     * @param activated The set of unique IDs for what children this parent may
     *                  activate next in the control flow.
     * @param edge_name A debug name to give the edge, to be used in dot display.
     */
    void add_connection(UniqueId parent, ordered_set<UniqueId> activated, cstring edge_name);
    /**
     * Output this graph to the dot file, as specified by the filename.
     */
    void to_dot(cstring filename);
    /**
     * Returns a vector of reachable leaf nodes in the graph from the provided
     * Node for the given UniqueId.
     */
    void get_leaves(UniqueId n, std::vector<UniqueId> &leaves);
    /**
     * Returns true if there a 'downward' connection from parent to child.
     */
    bool can_reach(UniqueId parent, UniqueId child);
    /**
      * Returns true if these tables can be active at the same time.
      * For Tofino, this function will return identical results to can_reach.
      * For Tofino2 and beyond, this takes into account parallel sub-graphs.
      * e.g.
      *           ---> C ----> D
      *          /              \
      *    ---> A               E ---> F
      *          \             /
      *           ---> B ------
      * If A activates both B and C, then even though B cannot reach D,
      * we have to consider B and D as activate at the same time for
      * dependency analysis.
      */
    bool active_simultaneously(UniqueId parent, UniqueId child);
    /**
     * Returns the predecessors of a node -- the tables that can invoke this node
     */
    const std::set<UniqueId> &predecessors(UniqueId child) const {
        if (pred_.count(child)) return pred_.at(child);
        static std::set<UniqueId> empty;
        return empty;
    }
    /**
     * Returns a topological sort of the Nodes in this graph.
     */
    std::vector<Node *> topo_sort();
    /**
     * For power estimation, returns a set of the Nodes that are found on the
     * worst-case table control flow.
     * A mapping form UniqueId to estimated power consumption is provided as
     * the "cost" for each Node.
     */
    double get_tables_on_worst_path(
        const std::map<UniqueId, PowerMemoryAccess> &table_memory_access, std::set<Node *> &rv);

    void dbprint(std::ostream &out, NodeAndEdgeSet *seen) const;
    void dbprint(std::ostream &out) const {
        NodeAndEdgeSet seen;
        dbprint(out, &seen);
    }

 private:
    int running_id_;  // For producing unique integer values of graph Nodes.
    Node *root_;
    // Memoize if a parent-child relationship is reachable.
    std::map<std::pair<int, int>, bool> found_reachable_ = {};
    // Memoize if a parent-child pair is active simultaneously.
    std::map<std::pair<int, int>, bool> found_simultaneous_ = {};
    // Memoize worst-case power computed for a node.
    std::map<int, double> computed_power_ = {};
    // Memoize worst_path computed for a node;
    std::map<int, std::set<UniqueId>> computed_worst_path_ = {};
    // Keep track of who parent was on worst-case power path.
    std::map<Node *, Edge *> worst_path_edges_ = {};

    /**
     * Creates a new Node object, unless node has already been created.
     * If Node already exists, return existing.
     */
    Node *create_node(UniqueId id);
    int get_next_id() { return running_id_++; }
    void topo_visit(Node *node, std::stack<Node *> *stack, std::map<Node *, bool> *visited);
    /**
     * Helper post-order visit of node, returning worst case power from
     * this node to end of control flow path(s).
     * The worst_path set is the set of Nodes after and including the
     * current node that are found to require the most power usage.
     */
    double visit_node_power(Node *n, const std::map<UniqueId, PowerMemoryAccess> &tma,
                            std::set<UniqueId> &worst_path);
};

}  // end namespace MauPower

#endif /* BF_P4C_MAU_SIMPLE_POWER_GRAPH_H_ */
