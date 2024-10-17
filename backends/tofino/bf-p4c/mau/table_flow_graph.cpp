/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <boost/graph/graphviz.hpp>
#include "bf-p4c/mau/table_flow_graph.h"

std::ostream &operator<<(std::ostream &out, const FlowGraph &fg) {
    auto all_vertices = boost::vertices(fg.g);
    if (++all_vertices.first == all_vertices.second) {
        out << "FLOW_GRAPH EMPTY" << std::endl;
        return out;
    }
    auto source_gress = fg.get_vertex(fg.v_source)->gress;
    out << "FLOW_GRAPH (" << source_gress << ")" << std::endl;
    FlowGraph::Graph::edge_iterator edges, edges_end;
    for (boost::tie(edges, edges_end) = boost::edges(fg.g); edges != edges_end; ++edges) {
        auto src = boost::source(*edges, fg.g);
        const IR::MAU::Table* source = fg.get_vertex(src);
        auto dst = boost::target(*edges, fg.g);
        const IR::MAU::Table* target = fg.get_vertex(dst);
        auto desc = fg.get_ctrl_dependency_info(*edges);
        out << "    " << (source ? source->name : "DEPARSER") <<
            (src == fg.v_source ? " (PARSER)" : "") << " -- " << desc << " --> " <<
            (target ? target->name : "DEPARSER") << std::endl;
    }
    return out;
}

std::string FlowGraph::viz_node_name(const IR::MAU::Table* tbl) {
    std::string name = std::string(tbl ? tbl->name : "DEPARSER");
    std::replace(name.begin(), name.end(), '-', '_');
    std::replace(name.begin(), name.end(), '.', '_');
    return name;
}

void FlowGraph::dump_viz(std::ostream &out, const FlowGraph::DumpTableDetails* details) {
    auto all_vertices = boost::vertices(g);
    if (++all_vertices.first == all_vertices.second) {
        out << "digraph empty {\n}" << std::endl;
        return;
    }
    auto source_gress = get_vertex(v_source)->gress;
    out << "digraph " << source_gress << " {" << std::endl;

    FlowGraph::Graph::vertex_iterator nodes, nodes_end;
    for (boost::tie(nodes, nodes_end) = boost::vertices(g); nodes != nodes_end; ++nodes) {
        auto t = vertexToTable.at(boost::vertex(*nodes, g));
        if (t && details)
            details->dump(out, t);
    }

    FlowGraph::Graph::edge_iterator edges, edges_end;
    for (boost::tie(edges, edges_end) = boost::edges(g); edges != edges_end; ++edges) {
        auto src = boost::source(*edges, g);
        const IR::MAU::Table* source = get_vertex(src);

        auto dst = boost::target(*edges, g);
        const IR::MAU::Table* target = get_vertex(dst);

        std::string src_name = viz_node_name(source);
        std::string dst_name = viz_node_name(target);

        if (src == v_source) out << "    PARSER -> " << src_name << std::endl;
        out << "    " << src_name << " -> " << dst_name << std::endl;
    }
    out << "}" << std::endl;
}

const std::set<const IR::MAU::Table*>
FlowGraph::get_dominators(const IR::MAU::Table* table) const {
    if (!dominators) {
        // Compute dominator sets. Taken from the Wikipedia article on dominators.
        using TableSet = std::set<const IR::MAU::Table*>;
        using DominatorMap = std::map<const IR::MAU::Table*, TableSet>;
        dominators = std::optional<DominatorMap>(DominatorMap());
        auto& dominators = *this->dominators;

        // The start node dominates itself. For all other nodes, set all nodes as the
        // dominators.
        auto* source = get_vertex(v_source);
        for (auto* table : tables) {
            if (table == source) dominators[table].insert(table);
            else
                dominators[table] = tables;
        }
        dominators[nullptr] = tables;

        // The dominator set for each table is the intersection of the dominator sets of all the
        // table's predecessors. This means that any time we recompute a table's dominator set, its
        // successors will also need to be recomputed.
        //
        // Start by recomputing all of the start node's successors.
        TableSet toRecompute;
        typename FlowGraph::Graph::out_edge_iterator out_edges, out_edges_end;
        boost::tie(out_edges, out_edges_end) = boost::out_edges(v_source, g);
        for (; out_edges != out_edges_end; ++out_edges) {
            auto target = boost::target(*out_edges, g);
            toRecompute.insert(get_vertex(target));
        }

        while (!toRecompute.empty()) {
            auto* table = *toRecompute.begin();
            toRecompute.erase(table);

            // Recompute the dominator set for the current table. Take the intersection of
            // the current dominator sets of all the table's predecessors.
            TableSet recomputed = tables;

            typename FlowGraph::Graph::in_edge_iterator edges, edges_end;
            boost::tie(edges, edges_end) = boost::in_edges(get_vertex(table), g);
            BUG_CHECK(edges != edges_end, "Table flow graph has more than one source node");
            for (; edges != edges_end; ++edges) {
                auto src = boost::source(*edges, g);
                const IR::MAU::Table* parent = get_vertex(src);
                TableSet parentDominators = dominators.at(parent);
                TableSet intersection;
                std::set_intersection(recomputed.begin(), recomputed.end(),
                                      parentDominators.begin(), parentDominators.end(),
                                      std::inserter(intersection, intersection.begin()));

                recomputed = std::move(intersection);
            }

            // Add the current table itself to the recomputed set.
            recomputed.insert(table);

            if (recomputed != dominators.at(table)) {
                // Save the recomputed set and add the current table's successors to the set of
                // tables whose dominator sets need to be recomputed.
                dominators[table] = std::move(recomputed);

                boost::tie(out_edges, out_edges_end) = boost::out_edges(get_vertex(table), g);
                for (; out_edges != out_edges_end; ++out_edges) {
                    auto target = boost::target(*out_edges, g);
                    toRecompute.insert(get_vertex(target));
                }
            }
        }

        // Remove the null table from the dominator set for the sink.
        dominators[nullptr].erase(nullptr);
    }

    return dominators->at(table);
}

bool FlowGraph::is_always_reached(const IR::MAU::Table* table) const {
    // Check that the table dominates the graph's sink node.
    return get_dominators(nullptr).count(table);
}

Visitor::profile_t FindFlowGraph::init_apply(const IR::Node* node) {
    auto rv = Inspector::init_apply(node);
    fg.clear();
    fg.add_sink_vertex();
    return rv;
}

bool FindFlowGraph::preorder(const IR::MAU::TableSeq* table_seq) {
    if (table_seq->tables.size() < 2) {
        return Inspector::preorder(table_seq);
    }

    // Override the behaviour for visiting table sequences so that we accurately track next_table.

    const auto* saved_next_table = next_table;

    bool first_iter = true;
    const IR::MAU::Table* cur_table = nullptr;
    for (const auto* next_table : table_seq->tables) {
        if (!first_iter) {
            // Visit cur_table with the new next_table.
            this->next_table = next_table;
            apply_visitor(cur_table);
        }

        first_iter = false;
        cur_table = next_table;
    }

    // Restore the saved next_table and visit the last table in the sequence.
    this->next_table = saved_next_table;
    apply_visitor(cur_table);

    return false;
}

std::pair<bool, cstring> FindFlowGraph::next_incomplete(const IR::MAU::Table *t) {
    // TODO: Handle $try_next_stage
    if (t->next.count("$hit"_cs) && t->next.count("$miss"_cs))
        return std::make_pair(false, ""_cs);

    if (t->next.count("$hit"_cs))
        // Miss falls through to next_table.
        return std::make_pair(true, "$miss"_cs);

    if (t->next.count("$miss"_cs))
        // Hit falls through to next_table.
        return std::make_pair(true, "$hit"_cs);

    if (t->next.count("$true"_cs) && t->next.count("$false"_cs))
        return std::make_pair(false, ""_cs);

    if (t->next.count("$true"_cs))
        // "false" case falls through to next_table.
        return std::make_pair(true, "$false"_cs);

    if (t->next.count("$false"_cs))
        // "true" case falls through to next_table.
        return std::make_pair(true, "$true"_cs);

    if (t->next.count("$default"_cs))
        return std::make_pair(false, ""_cs);

    if (t->next.size() == 0)
        return std::make_pair(true, "$default"_cs);

    // See if we have a next-table entry for every action. If not, next_table can be executed after
    // the given table.
    for (auto kv : t->actions) {
        if (!t->next.count(kv.first)) {
            return std::make_pair(true, "$hit"_cs);
        }
    }

    return std::make_pair(false, ""_cs);
}

bool FindFlowGraph::preorder(const IR::MAU::Table *t) {
    // Add edges for next-table entries.
    for (auto& next_entry : t->next) {
        auto& action_name = next_entry.first;
        auto& next_table_seq = next_entry.second;

        const IR::MAU::Table *dst;
        if (next_table_seq->tables.size() > 0) {
            dst = next_table_seq->tables[0];
        } else {
            // This will sometimes be null, which will cause an edge to be added to v_sink
            dst = next_table;
        }
        auto dst_name = dst ? dst->name : "DEPARSER";
        LOG1("Parent : " << t->name << " --> " << action_name << " --> " << dst_name);
        fg.add_edge(t, dst, action_name);
    }

    // Add edge for t -> next_table, if needed.
    LOG3("Table: " << t->name << " Next: " <<
        (next_table ? next_table->name : "<null>"));
    auto n = next_incomplete(t);
    LOG3("next - " << n.first << ":" << n.second);
    if (n.first) {
        auto dst_name = next_table ? next_table->name : "DEPARSER";
        LOG1("Parent : " << t->name << " --> " << n.second << " --> " << dst_name);
        // This will sometimes be null, which will cause an edge to be added to v_sink
        fg.add_edge(t, next_table, n.second);
    }
    return true;
}

bool FindFlowGraph::preorder(const IR::MAU::Action *act) {
    // Add deparser edges for exit actions (those that terminate MAU processing)
    if (act->exitAction) {
        if (auto *t = findContext<IR::MAU::Table>()) {
            LOG3("Parent : " << t->name << " --> " << act->name << " --> DEPARSER");
            fg.add_edge(t, nullptr, act->name);
        }
    }
    return true;
}

void FindFlowGraph::end_apply() {
    // Find the source node (i.e., the node with no incoming edges), and make sure there is only
    // one. As we do this, also populate tableToVertexIndex.
    typename FlowGraph::Graph::vertex_iterator v, v_end;
    bool source_found = false;
    for (boost::tie(v, v_end) = boost::vertices(fg.g); v != v_end; ++v) {
        fg.tableToVertexIndex[fg.get_vertex(*v)] = *v;

        if (*v == fg.v_sink)
            continue;

        auto in_edge_pair = boost::in_edges(*v, fg.g);
        if (in_edge_pair.first == in_edge_pair.second) {
            if (source_found) {
                LOG1("Source already seen -- are you running the flow graph on >1 gresses?");
                LOG1("Possible v_source extra is " << fg.get_vertex(*v)->name);
            } else {
                fg.v_source = *v;
                source_found = true;
            }
        }
    }

    LOG4(fg);
    if (LOGGING(4))
        fg.dump_viz(std::cout);
}

Visitor::profile_t FindFlowGraphs::init_apply(const IR::Node* root) {
    // Clear the flow graphs every time the visitor is applied.
    auto result = Inspector::init_apply(root);
    flow_graphs.clear();
    return result;
}

bool FindFlowGraphs::preorder(const IR::MAU::TableSeq* thread) {
    // Each top-level TableSeq should represent the whole MAU pipeline for a single gress. Build
    // the control-flow graph for that.
    if (!thread->empty()) {
        auto gress = thread->front()->gress;
        thread->apply(FindFlowGraph(flow_graphs[gress]));
    }

    // Prune the visitor.
    return false;
}

bool FindFlowGraphs::preorder(const IR::BFN::Deparser* dep)  {
    // We need to check if the given egress already exists in the sequence.
    // * yes - just continue
    // * no - we need to insert empty flow graph which will be later used in dominator tree code
    auto gress = dep->gress;
    if (!flow_graphs.count(gress)) {
        LOG1("Non-existing gress has been detected for " <<
            gress << ", inserting the empty flow graph.");
        auto& empty = flow_graphs[gress];
        empty.gress = gress;
    }

    // Prune the visitor
    return false;
}
