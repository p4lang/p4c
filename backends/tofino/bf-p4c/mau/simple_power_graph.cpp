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

#include "simple_power_graph.h"

#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <vector>

#include "ir/ir.h"
#include "lib/map.h"

namespace MauPower {

bool Edge::is_equivalent(const Edge *other) const {
    // Don't care about debug values, just about connectivity.
    if (child_nodes_.size() != other->child_nodes_.size()) return false;
    for (auto *c1 : child_nodes_) {
        bool found = false;
        for (auto *c2 : other->child_nodes_) {
            if (c1->is_equivalent(c2)) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

std::string Edge::get_edge_color() const {
    constexpr std::array<const char *, 8> LUT = {"red",    "blue",   "green", "orange",
                                                 "purple", "yellow", "brown", "cyan"};
    if (child_nodes_.size() > 1) {
        return LUT.at(id_ % LUT.size());
    }
    return "black";
}

void Node::create_and_add_edge(cstring edge_name, std::vector<Node *> &child_nodes) {
    if (LOGGING(4)) {
        LOG4("create_and_add_edge:");
        LOG4("   edge_name " << edge_name);
        LOG4("   child_nodes: " << child_nodes.size());
        for (auto c : child_nodes) {
            LOG4("     " << c->unique_id_);
        }
    }

    Edge *e = new Edge(out_edges_.size(), edge_name, child_nodes);
    add_edge(e);
}

void Node::add_edge(Edge *edge) {
    bool found = false;
    for (auto e : out_edges_) {
        if (e->is_equivalent(edge)) {
            found = true;
            break;
        }
    }
    if (!found) out_edges_.push_back(edge);
}

bool Node::is_equivalent(const Node *other) const {
    // Don't care about debug values, just about connectivity.
    if (unique_id_ != other->unique_id_) return false;
    if (out_edges_.size() != other->out_edges_.size()) return false;
    for (auto *e1 : out_edges_) {
        bool found = false;
        for (auto *e2 : other->out_edges_) {
            if (e1->is_equivalent(e2)) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

Node *SimplePowerGraph::create_node(UniqueId id) {
    Node *&n = nodes_[id];
    if (!n) {
        LOG4("create_node " << name_ << "::" << id);
        n = new Node(id, get_next_id());
    }
    return n;
}

void SimplePowerGraph::add_connection(UniqueId parent, ordered_set<UniqueId> activated,
                                      cstring edge_name) {
    // Setup parent
    Node *pnode = create_node(parent);
    // Setup edge
    std::vector<Node *> child_nodes;
    for (auto u : activated) {
        LOG4(parent << " -> " << u);
        Node *child = create_node(u);
        if (child) child_nodes.push_back(child);
        pred_[u].insert(parent);
    }
    if (child_nodes.size() > 0) pnode->create_and_add_edge(edge_name, child_nodes);
}

/**
 * Outputs the final placed table control flow graphs to a .dot file.
 */
void SimplePowerGraph::to_dot(cstring filename) {
    std::ofstream myfile;
    myfile.open(filename);

    std::queue<Node *> queue;
    queue.push(get_root());
    std::map<UniqueId, bool> visited = {};

    myfile << "digraph " << name_ << " {" << std::endl;

    // nodes
    for (auto i : nodes_) {
        myfile << i.second->id_;
        myfile << " [label=\"" << i.second->unique_id_ << "\"";
        myfile << " shape=box color=\"black\"];" << std::endl;
    }
    // edges
    for (auto i : nodes_) {
        for (auto e : i.second->out_edges_) {
            for (auto c : e->child_nodes_) {
                myfile << i.second->id_ << "-> " << c->id_;
                cstring ename = e->name_;
                myfile << " [label=\"" << ename << "\" color=\"" << e->get_edge_color() << "\"];"
                       << std::endl;
            }
        }
    }
    myfile << "}" << std::endl;
    myfile.close();
}

void SimplePowerGraph::get_leaves(UniqueId node, std::vector<UniqueId> &leaves) {
    if (!nodes_.count(node)) {
        return;
    }
    std::queue<Node *> queue;
    queue.push(nodes_.at(node));
    std::set<UniqueId> visited = {};
    while (!queue.empty()) {
        Node *cur = queue.front();
        queue.pop();
        if (visited.count(cur->unique_id_)) {  // already visited
            continue;
        }
        visited.insert(cur->unique_id_);
        if (cur->out_edges_.size() == 0) {  // leaf
            leaves.push_back(cur->unique_id_);
        }
        for (auto e : cur->out_edges_) {
            for (auto c : e->child_nodes_) {
                if (!visited.count(c->unique_id_)) {  // not found yet
                    queue.push(c);
                }
            }
        }
    }
}

/**
 * Topological visit of table control flow graph.
 */
void SimplePowerGraph::topo_visit(Node *node, std::stack<Node *> *stack,
                                  std::map<Node *, bool> *visited) {
    visited->emplace(node, true);
    for (auto *e : node->out_edges_) {
        for (auto *c : e->child_nodes_) {
            if (visited->find(c) == visited->end()) {  // not visited
                topo_visit(c, stack, visited);
            }
        }
    }
    stack->push(node);
}

/**
 * Returns a topological sorting of the graph.
 */
std::vector<Node *> SimplePowerGraph::topo_sort() {
    std::vector<Node *> topo;
    std::map<Node *, bool> visited = {};
    std::stack<Node *> stack;

    topo_visit(get_root(), &stack, &visited);
    while (!stack.empty()) {
        auto n = stack.top();
        stack.pop();
        topo.push_back(n);
    }
    return topo;
}

bool SimplePowerGraph::can_reach(UniqueId parent, UniqueId child) {
    if (!nodes_.count(parent) || !nodes_.count(child)) {
        return false;
    }
    LOG6("can_reach(" << parent << ", " << child << ")");
    std::pair<int, int> mem = std::make_pair(nodes_.at(parent)->id_, nodes_.at(child)->id_);
    if (found_reachable_.find(mem) != found_reachable_.end()) return found_reachable_.at(mem);
    std::queue<Node *> queue;
    queue.push(nodes_.at(parent));
    std::map<UniqueId, bool> visited = {};
    while (!queue.empty()) {
        Node *cur = queue.front();
        queue.pop();
        if (cur->unique_id_ == child) {
            found_reachable_.emplace(mem, true);
            LOG6("  true");
            return true;
        }
        if (visited.find(cur->unique_id_) != visited.end()) {  // already visited
            continue;
        }
        visited.emplace(cur->unique_id_, true);
        for (auto e : cur->out_edges_) {
            for (auto c : e->child_nodes_) {
                if (visited.find(c->unique_id_) == visited.end()) {  // not found yet
                    queue.push(c);
                }
            }
        }
    }
    found_reachable_.emplace(mem, false);
    LOG6("  false");
    return false;
}

bool SimplePowerGraph::active_simultaneously(UniqueId parent, UniqueId child) {
    if (!nodes_.count(parent) || !nodes_.count(child)) {
        return false;
    }
    LOG6("active_simultaneously(" << parent << ", " << child << ")");
    std::pair<int, int> mem = std::make_pair(nodes_.at(parent)->id_, nodes_.at(child)->id_);
    if (found_simultaneous_.find(mem) != found_simultaneous_.end())
        return found_simultaneous_.at(mem);
    if (can_reach(parent, child)) {
        LOG6("  active_simul = true");
        found_simultaneous_.emplace(mem, true);
        return true;
    }
    // If there exists any Edge where the 'left' node activates parallel sub-graphs
    // that include both parent and child, then parent and child are considered as
    // active at the same time.
    for (auto n : nodes_) {
        for (auto e : n.second->out_edges_) {
            bool reach_p = false;
            bool reach_c = false;
            // parallel sub-graphs only occurs when an Edge has more than one
            // child node
            if (e->child_nodes_.size() > 1) {
                for (auto n2 : e->child_nodes_) {
                    if (can_reach(n2->unique_id_, parent)) reach_p = true;
                    if (can_reach(n2->unique_id_, child)) reach_c = true;
                }
            }
            if (reach_p && reach_c) {
                LOG6("  active_simul = true");
                found_simultaneous_.emplace(mem, true);
                return true;
            }
        }
    }
    LOG6("  active_simul = false");
    found_simultaneous_.emplace(mem, false);
    return false;
}

double SimplePowerGraph::get_tables_on_worst_path(const std::map<UniqueId, PowerMemoryAccess> &tma,
                                                  std::set<Node *> &rv) {
    Log::TempIndent indent;
    LOG5("Get " << name_ << " tables on worst path" << indent);
    LOG7(*this);

    std::set<UniqueId> worst_path;
    double pwr = visit_node_power(get_root(), tma, worst_path);
    std::map<Node *, bool> worst = {};
    std::queue<Node *> queue;
    queue.push(get_root());
    while (!queue.empty()) {
        Node *n = queue.front();
        queue.pop();
        if (worst.find(n) == worst.end()) {
            if (worst_path_edges_.find(n) != worst_path_edges_.end()) {
                Edge *e = worst_path_edges_.at(n);
                for (Node *child : e->child_nodes_) {
                    queue.push(child);
                }
            }
            worst.emplace(n, true);
        }
    }
    for (auto w : worst) {
        rv.insert(w.first);
    }
    return pwr;
}

double SimplePowerGraph::visit_node_power(Node *n, const std::map<UniqueId, PowerMemoryAccess> &tma,
                                          std::set<UniqueId> &worst_path) {
    Log::TempIndent indent;
    // LOG3("Visiting node for power: " << n->unique_id_ << indent);
    if (computed_power_.find(n->id_) != computed_power_.end()) {
        worst_path = computed_worst_path_.at(n->id_);
        return computed_power_.at(n->id_);
    }
    // Determine which outgoing edge has the worst-case power estimate.
    double worst_power = 0.0;
    std::set<UniqueId> local_worst_path;  // after determine worst path, return in worst path
    for (auto e : n->out_edges_) {
        // Do not double-count tables that will be seen in "multi-execute" paths, e.g.
        // when a long branch that activates multiple tables reconverges into the control flow.
        std::set<UniqueId> edge_worst_path;
        for (auto c : e->child_nodes_) {
            std::set<UniqueId> next_worst_path;
            visit_node_power(c, tma, next_worst_path);
            for (auto uid : next_worst_path) {
                edge_worst_path.insert(uid);
            }
        }
        double edge_power = 0.0;
        for (auto uid : edge_worst_path) {
            if (auto pma = ::getref(tma, uid)) {
                edge_power += pma->compute_table_power(Device::numPipes());
                // LOG5("\tEdge Power (" << uid << ") :"
                // << pma->compute_table_power(Device::numPipes()));
            }
        }
        // LOG5("Edge Power Out Edge (" << e->to_string() << ") :" << edge_power
        //         << ", Worst Power: " << worst_power);

        // for (auto c : e->child_nodes_) {
        //    LOG4("\tEdge " << e->unique_id_ << " --> " << c->unique_id_
        //            << " : " << computed_power_.at(c->id_));
        // }
        if (edge_power > worst_power) {
            worst_power = edge_power;
            worst_path_edges_[n] = e;
            local_worst_path.clear();
            for (auto uid : edge_worst_path) {
                local_worst_path.insert(uid);
            }
        }
    }
    if (auto pma = ::getref(tma, n->unique_id_))
        worst_power += pma->compute_table_power(Device::numPipes());
    computed_power_.emplace(n->id_, worst_power);
    worst_path.insert(n->unique_id_);
    for (auto uid : local_worst_path) {
        worst_path.insert(uid);
    }
    computed_worst_path_.emplace(n->id_, worst_path);
    LOG4("Visiting node for power: " << n->unique_id_ << " is " << float2str(worst_power)
                                     << " with worst path - " << worst_path);
    return worst_power;
}

void Node::dbprint(std::ostream &out, NodeAndEdgeSet *seen) const {
    out << "[" << id_ << "] " << unique_id_;
    if (seen->nodes.insert(this).second) {
        if (!out_edges_.empty()) {
            out << Log::indent;
            for (auto *e : out_edges_) {
                out << Log::endl;
                e->dbprint(out, seen);
            }
            out << Log::unindent;
        }
    } else {
        out << " ...";
    }
}

void Edge::dbprint(std::ostream &out, NodeAndEdgeSet *seen) const {
    out << "[" << id_ << "] " << name_;
    if (seen->edges.insert(this).second) {
        if (!child_nodes_.empty()) {
            out << Log::indent;
            for (auto *c : child_nodes_) {
                out << Log::endl;
                c->dbprint(out, seen);
            }
            out << Log::unindent;
        }
    } else {
        out << " ...";
    }
}

void SimplePowerGraph::dbprint(std::ostream &out, NodeAndEdgeSet *seen) const {
    out << "SimplePowerGraph " << name_ << Log::indent << Log::endl;
    root_->dbprint(out, seen);
    out << Log::unindent;
}

}  // end namespace MauPower
