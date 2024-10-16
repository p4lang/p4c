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

#include "bf-p4c/mau/table_dependency_graph.h"
#include <assert.h>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/lookup_edge.hpp>
#include <boost/optional/optional_io.hpp>
#include "bf-p4c/common/run_id.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/lib/error_type.h"
#include "bf-p4c/logging/manifest.h"
#include "table_injected_deps.h"
#include "bf-p4c/mau/default_next.h"
#include "bf-p4c/mau/table_placement.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/ir/table_tree.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/log.h"


bool NameToTableMapBuilder::preorder(const IR::MAU::Table *tbl) {
    dg.name_to_table[tbl->externalName()] = tbl;
    return true;
}

// Static Map Init
ordered_map<DependencyGraph::dependencies_t, cstring> TableGraphEdge::labels_to_types = {
    { DependencyGraph::IXBAR_READ,                  "match"_cs},
    { DependencyGraph::ACTION_READ,                 "action"_cs},
    { DependencyGraph::OUTPUT,                      "action"_cs},
    { DependencyGraph::CONT_CONFLICT,               "action"_cs},
    { DependencyGraph::REDUCTION_OR_READ,           "reduction_or"_cs},
    { DependencyGraph::REDUCTION_OR_OUTPUT,         "reduction_or"_cs},
    { DependencyGraph::ANTI_TABLE_READ,             "control"_cs},
    { DependencyGraph::ANTI_ACTION_READ,            "control"_cs},
    { DependencyGraph::ANTI_NEXT_TABLE_DATA,        "control"_cs},
    { DependencyGraph::ANTI_NEXT_TABLE_CONTROL,     "control"_cs},
    { DependencyGraph::ANTI_NEXT_TABLE_METADATA,    "control"_cs},
    { DependencyGraph::ANTI_EXIT,                   "control"_cs},
    { DependencyGraph::NONE,                        "none"_cs},
    { DependencyGraph::CONTROL_ACTION,              "control"_cs},
    { DependencyGraph::CONTROL_COND_TRUE,           "control"_cs},
    { DependencyGraph::CONTROL_COND_FALSE,          "control"_cs},
    { DependencyGraph::CONTROL_TABLE_HIT,           "control"_cs},
    { DependencyGraph::CONTROL_TABLE_MISS,          "control"_cs},
    { DependencyGraph::CONTROL_DEFAULT_NEXT_TABLE,  "control"_cs},
    { DependencyGraph::CONTROL_EXIT,                "control"_cs}
};
ordered_map<DependencyGraph::dependencies_t, cstring> TableGraphEdge::labels_to_sub_types = {
    { DependencyGraph::IXBAR_READ,                  "ixbar_read"_cs},
    { DependencyGraph::ACTION_READ,                 "action_read"_cs},
    { DependencyGraph::OUTPUT,                      "output"_cs},
    { DependencyGraph::CONT_CONFLICT,               "cont_conflict"_cs},
    { DependencyGraph::REDUCTION_OR_READ,           "reduction_or_read"_cs},
    { DependencyGraph::REDUCTION_OR_OUTPUT,         "reduction_or_output"_cs},
    { DependencyGraph::ANTI_TABLE_READ,             "anti"_cs},
    { DependencyGraph::ANTI_ACTION_READ,            "anti"_cs},
    { DependencyGraph::ANTI_NEXT_TABLE_DATA,        "anti"_cs},
    { DependencyGraph::ANTI_NEXT_TABLE_CONTROL,     "anti"_cs},
    { DependencyGraph::ANTI_NEXT_TABLE_METADATA,    "anti"_cs},
    { DependencyGraph::ANTI_EXIT,                   "exit"_cs},
    { DependencyGraph::NONE,                        "none"_cs},
    { DependencyGraph::CONTROL_ACTION,              "action"_cs},
    { DependencyGraph::CONTROL_COND_TRUE,           "condition"_cs},
    { DependencyGraph::CONTROL_COND_FALSE,          "condition"_cs},
    { DependencyGraph::CONTROL_TABLE_HIT,           "table_hit"_cs},
    { DependencyGraph::CONTROL_TABLE_MISS,          "table_miss"_cs},
    { DependencyGraph::CONTROL_DEFAULT_NEXT_TABLE,  "default_next_table"_cs},
    { DependencyGraph::CONTROL_EXIT,                "control_exit"_cs}
};
ordered_map<DependencyGraph::dependencies_t, cstring> TableGraphEdge::labels_to_anti_types = {
    { DependencyGraph::ANTI_TABLE_READ,             "table_read"_cs},
    { DependencyGraph::ANTI_ACTION_READ,            "action_read"_cs},
    { DependencyGraph::ANTI_NEXT_TABLE_DATA,        "next_table_data"_cs},
    { DependencyGraph::ANTI_NEXT_TABLE_CONTROL,     "next_table_control"_cs},
    { DependencyGraph::ANTI_NEXT_TABLE_METADATA,    "table_metadata"_cs}
};
ordered_map<DependencyGraph::dependencies_t, bool> TableGraphEdge::labels_to_conds = {
    { DependencyGraph::CONTROL_COND_TRUE,  true},
    { DependencyGraph::CONTROL_COND_FALSE, false}
};

static const char* dep_types(DependencyGraph::dependencies_t dep) {
    switch (dep) {
        case DependencyGraph::NONE:                         return "NONE";
        case DependencyGraph::CONTROL_ACTION:               return "CONTROL_ACTION";
        case DependencyGraph::CONTROL_COND_TRUE:            return "CONTROL_COND_TRUE";
        case DependencyGraph::CONTROL_COND_FALSE:           return "CONTROL_COND_FALSE";
        case DependencyGraph::CONTROL_TABLE_HIT:            return "CONTROL_TABLE_HIT";
        case DependencyGraph::CONTROL_TABLE_MISS:           return "CONTROL_TABLE_MISS";
        case DependencyGraph::CONTROL_DEFAULT_NEXT_TABLE:   return "CONTROL_DEFAULT_NEXT_TABLE";
        case DependencyGraph::CONTROL_EXIT:                 return "CONTROL_EXIT";
        case DependencyGraph::CONT_CONFLICT:                return "CONT_CONFLICT";
        case DependencyGraph::IXBAR_READ:                   return "IXBAR_READ";
        case DependencyGraph::ACTION_READ:                  return "ACTION_READ";
        case DependencyGraph::ANTI_EXIT:                    return "ANTI_EXIT";
        case DependencyGraph::ANTI_TABLE_READ:              return "ANTI_TABLE_READ";
        case DependencyGraph::ANTI_ACTION_READ:             return "ANTI_ACTION_READ";
        case DependencyGraph::ANTI_NEXT_TABLE_DATA:         return "ANTI_NEXT_TABLE_DATA";
        case DependencyGraph::ANTI_NEXT_TABLE_CONTROL:      return "ANTI_NEXT_TABLE_CONTROL";
        case DependencyGraph::ANTI_NEXT_TABLE_METADATA:     return "ANTI_NEXT_TABLE_METADATA";
        case DependencyGraph::OUTPUT:                       return "OUTPUT";
        case DependencyGraph::REDUCTION_OR_READ:            return "REDUCTION_OR_READ";
        case DependencyGraph::REDUCTION_OR_OUTPUT:          return "REDUCTION_OR_OUTPUT";
        default:                                            return "UNKNOWN";
    }
}

std::ostream &operator<<(std::ostream &out, const DependencyGraph &dg) {
    out << "GRAPH" << std::endl;
    DependencyGraph::Graph::edge_iterator edges, edges_end;
    for (boost::tie(edges, edges_end) = boost::edges(dg.g); edges != edges_end; ++edges) {
        auto src = boost::source(*edges, dg.g);
        const IR::MAU::Table* source = dg.get_vertex(src);
        auto dst = boost::target(*edges, dg.g);
        const IR::MAU::Table* target = dg.get_vertex(dst);
        out << "    " << source->name << " -- " << dep_types(dg.g[*edges]) << " --> " <<
            target->name << std::endl; }


    out << "MIN STAGE INFO" << std::endl;
    out << "    Each table also indicates its dependency chain length" << std::endl;
    out << "    First number refers to a dependency chain from which each tables are not split"
        << std::endl;
    out << "    Second number refers to a dependency chain from which each tables are split as on "
        << " last allocation pass" << std::endl;

    std::map<int, safe_vector<std::pair<const IR::MAU::Table *,
                                        DependencyGraph::StageInfo>>> min_stage_info;
    for (auto &kv : dg.stage_info)
        min_stage_info[kv.second.min_stage].emplace_back(kv.first, kv.second);

    for (auto &msi : min_stage_info) {
        out << " Stage #" << msi.first << std::endl;
        for (auto val : msi.second) {
            out << "     " << val.first->name << " (" << val.first->externalName() << ") "
                << val.second.dep_stages_control_anti << " "
                << val.second.dep_stages_control_anti_split << std::endl;
            if (!dg.display_min_edges)
                continue;
            if (dg.min_stage_edges.count(val.first) == 0)
                continue;
            for (auto edge : dg.min_stage_edges.at(val.first)) {
                out << "\t- Edge " << dep_types(edge.second) << " " << edge.first->name
                    << " ( " << edge.first->externalName() << " )"
                    << std::endl;
            }
        }
    }

    return out;
}

void DependencyGraph::print_dep_type_map(std::ostream &out) const {
  out << "Dependency type map" << std::endl;
  for (auto t1 : dep_type_map) {
    out << "Table " << t1.first->externalName() << ":" << std::endl;
    for (auto t2 : dep_type_map.at(t1.first)) {
      out << "  table " << t2.first->externalName() << " : "
        << TableGraphEdge::labels_to_sub_types.at(dep_type_map.at(t1.first).at(t2.first))
        << std::endl;
        }
    }
}

void DependencyGraph::print_container_access(std::ostream &out) const {
    ordered_set<const IR::MAU::Table*> tbls;
    for (auto t1 : containers_write_) {
        tbls.insert(t1.first); }
    for (auto t1 : containers_read_xbar_) {
        tbls.insert(t1.first); }
    for (auto t1 : containers_read_alu_) {
        tbls.insert(t1.first); }

    for (auto t1 : tbls) {
        out << "Table " << t1->externalName() << std::endl;
        if (containers_write_.find(t1) != containers_write_.end()) {
        out << " writes containers:" << std::endl;
        for (auto c : containers_write_.at(t1)) {
            out << "   " << c.first << std::endl; }
        }
        if (containers_read_xbar_.find(t1) != containers_read_xbar_.end()) {
        out << " reads containers at match input crossbar:" << std::endl;
        for (auto c : containers_read_xbar_.at(t1)) {
            out << "   " << c.first << std::endl; }
        }
        if (containers_read_alu_.find(t1) != containers_read_alu_.end()) {
        out << " reads containers at PHV ALUs:" << std::endl;
        for (auto c : containers_read_alu_.at(t1)) {
            out << "   " << c.first << std::endl; }
        }
    }
}

void DependencyGraph::dump_viz(std::ostream &out, const DependencyGraph &dg) {
    static ordered_map<DependencyGraph::dependencies_t, std::pair<cstring, cstring>> dep_color = {
        { DependencyGraph::IXBAR_READ,
            std::make_pair("ixbar_read"_cs,                 "gold"_cs)},
        { DependencyGraph::ACTION_READ,
            std::make_pair("action_read"_cs,                "blue"_cs)},
        { DependencyGraph::OUTPUT,
            std::make_pair("output"_cs,                     "red"_cs)},
        { DependencyGraph::CONT_CONFLICT,
            std::make_pair("cont_conflict"_cs,              "navy"_cs)},
        { DependencyGraph::REDUCTION_OR_READ,
            std::make_pair("reduction_or_read"_cs,          "cyan"_cs)},
        { DependencyGraph::REDUCTION_OR_OUTPUT,
            std::make_pair("reduction_or_output"_cs,        "cyan"_cs)},
        { DependencyGraph::ANTI_TABLE_READ,
            std::make_pair("anti_table_read"_cs,            "pink"_cs)},
        { DependencyGraph::ANTI_ACTION_READ,
            std::make_pair("anti_next_action_read"_cs,      "pink"_cs)},
        { DependencyGraph::ANTI_NEXT_TABLE_DATA,
            std::make_pair("anti_next_table_data"_cs,       "pink"_cs)},
        { DependencyGraph::ANTI_NEXT_TABLE_CONTROL,
            std::make_pair("anti_next_table_control"_cs,    "pink"_cs)},
        { DependencyGraph::ANTI_NEXT_TABLE_METADATA,
            std::make_pair("anti_next_table_metadata"_cs,   "pink"_cs)},
        { DependencyGraph::ANTI_EXIT,
            std::make_pair("exit"_cs,                       "black"_cs)},
        { DependencyGraph::CONTROL_ACTION,
            std::make_pair("control_action"_cs,             "green"_cs)},
        { DependencyGraph::CONTROL_COND_TRUE,
            std::make_pair("control_condition_true"_cs,     "green"_cs)},
        { DependencyGraph::CONTROL_COND_FALSE,
            std::make_pair("control_condition_false"_cs,    "green"_cs)},
        { DependencyGraph::CONTROL_TABLE_HIT,
            std::make_pair("control_table_hit"_cs,          "green"_cs)},
        { DependencyGraph::CONTROL_TABLE_MISS,
            std::make_pair("control_table_miss"_cs,         "green"_cs)},
        { DependencyGraph::CONTROL_DEFAULT_NEXT_TABLE,
            std::make_pair("control_default_next_table"_cs, "green"_cs)},
        { DependencyGraph::CONTROL_EXIT,
            std::make_pair("control_exit"_cs,               "green"_cs)}
    };

    auto tableName = [](const IR::MAU::Table *tbl) {
        if (tbl) {
            if (tbl->match_table)
                return tbl->match_table->externalName();
            else
                return tbl->name;
        } else {
            return cstring("SINK");
        }
    };

    auto edgeName = [dg](const IR::MAU::Table *source, const IR::MAU::Table *dest) {
        auto depInfo = dg.get_data_dependency_info(source, dest);
        cstring res("");
        if (depInfo == std::nullopt) return res;
        for (auto d : depInfo.value()) {
            // the PHV field name
            res += d.first.first->name + "\n";
        }
        return res;
    };

    auto all_vertices = boost::vertices(dg.g);
    if (++all_vertices.first == all_vertices.second) {
        out << "digraph empty {\n}" << std::endl;
        return;
    }
    ordered_map<std::pair<cstring, cstring>, ordered_set<cstring>> name_pairs;
    ordered_map<int, ordered_set<cstring>> tables_per_stage;
    // order the tables by stage
    for (auto ts : dg.stage_info)
        tables_per_stage[ts.second.min_stage].insert(tableName(ts.first));

    out << "digraph table_deps {" << std::endl
        << "  splines=ortho; rankdir=LR;" << std::endl
        << "  label=\"Program: " << BackendOptions().programName << "\n"
        << "RunId: " << RunId::getId() << "\n\";" << std::endl
        << "  labelloc=t; labeljust=l;" << std::endl;
    // print the root nodes, ranked by the stage
    bool first = true;
    for (auto tables : tables_per_stage) {
        out << "  { ";
        if (!first) out << "rank = same; ";
        else        first = false;
        for (auto t : tables.second) out << "\"" << t << "\"; ";
        out << "}" << std::endl;
    }

    // list the edges
    DependencyGraph::Graph::edge_iterator edges, edges_end;
    for (boost::tie(edges, edges_end) = boost::edges(dg.g); edges != edges_end; ++edges) {
        auto src = boost::source(*edges, dg.g);
        const IR::MAU::Table* source = dg.get_vertex(src);
        auto dst = boost::target(*edges, dg.g);
        const IR::MAU::Table* target = dg.get_vertex(dst);
        auto src_name = tableName(source);
        auto dst_name = tableName(target);
        cstring edge_name = cstring(dep_types(dg.g[*edges]));
        auto p = std::make_pair(src_name, dst_name);
        name_pairs[p].insert(edge_name);
        out << "   \"" << src_name.c_str() << "\" -> \"" <<
            dst_name.c_str() << "\" [ " <<
            "label= \"" << edgeName(source, target) << "\"," <<
            "color=" << dep_color[dg.g[*edges]].second << " ];" << std::endl;
    }

    // Print the legend
    out << "  { rank=max;" << std::endl;
    out << "    subgraph cluster_legend { node [ shape=record; fontsize=10];" << std::endl
        << "      empty [label=<<table border=\"0\" cellborder=\"0\">"
        << "<tr><td colspan=\"8\">Edge colors</td></tr><tr>";
    for (auto c : dep_color)
        out << "<td><font color=\"" << c.second.second << "\">"
            << c.second.first << "</font></td>";
    out << "</tr></table>>;]" << std::endl;
    out << "    }" << std::endl;  // end subgraph
    out << "  }" << std::endl;    // end legend block
    out << "}" << std::endl;      // end digraph

    if (!LOGGING(4))
        return;  // generate other types of graphs only if dumping to console
    out << "digraph table_deps_merged {" << std::endl;
    for (auto& kv : name_pairs) {
        auto table_pair = kv.first;
        auto edges = kv.second;
        out << "    \"" << table_pair.first << "\" -> \"" <<
            table_pair.second << "\" [ label= \"";
        const char* sep = "";
        for (auto& edge : edges) {
            out << sep << edge;
            sep = ",\n";
        }
        out << "\" ];" << std::endl;
    }
    out << "}" << std::endl;
    out << "digraph table_deps_simple {" << std::endl;
    for (auto& kv : name_pairs) {
        auto table_pair = kv.first;
        auto edges = kv.second;
        ordered_set<cstring> simple_edges;
        for (auto& edge : edges) {
            if ((edge == "ANTI_EXIT")
             || (edge == "ANTI_TABLE_READ")
             || (edge == "ANTI_ACTION_READ")
             || (edge == "ANTI_NEXT_TABLE_DATA")
             || (edge == "ANTI_NEXT_TABLE_CONTROL")
             || (edge == "ANTI_NEXT_TABLE_METADATA")) {
                continue;
            } else if (edge == "IXBAR_READ" || edge == "ACTION_READ" || edge == "OUTPUT") {
                simple_edges.insert("DATA"_cs);
            } else if (edge == "REDUCTION_OR_OUTPUT" || edge == "REDUCTION_OR_READ") {
                simple_edges.insert("REDUCTION_OR"_cs);
            } else if ((edge == "CONTROL_ACTION")
             || (edge == "CONTROL_COND_TRUE")
             || (edge == "CONTROL_COND_FALSE")
             || (edge == "CONTROL_TABLE_HIT")
             || (edge == "CONTROL_TABLE_MISS")
             || (edge == "CONTROL_DEFAULT_NEXT_TABLE")
             || (edge == "CONTROL_EXIT")) {
                simple_edges.insert("CONTROL"_cs);
            } else if (edge == "CONT_CONFLICT") {
                simple_edges.insert("CONT_CONFLICT"_cs);
            } else {
                simple_edges.insert("UNKNOWN"_cs);
            }
        }
        out << "    \"" << table_pair.first << "\" -> \"" <<
            table_pair.second << "\" [ label= \"";
        const char* sep = "";
        for (auto& edge : simple_edges) {
            out << sep << edge;
            sep = ",\n";
        }
        cstring color = "black"_cs;
        if (simple_edges.count("DATA"_cs) && simple_edges.count("CONTROL"_cs))
            color = "blue"_cs;
        else if (simple_edges.count("DATA"_cs))
            color = "red"_cs;
        else if (simple_edges.count("CONTROL"_cs))
            color = "green"_cs;
        out << "\",color=" << color << " ];" << std::endl;
    }
    out << "}" << std::endl;
}

/* Return an edge descriptor.  If bool is true, then this is a
 * newly-created edge.  If false, then the edge descriptor points to the
 * edge from src to dst with edge_label that already existed.  */
std::pair<typename DependencyGraph::Graph::edge_descriptor*, bool> DependencyGraph::add_edge(
    const IR::MAU::Table* src,
    const IR::MAU::Table* dst,
    DependencyGraph::dependencies_t edge_label) {
    typename Graph::vertex_descriptor src_v, dst_v;
    src_v = add_vertex(src);
    dst_v = add_vertex(dst);

    DependencyGraph::Graph::edge_descriptor* e = nullptr;
    typename Graph::out_edge_iterator out, end;
    for (boost::tie(out, end) = boost::out_edges(src_v, g); out != end; ++out) {
        if (boost::target(*out, g) == dst_v && g[*out] == edge_label) {
            e = new DependencyGraph::Graph::edge_descriptor(*out);
            return { e, false };
        }
    }

    // Check if back edge present
    if (has_back_edge(src, dst)) {
        LOG7("Cannot add edge " << src->name << " --> " << dst->name
                << " as back-edge is present");
        return { e, false };
    }

    auto maybe_new_e = boost::add_edge(src_v, dst_v, edge_label, g);
    if (!maybe_new_e.second)
        // A vector-based adjacency_list (i.e. Graph) is a multigraph.
        // Inserting edges should always create new edges.
        BUG("Boost Graph Library failed to add edge.");

    // Check if the edge introduced a cycle in the graph
    if (has_cycle()) {
        LOG7("Cannot add edge " << src->name << " --> " << dst->name
                << " as it introduces a cycle");
        boost::remove_edge(maybe_new_e.first, g);
        return { e, false };
    }

    auto p = std::make_pair(dst, src);
    dependency_map.emplace(p, edge_label);
    LOG7("DST " << dst->name << " has dep " << dep_types(edge_label) << " to SRC " << src->name);
    e = new DependencyGraph::Graph::edge_descriptor(maybe_new_e.first);
    return { e, true };
}

TableGraphNode DependencyGraph::create_node(const int id, const IR::MAU::Table *tbl) const {
    TableGraphNode node;
    TableGraphNode::TableGraphNodeTable nodeTable;

    if (!tbl) return node;

    if (tbl->global_id()) {
        node.stage_number = tbl->stage();
        node.logical_id = *tbl->logical_id;
    }

    // Add all types here
    cstring type;
    node.id = id;
    if (auto m = tbl->match_table)
        nodeTable.name = cstring::to_cstring(canon_name(m->externalName()));
    else
        nodeTable.name = tbl->name;
    if (stage_info.count(tbl)) {
        node.min_stage = stage_info.at(tbl).min_stage;
        node.dep_chain = stage_info.at(tbl).dep_stages_control_anti;
    }
    if (tbl->conditional_gateway_only()) {
        type = "condition"_cs;
        nodeTable.condition = tbl->gateway_cond;
    } else {
        type = "match"_cs;
        auto match_type = tbl->get_table_type_string();
        if (match_type == "exact_match")
            nodeTable.match_type = "exact"_cs;
        else if (match_type == "ternary_match")
            nodeTable.match_type = "ternary"_cs;
        else if (match_type == "proxy_hash")
            nodeTable.match_type = "proxy_hash"_cs;
        else if (match_type == "hash_action")
            nodeTable.match_type = "hash_action"_cs;
        else if (tbl->layout.pre_classifier || tbl->layout.alpm)
            nodeTable.match_type = "algorithmic_lpm"_cs;
        else if (match_type == "atcam_match")
            nodeTable.match_type = "algorithmic_tcam"_cs;
    }
    nodeTable.table_type = type;
    node.nodeTables.push_back(nodeTable);

    for (auto att : tbl->attached) {
        auto attMem = att->attached;
        if (!attMem) continue;
        TableGraphNode::TableGraphNodeTable attNode;
        attNode.name = cstring::to_cstring(canon_name(attMem->name));
        if (attMem->to<IR::MAU::Counter>())
            attNode.table_type = "statistics"_cs;
        else if (attMem->to<IR::MAU::Meter>())
            attNode.table_type = "meter"_cs;
        else if (attMem->to<IR::MAU::StatefulAlu>())
            attNode.table_type = "stateful"_cs;
        else if (attMem->to<IR::MAU::Selector>())
            attNode.table_type = "selection"_cs;
        else if (attMem->to<IR::MAU::ActionData>())
            attNode.table_type = "action"_cs;
        else if (attMem->to<IR::MAU::TernaryIndirect>())
            attNode.table_type = "ternary_indirect"_cs;
        else if (attMem->to<IR::MAU::IdleTime>())
            attNode.table_type = "idletime"_cs;
        node.nodeTables.push_back(attNode);
    }

    return node;
}

// For more information on schema check
// compiler-interfaces/schemas/jgf_schema.py
// compiler-interfaces/schemas/table_graph_schema.py
void DependencyGraph::to_json(Util::JsonObject* dgsJson, const FlowGraph &fg,
                              cstring passContext, bool placed) {
    if (!dgsJson) return;
    LOG7("Generating dependency graph json");

    auto all_vertices = boost::vertices(g);
    if (all_vertices.first + 1 == all_vertices.second) {
        LOG7("digraph empty {\n}");
        return;
    }

    auto graphTypes = { INGRESS, EGRESS, GHOST };
    std::vector<Util::JsonObject *> dgJson, mdJson;
    std::vector<Util::JsonArray *> nodesJson, edgesJson;
    for (auto g : graphTypes) {
        (void)g;
        dgJson.push_back(new Util::JsonObject());
        mdJson.push_back(new Util::JsonObject());
        nodesJson.push_back(new Util::JsonArray());
        edgesJson.push_back(new Util::JsonArray());
    }

    // Generate CFG JSON Object
    typedef DependencyGraph::Graph::vertex_iterator vertex_iter;
    std::pair<vertex_iter, vertex_iter> vp;
    int node_id = 0;
    for (vp = all_vertices; vp.first != vp.second; ++vp.first) {
        TableGraphNode node;
        TableGraphNode::TableGraphNodeTable nodeTable;

        DependencyGraph::Graph::vertex_descriptor v = *vp.first;
        auto tbl = get_vertex(v);
        auto gress = static_cast<int>(tbl->gress);

        if (tbl->global_id()) {
            node.stage_number = tbl->stage();
            node.logical_id = *tbl->logical_id;
        }

        // Add all types here
        cstring type;
        node.id = v;
        // Track max node id value
        node_id = (node.id > node_id) ? node.id : node_id;
        if (auto m = tbl->match_table)
            nodeTable.name = cstring::to_cstring(canon_name(m->externalName()));
        else
            nodeTable.name = tbl->name;
        node.min_stage = stage_info.at(tbl).min_stage;
        node.dep_chain = stage_info.at(tbl).dep_stages_control_anti;
        if (tbl->conditional_gateway_only()) {
            type = "condition"_cs;
            nodeTable.condition = tbl->gateway_cond;
        } else {
            type = "match"_cs;
            nodeTable.match_type = TableGraphNode::get_node_match_type(tbl);
        }
        nodeTable.table_type = type;
        node.nodeTables.push_back(nodeTable);

        for (auto att : tbl->attached) {
            auto attMem = att->attached;
            if (!attMem) continue;
            TableGraphNode::TableGraphNodeTable attNode;
            attNode.name = cstring::to_cstring(canon_name(attMem->name));
            attNode.table_type = TableGraphNode::get_attached_table_type(attMem);
            node.nodeTables.push_back(attNode);
        }
        nodesJson[gress]->append(node.create_node_json());
    }

    DependencyGraph::Graph::edge_iterator edges, edges_end;
    int edge_id = 0;

    auto add_json_edge = [&](TableGraphEdge &e, int &id, int g) {
        e.is_critical = is_edge_critical(*edges);
        e.id = id++;
        edgesJson[g]->append(e.create_edge_json());
    };

    for (boost::tie(edges, edges_end) = boost::edges(g); edges != edges_end; ++edges) {
        TableGraphEdge edge;

        edge.source = boost::source(*edges, g);
        edge.target = boost::target(*edges, g);
        const IR::MAU::Table* source = get_vertex(edge.source);
        const IR::MAU::Table* target = get_vertex(edge.target);
        BUG_CHECK(source || target, "Edge has no source and no destination table");
        if (source && target && source->gress != target->gress)
            BUG("Invalid dependency graph edge from %1% (gress = %2%) --> %3% (gress = %4%) ",
                source->name, source->gress, target->name, target->gress);
        auto gress = source ? static_cast<int>(source->gress) : static_cast<int>(target->gress);
        std::string src_name = std::string(source ? source->name : "SOURCE");
        std::string dst_name = std::string(target ? target->name : "SINK");

        edge.label = g[*edges];
        LOG5(src_name.c_str() << " --- " << dep_types(edge.label) << " --> " << dst_name.c_str());
        if (edge.label == DependencyGraph::ANTI_EXIT) {
            if (data_annotations_exit.count(*edges)) {
                for (const auto act : data_annotations_exit.at(*edges)) {
                    edge.exit_action_name = act->name;
                    add_json_edge(edge, edge_id, gress);
                }
                continue;
            }
        } else if (edge.label == DependencyGraph::CONT_CONFLICT) {
            if (data_annotations_conflicts.count(*edges)) {
                auto phv_container = data_annotations_conflicts.at(*edges);
                edge.phv_number = Device::phvSpec().physicalAddress(phv_container, PhvSpec::MAU);
            }
        } else if (edge.label == DependencyGraph::ANTI_NEXT_TABLE_METADATA) {
            if (data_annotations_metadata.count(*edges)) {
                auto md = data_annotations_metadata.at(*edges);
                auto mdf = md->field();
                if (mdf) edge.add_dep_field(md->field());
            }
        } else if ((edge.label == DependencyGraph::OUTPUT)
                || (edge.label == DependencyGraph::IXBAR_READ)
                || is_anti_edge(edge.label)) {
            auto deps = get_data_dependency_info(*edges);
            if (deps) {
                auto local_data = deps.value();
                for (const auto& kv : local_data) {
                    edge.add_dep_field(kv.first);
                }
            }
        } else if (is_ctrl_edge(edge.label)) {
            if (edge.label == DependencyGraph::CONTROL_ACTION) {
                auto edge_data = get_ctrl_dependency_info(*edges);
                   edge.action_name = edge_data.value();
            }
        }

        edge.tags.push_back("dependency"_cs);
        add_json_edge(edge, edge_id, gress);
    }

    // Flow Graph Control Edges
    if (!fg.is_empty()) {
        // FlowGraph::dump_viz(std::cout, fg);
        int source_node_id[3] = { -1, -1, -1 };
        int sink_node_id[3]   = { -1, -1, -1 };
        FlowGraph::Graph::edge_iterator fedges, fedges_end;
        for (boost::tie(fedges, fedges_end) = boost::edges(fg.g); fedges != fedges_end; ++fedges) {
            TableGraphEdge edge;

            auto fsource = boost::source(*fedges, fg.g);
            auto ftarget = boost::target(*fedges, fg.g);
            const IR::MAU::Table* source = fg.get_vertex(fsource);
            const IR::MAU::Table* target = fg.get_vertex(ftarget);
            std::string src_name = std::string(source ? source->name : "SOURCE");
            std::string tgt_name = std::string(target ? target->name : "SINK");
            if (!source && !target)
            BUG_CHECK(source || target, " Invalid dependency graph edge found with no"
                                        " source and target node");

            auto src_gress = source ? source->gress : target->gress;
            auto tgt_gress = target ? target->gress : source->gress;

            if (src_gress != tgt_gress) {
                src_name = source ? source->externalName() : src_name;
                tgt_name = target ? target->externalName() : tgt_name;
                warning(" Invalid gress on flow graph edges while creating"
                    " dependency graph json, source %1%(%2%) and target %3%(%4%)",
                    src_name, toString(src_gress), tgt_name, toString(tgt_gress));
                continue;
            }

            if (labelToVertex.count(source)) {
                edge.source = labelToVertex.at(source);
            } else {
                if (source_node_id[src_gress] < 0) {
                    source = new IR::MAU::Table(src_name, src_gress);
                    auto node = create_node(++node_id, source);
                    nodesJson[static_cast<int>(src_gress)]->append(node.create_node_json());
                    source_node_id[src_gress] = node_id;
                }
                edge.source = source_node_id[src_gress];
            }
            if (labelToVertex.count(target)) {
                edge.target = labelToVertex.at(target);
            } else {
                if (sink_node_id[tgt_gress] < 0) {
                    target = new IR::MAU::Table(tgt_name, tgt_gress);
                    auto node = create_node(++node_id, target);
                    nodesJson[static_cast<int>(tgt_gress)]->append(node.create_node_json());
                    sink_node_id[tgt_gress] = node_id;
                }
                edge.target = sink_node_id[tgt_gress];
            }

            auto edge_data = fg.get_ctrl_dependency_info(*fedges);
            LOG5(src_name << (fsource == fg.v_source ? "_SOURCE" : "")
                << " --- " << " (" << edge_data << ") --> " << tgt_name);

            edge.label = get_control_edge_type(edge_data);
            if (edge.label == DependencyGraph::CONTROL_ACTION) {
               edge.action_name = edge_data;
            }

            auto e = boost::lookup_edge(edge.source, edge.target, g);
            edge.is_critical = e.second;
            edge.id = edge_id++;
            edge.tags.push_back("flow"_cs);
            edgesJson[static_cast<int>(src_gress)]->append(edge.create_edge_json());
        }
    }

    auto table_placement_round = TablePlacement::placement_round;
    if (placed) --table_placement_round;  // counter updates after each table placement
    auto description = passContext + " Round " + std::to_string(table_placement_round);
    for (auto g : { INGRESS, EGRESS, GHOST }) {
        mdJson[g]->emplace("gress"_cs, new Util::JsonValue(toString(g)));
        mdJson[g]->emplace("description"_cs, new Util::JsonValue(description));
        // mdJson[g]->emplace("compile_iteration", /* Additional Pass Info? TBD */);
        mdJson[g]->emplace("placement_complete"_cs, new Util::JsonValue(placed));

        dgJson[g]->emplace("metadata"_cs, mdJson[g]);
        dgJson[g]->emplace("nodes"_cs, nodesJson[g]);
        dgJson[g]->emplace("edges"_cs, edgesJson[g]);
    }

    auto check_and_add_graph = [&](Util::JsonArray* gJson) {
        for (auto g : { INGRESS, EGRESS, GHOST }) {
            if (dgJson[g]->size() > 0) {
                auto nodes = dgJson[g]->get("nodes"_cs)->to<Util::JsonArray>();
                auto edges = dgJson[g]->get("edges"_cs)->to<Util::JsonArray>();
                if (nodes->size() > 0 && edges->size() > 0)
                    gJson->append(dgJson[g]);
            }
        }
    };

    if (dgsJson->count("graphs"_cs)) {
        auto graphs = dgsJson->get("graphs"_cs)->to<Util::JsonArray>();
        check_and_add_graph(graphs);
    } else {
        auto graphs = new Util::JsonArray();
        check_and_add_graph(graphs);
        dgsJson->emplace("graphs"_cs, graphs);
        dgsJson->emplace("schema_version"_cs, "1.0.1"_cs);
    }
}

std::set<cstring> FindDataDependencyGraph::getFieldNameSlice(const PHV::Field *field,
                                                             le_bitrange range) const {
    std::set<cstring> retval;

    if (phv.alloc_done()) {
        field->foreach_alloc([&](const PHV::AllocSlice &sl) {
            if (range.overlaps(sl.field_slice()))
                retval.insert(field->name + sl.field_slice().formatAsSlice(field->size));
        });
    }

    // Used in case PHV Allocation was not completed or this field does not have any AllocSlices
    if (retval.empty()) {
        retval.insert(field->name);
    }
    return retval;
}

/**
 * The reduction or dependencies must be tracked so that all tables that are in the same
 * reduction or group do not have any dependencies between them, but they as a block do
 * have dependencies with any other standard reads or writes.
 *
 * Thus when a reduction or is found, it is established whether or not the previous write
 * was the same reduction or group.  If so, no data dependency is added, but if not, then
 * this table is data dependent with this table.
 */
class FindDataDependencyGraph::AddDependencies : public MauInspector, TofinoWriteContext {
    FindDataDependencyGraph             &self;
    const IR::MAU::Table                *table;
    ordered_map<PHV::Container, ordered_set<write_op_t>>    cont_writes;

 public:
    AddDependencies(FindDataDependencyGraph &self,
                    const IR::MAU::Table *t) :
        self(self), table(t) { }

 private:
    profile_t init_apply(const IR::Node* root) override {
        cont_writes.clear();
        return Inspector::init_apply(root);
    }

    void addDeps(ordered_set<std::pair<const IR::MAU::Table*, const IR::MAU::Action*>> tables,
                 DependencyGraph::dependencies_t dep, const PHV::Field *field) {
        for (auto upstream_t_pair : tables) {
            auto upstream_t = upstream_t_pair.first;
            if (self.ignore.ignore_deps(table, upstream_t)) {
                LOG4("\tIgnoring dependency from " << upstream_t->name << " to " << table->name);
                continue;
            }
            auto new_dep = dep;
            if (dep == DependencyGraph::ANTI_TABLE_READ) {
                if (!upstream_t_pair.second)
                    new_dep = DependencyGraph::ANTI_ACTION_READ;
            }

            if (self.dg.has_back_edge(upstream_t, table)) {
                LOG7("\tCannot add edge between " << upstream_t->name
                        << " and " << table->name << " as a backedge already exists");
                continue;
            }

            // Handle the case of incorrect ARA-->ARA dependencies
            // which are added due to depth-first traversal of the
            // control flow visitor
            if (upstream_t->is_always_run_action() && table->is_always_run_action() &&
                (upstream_t->stage() > table->stage())) {
                LOG7("\t addDeps: Will not add edge " << dep_types(dep) << " between tables " <<
                     upstream_t->name << " and " << table->name);
                continue;
            }

            // Detached attached tables should be control/data independent.
            if (((upstream_t->is_detached_attached_tbl) || (table->is_detached_attached_tbl))) {
                LOG7("\t addDeps: Will not add edge " << dep_types(dep) << " between tables " <<
                     upstream_t->name << " and " << table->name);
                continue;
            }

            auto edge_pair = self.dg.add_edge(upstream_t, table, new_dep);
            if (!edge_pair.first) continue;
            const IR::MAU::Action *action_use_context = findContext<IR::MAU::Action>();
            if (upstream_t_pair.second) {
                LOG5("\tAdd " << dep_types(dep) << " dependency from " << upstream_t->name
                        << " to " << table->name << " because of field " << field->name <<
                        " for actions " << upstream_t_pair.second->name << " - " <<
                        (action_use_context ? action_use_context->name.toString() : "nullptr"));
            } else {
                LOG5("\tAdd " << dep_types(dep) << " dependency from " << upstream_t->name
                        << " to " << table->name << " because of field " << field->name);
            }
            self.dg.data_annotations[*edge_pair.first][field].first.insert(upstream_t_pair.second);
            self.dg.data_annotations[*edge_pair.first][field].second.insert(action_use_context);
        }
    }

    void addContDeps(const cont_write_t &tables, const ordered_set<write_op_t>& writes,
                     const PHV::Container container) {
        const auto merge_range = [](const ordered_set<write_op_t> &writes) {
            bitvec rst;
            for (const auto &w : writes) {
                rst |= w.second;
            }
            return rst;
        };

        if (LOGGING(7)) {
            LOG7("-------------------------------------------------------------");
            LOG7("\tCurrent Table Writes");
            for (auto f : writes)
                LOG7("\t\t" << f.first << " , " << f.second);
            LOG7("\tAll Table Writes: ");
            for (auto t : tables) {
                LOG7("\t\tTable: " << t.first->name);
                for (auto f : t.second)
                    LOG7("\t\t\t" << f.first << " , " << f.second);
            }
            LOG7("\tChecking Mutex Fields");
            for (auto f1 : writes) {
                auto f1F = f1.first.field();
                for (auto t : tables) {
                    for (auto f2 : t.second) {
                        auto f2F = f2.first.field();
                        LOG7("\t\t( " << f1F << " , " << f2F << " ) = FeldMutex "
                                << self.phv.isFieldMutex(f1F, f2F)
                                << ", MetdataMutex " << self.phv.isMetadataMutex(f1F, f2F));
                    }
                }
            }
            LOG7("-------------------------------------------------------------");
        }

        for (auto upstream_t : tables) {
            auto upstream_range = merge_range(upstream_t.second);
            auto range = merge_range(writes);
            if (self.ignore.ignore_deps(table, upstream_t.first)) {
                WARN_CHECK(upstream_range == range, BFN::ErrorType::WARN_PRAGMA_USE,
                           "Table %1%: pragma ignore_table_dependency "
                           "of %2% is also ignoring PHV added action dependencies over container "
                           "%3%, which may not have been the desired outcome", table,
                            upstream_t.first->name, container.toString());
                continue;
            }

            // Skip adding container conflict between tables if all fields between them are mutually
            // exclusive.
            for (auto uw : upstream_t.second) {
                auto uf = uw.first.field();
                for (auto w : writes) {
                    auto f = w.first.field();
                    // TBD: Checking for metadata mutex along with field muted fails
                    // if (self.phv.isFieldMutex(uf, f) || self.phv.isMetadataMutex(uf, f)) {
                    if (self.phv.isFieldMutex(uf, f)) {
                        LOG1("\t Skipping container conflict (container "
                                << container << ") between tables as ( "
                                << table->name << " and " << upstream_t.first->name
                                << " ) as fields ( " << f << " and " << uf
                                << " are mutually exclusive");
                    } else {
                        // TBD: Container conflict edges if added to the dependency graph
                        // should account for cases when table placement runs without
                        // container conflicts.
                        // auto edge_pair = self.dg.add_edge(upstream_t.first, table,
                        //                                   DependencyGraph::CONT_CONFLICT);
                        // self.dg.data_annotations_conflicts.emplace(edge_pair.first, container);
                        LOG1("\tAdd container conflict between table " << upstream_t.first->name <<
                                " and table " << table->name << " because of container " <<
                                container);
                        self.dg.container_conflicts[upstream_t.first].insert(table);
                        self.dg.container_conflicts[table].insert(upstream_t.first);
                    }
                }
            }

            // any_unavoidable returns true only if there exist two writes to two
            // fieldslices that must be allocated to the same container because of PARDE
            // constraints, from two tables.
            const auto any_unavoidable = [&]() {
                for (const auto &upstream_write : upstream_t.second) {
                    for (const auto &table_write : writes) {
                        if (self.phv.must_alloc_same_container(upstream_write.first,
                                                               table_write.first)) {
                            return true;
                        }
                    }
                }
                return false;
            };
            if (any_unavoidable()) {
                LOG1("\tAdd unavoidable container conflict between table " << upstream_t.first->name
                     << " and table " << table->name << " because of container " << container);
                self.dg.unavoidable_container_conflicts[upstream_t.first].insert(table);
                self.dg.unavoidable_container_conflicts[table].insert(upstream_t.first);
            }
        }
    }

    bool preorder(const IR::Expression *e) override {
        le_bitrange range;
        auto* originalField = self.phv.field(e, &range);
        if (!originalField) return true;
        ordered_set<const PHV::Field*> candidateFields;
        candidateFields.insert(originalField);
        if (self.phv.getAliasMap().count(originalField))
            candidateFields.insert(self.phv.getAliasMap().at(originalField));
        for (const PHV::Field* field : candidateFields) {
            cstring red_or_key;
            bool is_red_or = self.dg.red_info.is_reduction_or(findContext<IR::MAU::Instruction>(),
                                                              table, red_or_key);
            bool non_first_write_red_or = false;

            std::set<cstring> field_slice_set = self.getFieldNameSlice(field, range);
            for (const cstring &field_name : field_slice_set) {
                if (self.access.count(field_name)) {
                    LOG6("\t\tadd_dependency(" << field_name << ") ");
                    // AlwaysRun Actions (ARA)
                    //   1) either initialize to zero or
                    //   2) move between PHVs.
                    // In either case the write operand of
                    // the ARA is not dependent to prior accesses because
                    //   1) the zero-init is the first access of this field or
                    //   2) the read of the move is dependent to prior accesses.
                    if (isWrite()) {
                        if (table->is_always_run_action() || (table->is_detached_attached_tbl)) {
                            LOG7("\t Skipping dependencies for write expression " << e <<
                                 " of ARA " << table->name);
                        } else {
                            // Write-after-read dependence.
                            addDeps(self.access[field_name].ixbar_read,
                                    DependencyGraph::ANTI_TABLE_READ, field);
                            addDeps(self.access[field_name].action_read,
                                    DependencyGraph::ANTI_ACTION_READ, field);
                            // Write-after-write dependence.
                            if (is_red_or) {
                                auto pos = self.red_or_use.find(field_name);
                                // If reduction_or, and the previous write was a reduction_or
                                if (pos != self.red_or_use.end()) {
                                    ERROR_CHECK(pos->second == red_or_key, "%s: The reduction_or "
                                                "groups collide on field %s, over group %s and "
                                                "group %s", table->srcInfo, field_name, red_or_key,
                                                pos->second);
                                    addDeps(self.access[field_name].reduction_or_write,
                                            DependencyGraph::REDUCTION_OR_OUTPUT, field);
                                    non_first_write_red_or = true;
                                } else {
                                    // If reduction_or and previous write was not reduction_or
                                    addDeps(self.access[field_name].write, DependencyGraph::OUTPUT,
                                            field);
                                }
                            } else {
                                // If normal
                                addDeps(self.access[field_name].write, DependencyGraph::OUTPUT,
                                        field);
                            }
                        }
                    } else {
                        // Read-after-write dependence.
                        if (isIxbarRead()) {
                            addDeps(self.access[field_name].write,
                                    DependencyGraph::IXBAR_READ, field);
                        } else if (is_red_or) {
                            // If reduction_or, and the previous write was a reduction_or
                            auto pos = self.red_or_use.find(field_name);
                            if (pos != self.red_or_use.end()) {
                                ERROR_CHECK(pos->second == red_or_key, "%s: "
                                "The reduction_or groups collide on field %s, over group %s and "
                                "group %s", table->srcInfo, field_name, red_or_key, pos->second);
                                addDeps(self.access[field_name].reduction_or_write,
                                        DependencyGraph::REDUCTION_OR_READ, field);
                            } else {
                                // If reduction_or, and the previous write was normal
                                addDeps(self.access[field_name].write, DependencyGraph::ACTION_READ,
                                        field);
                            }
                        } else {
                            // If normal
                            addDeps(self.access[field_name].write,
                                    DependencyGraph::ACTION_READ, field);
                        }
                    }
                }
            }
            if (isWrite() && !non_first_write_red_or && self.phv.alloc_done()) {
                /// FIXME(cc): Do we need to restrict the context here, or is it always the
                /// whole pipeline?
                // Consider actual field slices instead of entire fields when calculating
                // container conflicts.
                static PHV::FieldUse WRITE(PHV::FieldUse::WRITE);
                field->foreach_alloc(range, table, &WRITE, [&](const PHV::AllocSlice &sl) {
                    bitvec cont_range(sl.container_slice().lo, sl.width());
                    LOG5("Adding container write: " << sl.container() << " = Field: " <<
                            sl.field()->name << ", slice: " << sl.field_slice() << ", range: " <<
                            cont_range);
                    cont_writes[sl.container()].insert(
                        {PHV::FieldSlice(sl.field(), sl.field_slice()), cont_range});
                });
            }
        }
        return false;
    }

    bool preorder(const IR::Annotation *) override { return false; }

    bool preorder(const IR::MAU::StatefulCall *) override {
        return false;
    }

    bool preorder(const IR::MAU::Instruction *) override {
        return true;
    }

    void end_apply() override {
        // cont_writes: Container -> Field1, FIeld2, Field3
        // cont_write: Container -> { Table , FieldSlice }
        for (auto entry : cont_writes) {
            LOG5("Add container dependencies for entry " << entry.first);
            addContDeps(self.cont_write[entry.first], entry.second, entry.first);
        }
        for (auto& edge_map_pair : self.dg.data_annotations) {
            for (auto& kv : edge_map_pair.second) {
                kv.second.first.erase(nullptr);
                kv.second.second.erase(nullptr);
            }
        }
    }
};

class FindDataDependencyGraph::UpdateAccess : public MauInspector , TofinoWriteContext {
    FindDataDependencyGraph                &self;
    const IR::MAU::Table                   *table;
    bool gateway_context = false;
    ordered_map<PHV::Container, ordered_set<write_op_t>>    cont_writes;

 public:
    UpdateAccess(FindDataDependencyGraph &self, const IR::MAU::Table *t) : self(self), table(t) {}
    UpdateAccess(FindDataDependencyGraph &self, const IR::MAU::Table *t, bool gw_ctxt) :
      self(self), table(t), gateway_context(gw_ctxt) {}

    profile_t init_apply(const IR::Node* root) override {
        cont_writes.clear();
        return Inspector::init_apply(root);
    }

    // Don't track dependencies of items from
    // attached tables left as primitives by exploring
    // via the primitive. Instead track them by exploring
    // the attached tables directly.
    bool preorder(const IR::MAU::StatefulCall *) override {
        return false;
    }

    // We do still want to explore instructions.
    bool preorder(const IR::MAU::Instruction *) override {
        return true;
    }

    bool preorder(const IR::Expression *e) override {
        le_bitrange range;
        auto* originalField = self.phv.field(e, &range);
        if (!originalField) {
          return true; }
        ordered_set<const PHV::Field*> candidateFields;
        candidateFields.insert(originalField);
        if (self.phv.getAliasMap().count(originalField))
            candidateFields.insert(self.phv.getAliasMap().at(originalField));

        // This will often be null for expressions that don't appear within actions
        const IR::MAU::Action *action_context = findContext<IR::MAU::Action>();

        for (const PHV::Field* field : candidateFields) {
            cstring red_or_key;

            std::set<cstring> field_slice_set = self.getFieldNameSlice(field, range);
            for (const cstring &field_name : field_slice_set) {
                auto &a = self.access[field_name];
                if (isWrite()) {
                    LOG6("\t\tupdate_access write " << field_name);
                    a.ixbar_read.clear();
                    a.action_read.clear();
                    a.reduction_or_write.clear();
                    a.reduction_or_read.clear();
                    a.write.clear();
                    bool is_red_or = self.dg.red_info.is_reduction_or(
                                                    findContext<IR::MAU::Instruction>(),
                                                    table, red_or_key);
                    if (is_red_or) {
                        a.reduction_or_write.insert(std::make_pair(table, action_context));
                        self.red_or_use[field_name] = red_or_key;
                    } else if (self.red_or_use.count(field_name)) {
                        self.red_or_use.erase(self.red_or_use.find(field_name));
                    }
                    a.write.insert(std::make_pair(table, action_context));
                } else {
                    LOG6("\t\tupdate_access read " << field_name);
                    if (isIxbarRead()) {
                        a.ixbar_read.insert(std::make_pair(table, action_context));
                    } else {
                        bool is_red_or = self.dg.red_info.is_reduction_or(
                                                        findContext<IR::MAU::Instruction>(),
                                                        table, red_or_key);
                        if (is_red_or) {
                            a.reduction_or_read.insert(std::make_pair(table, action_context));
                        }
                        a.action_read.insert(std::make_pair(table, action_context));
                    }
                }
            }

            if (self.phv.alloc_done()) {
              static PHV::FieldUse READ(PHV::FieldUse::READ);
              static PHV::FieldUse WRITE(PHV::FieldUse::WRITE);
              if (isWrite()) {
                  /// FIXME(cc): Do we need to restrict the context here, or is it always the
                  /// whole pipeline?
                  field->foreach_alloc(range, table, &WRITE, [&](const PHV::AllocSlice &sl) {
                      bitvec cont_range(sl.container_slice().lo, sl.width());
                      cont_writes[sl.container()].insert(
                          {PHV::FieldSlice(sl.field(), sl.field_slice()), cont_range});
                      self.dg.containers_write_[table][sl.container()] = true;
                  });
              }
              if (isIxbarRead() || gateway_context) {
                field->foreach_alloc(range, table, &READ, [&](const PHV::AllocSlice &sl) {
                    self.dg.containers_read_xbar_[table][sl.container()] = true;
                });
              } else if (isRead()) {
                field->foreach_alloc(range, table, &READ, [&](const PHV::AllocSlice &sl) {
                    self.dg.containers_read_alu_[table][sl.container()] = true;
                });
              }
            }
        }
        return false;
    }

    void end_apply() override {
        for (auto entry : cont_writes) {
            auto &cw = self.cont_write[entry.first];
            cw.emplace(table, entry.second);
        }
    }
};

bool FindDataDependencyGraph::preorder(const IR::MAU::TableSeq * seq ) {
    visitAgain();
    const Context *ctxt = getContext();
    LOG5("\t TableSeq (" << seq->size() << ")   front: " <<
         (seq->front() ? seq->front()->name : "null") << " back: " <<
         (seq->back() ? seq->back()->name : "null"));

    if (ctxt && ctxt->node->is<IR::BFN::Pipe>()) {
        LOG5("\t Hit Top of pipe - clearing access");
        access.clear();
    }

    return true;
}

/**
 * Due to an IR::MAU::Table object appearing in multiple TableSeq objects, it is now
 * important to include a visitAgain function call in all Table objects.
 *
 * Examine the following program:
 *
 *     switch(t1.apply().action_run) {
 *         a1 : { t2.apply(); t3.apply(); t4.apply(); }
 *         a2 : { t5.apply(); t3.apply(); t4.apply(); }
 *     }
 *
 * Two TableSeq objects [ t2, t3, t4 ] and [ t5, t3, t4 ] are created.  When the algorithm
 * runs, t3 and its associated effects needs to be run on both of these sequences, and not
 * visited in only one of the sequence.  Thus visitAgain is now necessary
 */
bool FindDataDependencyGraph::preorder(const IR::MAU::Table *t) {
    visitAgain();
    LOG5("\tFindDep table " << t->name);

    // Add this table as a vertex in the dependency graph if it's not
    // already there.
    dg.add_vertex(t);

    // Add data dependences induced by gateways, matches, and actions.
    for (auto &gw : t->gateway_rows)
        if (gw.first) gw.first->apply(AddDependencies(*this, t));
    for (auto ixbar_read : t->match_key)
        ixbar_read->apply(AddDependencies(*this, t));
    for (auto &action : Values(t->actions))
        action->apply(AddDependencies(*this, t));
    for (auto &at : t->attached)
        at->apply(AddDependencies(*this, t));

    // Mark fields read/written by this table in accesses.
    // FIXME: Should have a separate gateway row IR to visit rather than other information
    for (auto &gw : t->gateway_rows)
        if (gw.first) gw.first->apply(UpdateAccess(*this, t, true));

    // FIXME: Need to have this as part of the visitors on Actions, rather than on Attached
    // Tables, but these visitor information really needs to be cleaned up.
    for (auto &at : t->attached)
        at->apply(UpdateAccess(*this, t));

    return true;
}

bool FindDataDependencyGraph::preorder(const IR::MAU::TableKey *read) {
    visitAgain();
    auto tbl = findContext<IR::MAU::Table>();
    LOG5("\t TableKey for table " << (tbl ? tbl->name : "null"));
    read->apply(UpdateAccess(*this, tbl));
    return false;
}

bool FindDataDependencyGraph::preorder(const IR::MAU::Action *act) {
    visitAgain();
    auto tbl = findContext<IR::MAU::Table>();
    LOG5("\t Action for table " << (tbl ? tbl->name : "null"));
    act->apply(UpdateAccess(*this, tbl));
    return false;
}

template<class T>
inline ordered_set<T> &operator |=(ordered_set<T> &s, const ordered_set<T> &a) {
    s.insert(a.begin(), a.end());
    return s; }

void FindDataDependencyGraph::flow_merge(Visitor &v) {
    for (auto &a : dynamic_cast<FindDataDependencyGraph &>(v).access) {
        access[a.first].ixbar_read |= a.second.ixbar_read;
        access[a.first].action_read |= a.second.action_read;
        access[a.first].write |= a.second.write;
        access[a.first].reduction_or_write |= a.second.reduction_or_write;
        access[a.first].reduction_or_read |= a.second.reduction_or_read;
    }

    for (auto &r : dynamic_cast<FindDataDependencyGraph &>(v).red_or_use) {
        red_or_use[r.first] = r.second;
    }

    for (auto &cw : dynamic_cast<FindDataDependencyGraph &>(v).cont_write) {
        for (auto entry : cw.second) {
            cont_write[cw.first][entry.first].insert(entry.second.begin(), entry.second.end());
        }
    }
}
void FindDataDependencyGraph::flow_copy(::ControlFlowVisitor &v_) {
    auto &v = dynamic_cast<FindDataDependencyGraph &>(v_);
    access = v.access;
    red_or_use = v.red_or_use;
    cont_write = v.cont_write;
}

void DependencyGraph::fill_dep_stages_from_topo(
        const std::vector<ordered_set<DependencyGraph::Graph::vertex_descriptor>> &topo,
        bool include_stages, const TableSummary *summary) {
    for (int i = int(topo.size()) - 1; i >= 0; --i) {
        for (const auto& vertex : topo[i]) {
            const IR::MAU::Table* table = get_vertex(vertex);
            auto& happens_later = happens_before_work_map[table];
            int tbl_dep_stages =
                std::accumulate(happens_later.begin(), happens_later.end(), 0,
                        [&] (int sz, const IR::MAU::Table* later) {
                    int stage_addition = 0;
                    if (dep_type_map.count(table) && dep_type_map.at(table).count(later)
                        && (!(is_ctrl_edge(dep_type_map.at(table).at(later))) ||
                            later->getAnnotation("separate_gateway"_cs))
                        && dep_type_map.at(table).at(later) != DependencyGraph::CONT_CONFLICT
                        && dep_type_map.at(table).at(later) != DependencyGraph::REDUCTION_OR_READ
                        && dep_type_map.at(table).at(later) != DependencyGraph::REDUCTION_OR_OUTPUT
                        && !(is_anti_edge(dep_type_map.at(table).at(later)))) {
                        if (include_stages && summary)
                            stage_addition = summary->stages(table, true).size();
                        if (!stage_addition)
                            stage_addition = 1;
                    }
                    if (include_stages)
                        stage_addition += stage_info[later].dep_stages_control_anti_split;
                    else
                        stage_addition += stage_info[later].dep_stages_control_anti;

                    return std::max(sz, stage_addition);
            });
            if (include_stages)
                stage_info[table].dep_stages_control_anti_split = tbl_dep_stages;
            else
                stage_info[table].dep_stages_control_anti = tbl_dep_stages;
            stage_info[table].max_stage =
                Device::numStages() - 1 - stage_info[table].dep_stages_control_anti_split;
        }
    }
}

bool DependencyGraph::is_anti_edge(DependencyGraph::dependencies_t dep) const {
    return (dep == DependencyGraph::ANTI_EXIT
        || dep == DependencyGraph::ANTI_TABLE_READ
        || dep == DependencyGraph::ANTI_ACTION_READ
        || dep == DependencyGraph::ANTI_NEXT_TABLE_DATA
        || dep == DependencyGraph::ANTI_NEXT_TABLE_METADATA
        || dep == DependencyGraph::ANTI_NEXT_TABLE_CONTROL);
}

bool DependencyGraph::is_ctrl_edge(DependencyGraph::dependencies_t dep) const {
    return (dep == DependencyGraph::CONTROL_ACTION
        || dep == DependencyGraph::CONTROL_COND_TRUE
        || dep == DependencyGraph::CONTROL_COND_FALSE
        || dep == DependencyGraph::CONTROL_TABLE_HIT
        || dep == DependencyGraph::CONTROL_TABLE_MISS
        || dep == DependencyGraph::CONTROL_DEFAULT_NEXT_TABLE
        || dep == DependencyGraph::CONTROL_EXIT);
}

bool DependencyGraph::is_non_directional_edge(DependencyGraph::dependencies_t dep) const {
    return (dep == DependencyGraph::CONT_CONFLICT
        || dep == DependencyGraph::REDUCTION_OR_READ
        || dep == DependencyGraph::REDUCTION_OR_OUTPUT);
}

bool DependencyGraph::is_edge_critical(typename Graph::edge_descriptor e) const {
    auto source = get_vertex(boost::source(e, g));
    auto target = get_vertex(boost::target(e, g));
    if (min_stage_edges.count(target) == 0) return false;
    for (auto s : min_stage_edges.at(target)) {
        if ((s.first == source) && (s.second == g[e])) {
            return true;
        }
    }
    return false;
}

std::optional<ordered_map<
    std::pair<const PHV::Field *, DependencyGraph::dependencies_t>,
    std::pair<ordered_set<const IR::MAU::Action *>, ordered_set<const IR::MAU::Action *>>>>
DependencyGraph::get_data_dependency_info(const IR::MAU::Table *upstream,
                                          const IR::MAU::Table *downstream) const {
    if (!labelToVertex.count(upstream)) {
        LOG4("Upstream vertex " << upstream->name << " not found in graph");
        return std::nullopt;
    }
    if (!labelToVertex.count(downstream)) {
        LOG4("Downstream vertex " << downstream->name << "not found in graph");
        return std::nullopt;
    }
    auto upstream_v = labelToVertex.at(upstream);
    typename Graph::out_edge_iterator out, end;
    ordered_map<
        std::pair<const PHV::Field *, DependencyGraph::dependencies_t>,
        std::pair<ordered_set<const IR::MAU::Action *>, ordered_set<const IR::MAU::Action *>>>
        gathered_data;
    bool found_downstream = false;
    for (boost::tie(out, end) = boost::out_edges(upstream_v, g); out != end; ++out) {
        const IR::MAU::Table *test_v = get_vertex(boost::target(*out, g));
        if (test_v == downstream && (!is_ctrl_edge(g[*out]))) {
            found_downstream = true;
            auto edge_type = g[*out];
            auto local_data_opt = get_data_dependency_info(*out);
            if (!local_data_opt) return std::nullopt;
            auto local_data = local_data_opt.value();
            for (const auto &kv : local_data) {
                gathered_data[{kv.first, edge_type}].first |= local_data[kv.first].first;
                gathered_data[{kv.first, edge_type}].second |= local_data[kv.first].second;
            }
        }
    }
    if (!found_downstream) {
        LOG4("Edge not found between tables " << upstream->name << ", " << downstream->name);
        return std::nullopt;
    }
    return gathered_data;
}

DependencyGraph::mau_dependencies_t DependencyGraph::find_mau_dependency(
        const IR::MAU::Table *from, const IR::MAU::Table *to) {
    check_finalized();
    std::pair<const IR::MAU::Table *, const IR::MAU::Table *> p = std::make_pair(from, to);
    if (table_dep_.find(p) != table_dep_.end()) return table_dep_.at(p);
    if (from && to && from->gress == to->gress) {
        if (containers_write_.find(to) != containers_write_.end()) {
            // check if containers read at xbar = match dependency
            if (containers_read_xbar_.find(from) != containers_read_xbar_.end()) {
                for (auto c : containers_write_.at(to)) {
                    for (auto c2 : containers_read_xbar_.at(from)) {
                        if (c == c2) {
                            table_dep_.emplace(p, MAU_DEP_MATCH);
                            return MAU_DEP_MATCH;
                        }
                    }
                }
            }
            // check if containers read at PHV ALUs = action dependency
            if (containers_read_alu_.find(from) != containers_read_alu_.end()) {
                for (auto c : containers_write_.at(to)) {
                    for (auto c2 : containers_read_alu_.at(from)) {
                        if (c == c2) {
                            table_dep_.emplace(p, MAU_DEP_ACTION);
                            return MAU_DEP_ACTION;
                        }
                    }
                }
            }
            // check if containers written at PHV ALUs = action dependency
            if (containers_write_.find(from) != containers_write_.end()) {
                for (auto c : containers_write_.at(to)) {
                    for (auto c2 : containers_write_.at(from)) {
                        if (c == c2) {
                            table_dep_.emplace(p, MAU_DEP_ACTION);
                            return MAU_DEP_ACTION;
                        }
                    }
                }
            }
        }
    }
    table_dep_.emplace(p, MAU_DEP_CONCURRENT);
    return MAU_DEP_CONCURRENT;
}

// static
cstring DependencyGraph::dep_to_name(dependencies_t dep) {
    if ((dep == DependencyGraph::CONTROL) || (dep == DependencyGraph::ANTI_NEXT_TABLE_CONTROL) ||
        (dep == DependencyGraph::CONTROL_ACTION) || (dep == DependencyGraph::CONTROL_COND_TRUE) ||
        (dep == DependencyGraph::CONTROL_COND_FALSE) ||
        (dep == DependencyGraph::CONTROL_TABLE_HIT) ||
        (dep == DependencyGraph::CONTROL_TABLE_MISS) ||
        (dep == DependencyGraph::CONTROL_DEFAULT_NEXT_TABLE) ||
        (dep == DependencyGraph::CONTROL_EXIT)) {
        return "control"_cs;
    } else if (dep == IXBAR_READ) {
        return "ixbar_read"_cs;
    } else if (dep == ACTION_READ) {
        return "action_read"_cs;
    } else if ((dep == DependencyGraph::ANTI_EXIT) || (dep == DependencyGraph::ANTI_TABLE_READ) ||
               (dep == DependencyGraph::ANTI_ACTION_READ) ||
               (dep == DependencyGraph::ANTI_NEXT_TABLE_DATA) ||
               (dep == DependencyGraph::ANTI_NEXT_TABLE_METADATA)) {
        return "anti"_cs;
    } else if (dep == OUTPUT) {
        return "output"_cs;
    } else {
        return "concurrent"_cs;
    }
}

// static
DependencyGraph::dependencies_t DependencyGraph::get_control_edge_type(cstring annot) {
    if (annot == "$hit")
        return DependencyGraph::CONTROL_TABLE_HIT;
    else if (annot == "$miss" || annot == "$try_next_stage")
        return DependencyGraph::CONTROL_TABLE_MISS;
    else if (annot == "$default")
        return DependencyGraph::CONTROL_DEFAULT_NEXT_TABLE;
    else if (annot == "$true")
        return DependencyGraph::CONTROL_COND_TRUE;
    else if (annot == "$false")
        return DependencyGraph::CONTROL_COND_FALSE;
    else if (annot == "exit")
        return DependencyGraph::CONTROL_EXIT;
    else
        return DependencyGraph::CONTROL_ACTION;
}

/** Topological Sorting Algorithm, but
 *  return a vector of set of vertices, where,
 *  the index of the vector represent the min_stage of vertices in that set.
 *  i.e., generations of vertices sorted by the stage#.
 *
 *  Example:
 *  rst[0] = {A, B}
 *  rst[1] = {C, D}
 *  means that A, B could be in stage#0 and C, B could be in at least state#1.
 *
 * FIXME -- the above comment appears to be completely wrong -- the indexes have
 * nothing to do with the possible stages tables can be placed in.  Instead, the
 * indexes appear to be the depth of the table in the table dependency graph.  The
 * resulting vector is generally used to do a breadth-first traversal of the tables
 */
std::vector<ordered_set<DependencyGraph::Graph::vertex_descriptor>>
FindDependencyGraph::calc_topological_stage(unsigned dep_flags,
        DependencyGraph *local_dg) {
    typename DependencyGraph::Graph::vertex_iterator v, v_end;
    typename DependencyGraph::Graph::edge_iterator out, out_end;

    bool include_control = dep_flags & DependencyGraph::CONTROL;
    bool include_anti = dep_flags & DependencyGraph::ANTI;

    // Current in-degree of vertices
    ordered_map<DependencyGraph::Graph::vertex_descriptor, int> n_depending_on;
    ordered_map<DependencyGraph::Graph::vertex_descriptor,
        std::set<DependencyGraph::Graph::edge_descriptor>> n_depending_on_with_edges;

    // Build initial n_depending_on, and happens_after_work_map
    const auto& dep_graph = local_dg ? local_dg->g : dg.g;
    auto &curr_dg = local_dg ? *local_dg : dg;

    if (curr_dg.has_cycle())
        LOG7("The graph has a cycle");

    std::vector<const IR::MAU::Table*> id_to_table;
    std::unordered_map<const IR::MAU::Table *, int> table_to_id;
    ordered_map<const IR::MAU::Table*, bitvec> happens_after_work_map;

    curr_dg.happens_after_work_map.clear();
    curr_dg.happens_before_work_map.clear();

    for (boost::tie(v, v_end) = boost::vertices(dep_graph); v != v_end; ++v) {
        n_depending_on[*v] = 0;

        const IR::MAU::Table* label_table = curr_dg.get_vertex(*v);
        if (!table_to_id.count(label_table)) {
            int new_id = table_to_id.size();
            table_to_id[label_table] = new_id;
            id_to_table.push_back(label_table);
        }
        happens_after_work_map[label_table] = {};
        curr_dg.happens_after_work_map[label_table] = {};
        curr_dg.happens_before_work_map[label_table] = {};
    }

    for (boost::tie(out, out_end) = boost::edges(dep_graph); out != out_end; ++out) {
        auto dep = dep_graph[*out];
        auto vertex_later = boost::target(*out, dep_graph);
        const auto* table_later = curr_dg.get_vertex(vertex_later);
        if ((include_anti || !(dg.is_anti_edge(dep)))
            && (include_control || !(dg.is_ctrl_edge(dep)) ||
                table_later->getAnnotation("separate_gateway"_cs))
            && dep != DependencyGraph::CONT_CONFLICT
            && dep != DependencyGraph::REDUCTION_OR_OUTPUT
            && dep != DependencyGraph::REDUCTION_OR_READ) {
            auto dst = boost::target(*out, dep_graph);
            n_depending_on[dst]++;
            n_depending_on_with_edges[dst].insert(*out);
        }
    }

    std::vector<ordered_set<DependencyGraph::Graph::vertex_descriptor>> rst;
    ordered_set<DependencyGraph::Graph::vertex_descriptor> processed;
    while (processed.size() < num_vertices(dep_graph)) {
        ordered_set<DependencyGraph::Graph::vertex_descriptor> this_generation;
        // Select vertices of those current in-degree is 0 as members of this_generation.
        // as long as they have not been processed yet.
        LOG5("\t New while iteration over num_vertices - processed size: " <<
            processed.size() << "   graph vertices: " << num_vertices(dep_graph));

        for (auto& kv : n_depending_on) {
            if (!processed.count(kv.first) && kv.second == 0) {
                this_generation.insert(kv.first);
                LOG5("\t\t Non processed vertex: " << curr_dg.get_vertex(kv.first)->name <<
                     " with " << kv.second << " incoming deps");
            } else if (!processed.count(kv.first) && kv.second) {
                LOG5("\t\t Non processed vertex: " << curr_dg.get_vertex(kv.first)->name <<
                     " with " << kv.second << " incoming deps");
            } else {
                LOG5("\t\t Processed vertex: " << curr_dg.get_vertex(kv.first)->name <<
                     " with " << kv.second << " incoming deps");
            }
        }

        // There are no remaining vertices, so it must be a loop.
        if (this_generation.size() == 0) {
            for (boost::tie(v, v_end) = boost::vertices(dep_graph);
                 v != v_end;
                 ++v) {
                if (processed.count(*v) == 0) {
                    LOG5("Unprocessed vertex " << curr_dg.get_vertex(*v)->name <<
                         " with " << n_depending_on[*v] << " incoming deps");
                    if (n_depending_on_with_edges.count(*v)) {
                        auto back_edges = n_depending_on_with_edges[*v];
                        for (auto be : back_edges) {
                            auto be_src = boost::source(be, dep_graph);
                            const auto* be_src_tbl = curr_dg.get_vertex(be_src);
                            LOG5("  Possible back edge for vertex to "
                                << be_src_tbl->name << " of type " << dep_graph[be]);
                        }
                    }
                }
            }
            LOG5(dg);
            BUG("There is a loop in the table dependency graph.");
            break;
        }

        // Remove out-edge destination of these vertices.
        for (auto& v : this_generation) {
            auto out_edge_itr_pair = out_edges(v, dep_graph);
            auto& out = out_edge_itr_pair.first;
            auto& out_end = out_edge_itr_pair.second;
            const auto* table = curr_dg.get_vertex(v);
            for (; out != out_end; ++out) {
                auto dep = dep_graph[*out];
                auto vertex_later = boost::target(*out, dep_graph);
                const auto* table_later = curr_dg.get_vertex(vertex_later);
                if ((include_anti || !(dg.is_anti_edge(dep)))
                    && (include_control || !(dg.is_ctrl_edge(dep)) ||
                        table_later->getAnnotation("separate_gateway"_cs))
                    && dep != DependencyGraph::CONT_CONFLICT
                    && dep != DependencyGraph::REDUCTION_OR_OUTPUT
                    && dep != DependencyGraph::REDUCTION_OR_READ) {
                    happens_after_work_map[table_later].setbit(table_to_id.at(table));
                    happens_after_work_map[table_later] |= happens_after_work_map[table];
                    n_depending_on[vertex_later]--;
                    auto &vertex_later_edges = n_depending_on_with_edges[vertex_later];
                    auto rm_edge = std::find(vertex_later_edges.begin(),
                                                vertex_later_edges.end(), *out);
                    vertex_later_edges.erase(rm_edge);
                }
            }
         }

        processed.insert(this_generation.begin(), this_generation.end());
        rst.emplace_back(std::move(this_generation));
    }

    for (const auto& kv : happens_after_work_map) {
        auto* table_later = kv.first;
        for (int tbl_id : kv.second) {
            auto* table_earlier = id_to_table.at(tbl_id);
            curr_dg.happens_after_work_map[table_later].push_back(table_earlier);
            curr_dg.happens_before_work_map[table_earlier].push_back(table_later);
        }
    }

    return rst;
}

/**
 * The purpose of this class is to gather for each table (in Tofino), the set of tables
 * that must be placed due to next table propagation.  Both the leaves as well as the total
 * direct control dominating set are calculated.
 *
 * One cannot use the DependencyGraph to calculate this, as the inject control dependencies
 * calculated in InjectControlDependencies are not what the next table needs to propagate
 * through before the table can fully be placed.
 */
void CalculateNextTableProp::postorder(const IR::MAU::Table *tbl) {
    bool empty_seq = true;
    name_to_table[tbl->name] = tbl;
    for (auto seq : Values(tbl->next)) {
        empty_seq &= seq->tables.empty();
    }
    bool no_next_tables = tbl->next.empty() || empty_seq;

    if (no_next_tables) {
        next_table_leaves[tbl].insert(tbl);
    }

    control_dom_set[tbl].insert(tbl);

    LOG4(" NTP : For table : " << tbl->name);
    for (auto seq : Values(tbl->next)) {
        for (auto control_tbl : seq->tables) {
            next_table_leaves[tbl] |= next_table_leaves.at(control_tbl);
            control_dom_set[tbl] |= control_dom_set.at(control_tbl);
            LOG4(" NTP :   Next Table : " << control_tbl->name);
        }
    }
}

bool ControlPathwaysToTable::equiv_seqs(const IR::MAU::TableSeq *a,
        const IR::MAU::TableSeq *b) const {
    if (a == b) return true;
    return a->tables == b->tables;
}

ControlPathwaysToTable::Path ControlPathwaysToTable::common_reverse_path(const Path &a,
        const Path &b, bool check_diff_is_seq) const {
    Path rv;
    auto a_it = a.rbegin();
    auto b_it = b.rbegin();
    while (a_it != a.rend() && b_it != b.rend()) {
        const IR::Node *a_node = *a_it;
        const IR::Node *b_node = *b_it;
        auto a_seq = a_node->to<IR::MAU::TableSeq>();
        auto b_seq = b_node->to<IR::MAU::TableSeq>();
        if (a_seq && b_seq && equiv_seqs(a_seq, b_seq)) {
            rv.push_back(a_node);
        } else if (a_node == b_node) {
            rv.push_back(a_node);
        } else {
            if (check_diff_is_seq)
                BUG_CHECK(a_seq && b_seq, "The dominator pass is broken as these node should be "
                    "sequences");
            break;
        }
        a_it++;
        b_it++;
    }

    if ((a_it == a.rend() || b_it == b.rend()) && check_diff_is_seq)
        BUG("The dominator pass is broken as the iteration should not end");

    std::reverse(rv.begin(), rv.end());
    return rv;
}

// Eventually check for TableSeqs that aren't the same pointer but equivalent in their tables
bool ControlPathwaysToTable::equiv(const Path &a, const Path &b) const {
    auto path = common_reverse_path(a, b);
    return path.size() == a.size() && path.size() == b.size();
}

bool ControlPathwaysToTable::preorder(const IR::MAU::Table *tbl) {
    const Context *ctxt = getContext();
    Path pathway;
    pathway.emplace_back(tbl);
    while (ctxt) {
        pathway.push_back(ctxt->node);
        ctxt = ctxt->parent;
    }

    for (auto &path : table_pathways[tbl]) {
        if (equiv(path, pathway))
            return true;
    }

    table_pathways[tbl].emplace_back(pathway);
    return true;
}

/**
 * This function walks up all possible control pathways from a table to the top of the pipe.
 * The point in which two pathways differ is the point where next tables have to propagate
 * through in Tofino.
 *
 * The propagation point is let's say table A and table B are in the same table sequence, i.e.
 *
 * apply {
 *     switch (A.apply().action_run) {
 *         ... (let's say C is here)
 *     }
 *     switch (B.apply().action_run) {
 *         ... (let's say D is here)
 *     }
 * }
 *
 * Now due to the limitations of next table in Tofino, when A is placed, everything directly
 * control dependent on A must also be placed before B can be placed.  This is in order to
 * pass the next table pointer through the tables.  Thus if anything in A's control dominating
 * set has either an ANTI or DATA dependency on anything in B's control dominating set, then
 * A and it's control dependent set must logical be placed before B's.
 *
 * The goal for this function is when comparing two tables, i.e. C and D, to return to the point
 * where this inherent next table propagation is found.
 */
ControlPathwaysToTable::InjectPoints
        ControlPathwaysToTable::get_inject_points(const IR::MAU::Table *a,
        const IR::MAU::Table *b, bool tbls_only) const {
    InjectPoints rv;

    for (safe_vector<const IR::Node *> a_path : table_pathways.at(a)) {
        for (safe_vector<const IR::Node *> b_path : table_pathways.at(b)) {
            // Attempting to find the first divergence
            const IR::Node *a_first_div = nullptr;
            const IR::Node *b_first_div = nullptr;

            // If one table is within the next table pathway propagation pathway of another,
            // then there is nothing to inject, as this is already control dependendent
            if (std::find(a_path.begin(), a_path.end(), b) != a_path.end())
                continue;
            if (std::find(b_path.begin(), b_path.end(), a) != b_path.end())
                continue;

            auto a_path_it = a_path.rbegin();
            auto b_path_it = b_path.rbegin();

            // Start from the Pipe, and work down until the two pathways differ
            while (a_path_it != a_path.rend() && b_path_it != b_path.rend()) {
                if (*a_path_it == *b_path_it) {
                    a_path_it++;
                    b_path_it++;
                    continue;
                }

                a_first_div = *a_path_it;
                b_first_div = *b_path_it;
                break;
            }

            if (a_first_div->is<IR::MAU::TableSeq>()) {
                BUG_CHECK(b_first_div->is<IR::MAU::TableSeq>(), "The first divergence is not "
                    "the same IR node, thus the IR tree is inconsistent");
                if (!tbls_only)
                    rv.emplace_back(a_first_div, b_first_div);
                continue;
            }

            auto a_dom = a_first_div->to<IR::MAU::Table>();
            if (a_dom != nullptr) {
                auto b_dom = b_first_div->to<IR::MAU::Table>();
                BUG_CHECK(b_dom != nullptr, "The first divergence is not the same IR Node, thus "
                          "the IR tree is inconsistent");
                rv.emplace_back(a_dom, b_dom);
                continue;
            }
            BUG("The Table IR structure has a non-recognizable Node");
        }
    }
    return rv;
}

void ControlPathwaysToTable::print_paths(safe_vector<Path> &paths) const {
    for (auto path : paths) {
        LOG1("    Printing path in reverse:");
        for (auto it = path.rbegin(); it != path.rend(); it++) {
            if (auto tbl = (*it)->to<IR::MAU::Table>()) {
                if (tbl->is_placed())
                    LOG1("\tTable " << tbl->unique_id().build_name());
                else
                    LOG1("\tTable " << tbl->externalName());
            } else if (auto seq = (*it)->to<IR::MAU::TableSeq>()) {
                std::string print = "[ ";
                std::string sep = "";
                for (auto tbl : seq->tables) {
                    print += sep;
                    print += tbl->externalName();
                    sep = ", ";
                }
                print += " ]";
                LOG1("\tSeq " << print);
            } else if (auto pipe = (*it)->to<IR::BFN::Pipe>()) {
                LOG1("\tPipe " << pipe->canon_name());
            } else {
                LOG1("\tIDK?");
            }
        }
    }
}

/**
 * The purpose of this pass is to determine the dominator of a IR::MAU::Table object within
 * the IR graph.  If a table is applied multiple times, in accordance with the rules of the
 * IR, due to the mutually exclusion rules of the tables, a table is only applied once per
 * packet.  This means something like the following:
 *
 *     if (condition) {
 *         ...
 *         match_table.apply();
 *         ...
 *     } else {
 *         ...
 *         match_table.apply();
 *         ...
 *     }
 *
 * In this case, the condition is the dominator.  The same can be done for applications
 * of tables in either hitting or missing, or an action_chain:
 *
 *     if (t1.apply().hit) || if (t1.apply().miss) || switch(t1.apply().action_run)
 *
 * Currently, the use of a table will only be able to know if each apply of a Table
 * is mutually exclusive because it is entirely under some dominating table application, i.e.
 *
 *     switch (t1.apply().action_run) {
 *         a1 : { t2.apply(); t3.apply();
 *         default { t3.apply(); }
 *     }
 *
 * Table t3's applies are mutually exclusive, and always has a Table dominator, table t1.  The
 * following apply statement for example, is not supported.
 *
 *     if (x == 0)
 *         t1.apply();
 *     ... (no change to x)
 *     if (x != 0)
 *         t1.apply();
 *
 * In this technical mutual exclusion, the find_dominator would not be able to return a
 * table.  At that point, this function might have to change in order to capture this
 * kind of dominator
 */
const IR::MAU::Table *ControlPathwaysToTable::find_dominator(const IR::MAU::Table *init) const {
    auto paths = table_pathways.at(init);

    BUG_CHECK(paths.size() >= 1, "Path calculation is incorrect");
    if (paths.size() == 1)
        return init;

    Path common_path;
    for (uint32_t i = 0; i < paths.size(); i++) {
        if (i == 0)
            common_path = paths.at(i);
        else
            common_path = common_reverse_path(common_path, paths.at(i));
    }

    BUG_CHECK(common_path.size() > 0, "Dominator pathway is incorrect");

    auto dom = common_path[0]->to<IR::MAU::Table>();
    BUG_CHECK(dom, "Dominator pathway is incorrect");
    return dom;
}

void DepStagesThruDomFrontier::postorder(const IR::MAU::Table *tbl) {
    LOG3("DepStagesThruDomFrontier postorder : " << tbl->name);
    auto &dom_frontier = ntp.control_dom_set.at(tbl);
    if (dom_frontier.size() == 1) {
        dg.stage_info[tbl].dep_stages_dom_frontier = 0;
        return;
    }

    DependencyGraph local_dg;
    ordered_map<const IR::MAU::Table *,
        DependencyGraph::Graph::vertex_descriptor> local_labelToVertex;
    for (auto cd_tbl : dom_frontier) {
        local_dg.add_vertex(cd_tbl);
    }

    for (auto cd_tbl : dom_frontier) {
        auto src_v = dg.labelToVertex.at(cd_tbl);
        auto out_edge_itr_pair = boost::out_edges(src_v, dg.g);
        auto &out = out_edge_itr_pair.first;
        auto &out_end = out_edge_itr_pair.second;
        for (; out != out_end; ++out) {
            auto dst_v = boost::target(*out, dg.g);
            auto second_tbl = dg.get_vertex(dst_v);
            if (dom_frontier.count(second_tbl) == 0)
                continue;

            if (local_dg.has_back_edge(cd_tbl, second_tbl)) {
                LOG5("\tCannot add edge between " << cd_tbl->name
                        << " and " << second_tbl->name << ": " << dg.g[*out]
                        << " as a backedge already exists");
                continue;
            }
            LOG5("\tAdding edge between " << cd_tbl->name << " and "
                << second_tbl->name << " : " << dg.g[*out]);
            local_dg.add_edge(cd_tbl, second_tbl, dg.g[*out]);
        }
    }

    LOG4("CALC_TOPOLOGICAL_STAGE 1");

    auto topo_rst = self.calc_topological_stage(DependencyGraph::CONTROL_AND_ANTI, &local_dg);

    typename DependencyGraph::Graph::edge_iterator edges, edges_end;
    local_dg.dep_type_map.clear();
    for (boost::tie(edges, edges_end) = boost::edges(local_dg.g); edges != edges_end; ++edges) {
        const IR::MAU::Table* src = local_dg.get_vertex(boost::source(*edges, local_dg.g));
        const IR::MAU::Table* dst = local_dg.get_vertex(boost::target(*edges, local_dg.g));
        if ((local_dg.dep_type_map.count(src) == 0) ||
            (local_dg.dep_type_map.at(src).count(dst) == 0)
            || local_dg.is_ctrl_edge(local_dg.dep_type_map.at(src).at(dst))
            || (local_dg.dep_type_map.at(src).at(dst) == DependencyGraph::CONT_CONFLICT)

            || (local_dg.dep_type_map.at(src).at(dst) == DependencyGraph::REDUCTION_OR_READ)
            || (local_dg.dep_type_map.at(src).at(dst) == DependencyGraph::REDUCTION_OR_OUTPUT)
            || local_dg.is_anti_edge(local_dg.dep_type_map.at(src).at(dst))) {
            local_dg.dep_type_map[src][dst] = local_dg.g[*edges];
        }
    }

    local_dg.fill_dep_stages_from_topo(topo_rst, false, summary);

    // Compute the downward dependency chain by adding the number of stages required for each
    // indivual table based on last table allocation pass.
    local_dg.fill_dep_stages_from_topo(topo_rst, true, summary);

    dg.stage_info[tbl].dep_stages_dom_frontier
        = local_dg.stage_info.at(tbl).dep_stages_control_anti;
}

/**
 * Tofino specific (currently necessary for JBay until placement algorithm changes)
 *
 * Say we have the following program:
 *
 * if (t1.apply().hit) {
 *     t2.apply();
 * }
 * t3.apply();
 *
 * Now in Tofino, if t1 is placed, then t2 must always be placed, in order to correctly
 * propagate the next table, before any other table.  Thus if t3 were to a have a dependency
 * either LOGICAL (i.e. ANTI) or DATA (i.e. IXBAR_READ) to t1, any dependency that is directly
 * under CONTROL dependencies would reflect in the dependency chain length of t2.
 *
 * The solution to reflect that chain correctly is to add a LOGICAL dependence between t2 and
 * t3 (given that t1 and t3 have a dependence).   By definition, t3 would have to follow
 * t2, if it would have to follow t1.
 *
 *
 * This also applies to data dependencies through control dependent tables:
 *
 * if (t1.apply().hit) {
 *     t2.apply();
 * }
 *
 * if (t3.apply().hit) {
 *     t4.apply();
 * }
 *
 * Now if t2 and t4 have a data dependency or anti dependency as well, then t1 and t3 are ordered,
 * and thus an ANTI dependency through next table belongs between any control leaves of t1 and
 * the same tables in the sequence of t1, and this has to be reflected in the MIN STAGE analysis
 *
 * The algorithm is based on the following:
 *    1. If t_b logical has to be placed after t_a, then t_b has to be logically placed after
 *       all next tables propagation of t_a.
 *    2. Thus if t_b is not directly next table propagation dependent of t_a, add a LOGICAL
 *       dependence on all next table paths out of t_a and the TableSeq control dependent of
 *       t_b that are not mutually exclusive with t_b.
 */
void FindDependencyGraph::add_logical_deps_from_control_deps(void) {
    LOG4("CALC_TOPOLOGICAL_STAGE 2");

    std::vector<ordered_set<DependencyGraph::Graph::vertex_descriptor>>
        topo_rst = calc_topological_stage(DependencyGraph::ANTI);

    typedef std::pair<const IR::MAU::Table*, const IR::MAU::Table*> tpair;
    ordered_map<tpair, DependencyGraph::Graph::edge_descriptor> edges_to_add;

    for (int i = int(topo_rst.size()) - 1; i >= 0; --i) {
        for (auto& v : topo_rst[i]) {
            const IR::MAU::Table* table = dg.get_vertex(v);
            auto out_edge_itr_pair = boost::out_edges(v, dg.g);
            auto& out = out_edge_itr_pair.first;
            auto& out_end = out_edge_itr_pair.second;
            for (; out != out_end; ++out) {
                if (dg.is_ctrl_edge(dg.g[*out])) continue;
                auto vertex_later = boost::target(*out, dg.g);
                const auto* table_later = dg.get_vertex(vertex_later);

                auto &table_nt_leaves = ntp.next_table_leaves.at(table);
                auto &table_dom_set = ntp.control_dom_set.at(table);
                if (table_dom_set.count(table_later)) continue;
                auto inject_points = con_paths.get_inject_points(table, table_later);
                for (auto frontier_leaf : table_nt_leaves) {
                    if (mutex(table_later, frontier_leaf)) continue;
                    for (auto table_seq_pair : inject_points) {
                         auto tbl = table_seq_pair.second->to<IR::MAU::Table>();
                         auto key = std::make_pair(frontier_leaf, tbl);
                         edges_to_add[key] = *out;
                    }
                }
            }
        }
    }

    for (auto pair : edges_to_add) {
        /* auto src_v = */ dg.add_vertex(pair.first.first);
        /* auto dst_v = */ dg.add_vertex(pair.first.second);
        // auto e = boost::lookup_edge(src_v, dst_v, dg.g);
        // auto dep = e.second ? DependencyGraph::ANTI : DependencyGraph::ANTI_NEXT_TABLE_DATA;
        auto dep = DependencyGraph::ANTI_NEXT_TABLE_DATA;
        auto source = boost::source(pair.second, dg.g);
        auto target = boost::target(pair.second, dg.g);
        const IR::MAU::Table* tsource = dg.get_vertex(source);
        const IR::MAU::Table* ttarget = dg.get_vertex(target);
        std::string src_name = std::string(tsource ? tsource->name : "SINK");
        std::string dst_name = std::string(ttarget ? ttarget->name : "SINK");
        LOG4("\t\tadd_dependency " << dep_types(dep) << " " << pair.first.first->name << "-"
             << pair.first.second->name << " due to original dependency on "
             << src_name << "->" << dst_name << " of type " << dep_types(dg.g[pair.second]));

        auto edge_pair = dg.add_edge(pair.first.first, pair.first.second, dep);
        if (!edge_pair.first) continue;
        dg.data_annotations[*edge_pair.first] = dg.data_annotations[pair.second];

        LOG4("CALC_TOPOLOGICAL_STAGE 3");

        calc_topological_stage(DependencyGraph::CONTROL_AND_ANTI);
    }
}

void FindDependencyGraph::finalize_dependence_graph(void) {
    if (!Device::hasLongBranches() || (options && options->disable_long_branch))
        add_logical_deps_from_control_deps();

    // Topological sort

    LOG4("CALC_TOPOLOGICAL_STAGE 4");

    std::vector<ordered_set<DependencyGraph::Graph::vertex_descriptor>>
        topo_rst = calc_topological_stage();


    if (LOGGING(4)) {
        for (size_t i = 0; i < topo_rst.size(); ++i) {
            LOG4(">>> Stage#" << i << ":");
            for (const auto& vertex : topo_rst[i])
                LOG4("Table:  " << vertex << ", " << dg.get_vertex(vertex)->name);
        }
    }

    // Build dep_stages
    for (int i = int(topo_rst.size()) - 1; i >= 0; --i) {
        for (const auto& vertex : topo_rst[i]) {
            const IR::MAU::Table* table = dg.get_vertex(vertex);
            auto& happens_later = dg.happens_before_work_map[table];
            dg.stage_info[table].dep_stages =
                std::accumulate(happens_later.begin(), happens_later.end(), 0,
                                [this] (int sz, const IR::MAU::Table* later) {
                                    return std::max(sz, dg.stage_info[later].dep_stages + 1); });
        }
    }

    typename DependencyGraph::Graph::edge_iterator edges, edges_end;
    dg.dep_type_map.clear();
    for (boost::tie(edges, edges_end) = boost::edges(dg.g); edges != edges_end; ++edges) {
        const IR::MAU::Table* src = dg.get_vertex(boost::source(*edges, dg.g));
        const IR::MAU::Table* dst = dg.get_vertex(boost::target(*edges, dg.g));
        if ((dg.dep_type_map.count(src) == 0) || (dg.dep_type_map.at(src).count(dst) == 0)
            || dg.is_ctrl_edge(dg.dep_type_map.at(src).at(dst))
            || (dg.dep_type_map.at(src).at(dst) == DependencyGraph::CONT_CONFLICT)
            || (dg.dep_type_map.at(src).at(dst) == DependencyGraph::REDUCTION_OR_READ)
            || (dg.dep_type_map.at(src).at(dst) == DependencyGraph::REDUCTION_OR_OUTPUT)) {
            if (!dg.is_anti_edge(dg.g[*edges])) {
                dg.dep_type_map[src][dst] = dg.g[*edges];
            }
        }
    }

    // Build dep_stages_control

    LOG4("CALC_TOPOLOGICAL_STAGE 5");

    std::vector<ordered_set<DependencyGraph::Graph::vertex_descriptor>>
        topo_rst_control = calc_topological_stage(DependencyGraph::CONTROL);
    for (int i = int(topo_rst_control.size()) - 1; i >= 0; --i) {
        for (const auto& vertex : topo_rst_control[i]) {
            const IR::MAU::Table* table = dg.get_vertex(vertex);
            auto& happens_later = dg.happens_before_work_map[table];
            dg.stage_info[table].dep_stages_control = std::accumulate(happens_later.begin(),
                happens_later.end(), 0, [this, table] (int sz, const IR::MAU::Table* later) {
                    int stage_addition = 0;
                    if (dg.dep_type_map.count(table) && dg.dep_type_map.at(table).count(later)
                        && !(dg.is_ctrl_edge(dg.dep_type_map.at(table).at(later)))
                        && dg.dep_type_map.at(table).at(later) !=
                           DependencyGraph::CONT_CONFLICT
                        && dg.dep_type_map.at(table).at(later) !=
                           DependencyGraph::REDUCTION_OR_READ
                        && dg.dep_type_map.at(table).at(later) !=
                           DependencyGraph::REDUCTION_OR_OUTPUT) {
                            if (dg.is_anti_edge(dg.dep_type_map.at(table).at(later))) {
                                LOG4("Adding stage from anti");
                            }
                            stage_addition = 1;
                    }
                    return std::max(sz, dg.stage_info[later].dep_stages_control + stage_addition);
           });
            LOG4("Dep stages of " << dg.stage_info[table].dep_stages <<
                " for table " << table->name);
            LOG4("Dep stages control of " << dg.stage_info[table].dep_stages_control <<
                " for table " << table->name);
        }
    }

    // dg.happens_before_control_map = dg.happens_before_work_map;
    for (const auto& kv : dg.happens_before_work_map) {
        auto* table = kv.first;
        for (const auto* prev : kv.second) {
            dg.happens_before_control_map[prev].insert(table); } }
    if (LOGGING(4)) {
        std::stringstream ss;
        for (auto& kv : dg.happens_before_control_map) {
            ss << "Table " << kv.first->name << " has priors of ";
            for (auto& tbl : kv.second) {
                ss << tbl->name << ", ";
            }
            ss << "\n";
        }
        LOG4(ss.str());
        for (auto& kv_outer : dg.dep_type_map) {
            for (auto& kv_inner : kv_outer.second) {
                auto* initial_table = kv_outer.first;
                auto* later_table = kv_inner.first;
                auto value = kv_inner.second;
                LOG4("Initial table " << initial_table->name << " with later table "
                    << later_table->name << " and value " << value);
            }
        }
    }

    // When we include control and anti dependences, what we're really computing is the
    // happens_before with respect to logical IDs, not stages. In other words, adding in control
    // dependences and anti dependences tells us the set of tables a table needs to be placed
    // logically before to guarantee correct execution. This is a stricter requirement than without
    // control and anti dependences. If Table A has a control/anti dependence on Table B, while we
    // may be able to place Table A and Table B in the same stage, we cannot place Table A in an
    // earlier stage than Table B. To enforce this (since table placement only places one table at a
    // time), we guarantee that Table B has a lower logical ID than Table A (i.e. Table A doesn't
    // get placed until Table B has been placed).

    LOG4("CALC_TOPOLOGICAL_STAGE 6");

    auto topo_rst_logical = calc_topological_stage(DependencyGraph::CONTROL_AND_ANTI);
    // Log the resulting sort
    if (LOGGING(4)) {
        LOG4("Printing results of topological sorting with control and anti dependences included."
             << " This is a topological sort with respect to logical placement");
        for (size_t i = 0; i < topo_rst_logical.size(); ++i) {
            LOG4(">>> Stage#" << i << ":");
            for (const auto& vertex : topo_rst_logical[i]) {
                const auto* t = dg.get_vertex(vertex);
                LOG4("Table: " << vertex << ", " << t->name);
            }
        }
    }
    // Construct the maps
    for (auto kv : dg.happens_before_work_map) {
        auto& map = dg.happens_logi_before_map[kv.first];
        for (auto* tbl : kv.second)
            map.insert(tbl);
    }
    for (auto kv : dg.happens_after_work_map) {
        auto& map = dg.happens_logi_after_map[kv.first];
        for (auto* tbl : kv.second)
            map.insert(tbl);
    }

    if (LOGGING(4)) {
        std::stringstream ss;
        for (auto& kv : dg.happens_logi_before_map) {
            ss << "Table " << kv.first->name << " has priors of ";
            for (auto& tbl : kv.second)
                ss << tbl->name << ", ";
            ss << std::endl;
        }
        LOG4(ss.str());
    }
    typename DependencyGraph::Graph::edge_iterator edges2, edges2_end;
    dg.dep_type_map.clear();
    for (boost::tie(edges2, edges2_end) = boost::edges(dg.g); edges2 != edges2_end; ++edges2) {
        const IR::MAU::Table* src = dg.get_vertex(boost::source(*edges2, dg.g));
        const IR::MAU::Table* dst = dg.get_vertex(boost::target(*edges2, dg.g));
        if ((dg.dep_type_map.count(src) == 0) || (dg.dep_type_map.at(src).count(dst) == 0)
            || (dg.dep_type_map.at(src).at(dst) == DependencyGraph::CONT_CONFLICT)
            || dg.is_ctrl_edge(dg.dep_type_map.at(src).at(dst))
            || (dg.dep_type_map.at(src).at(dst) == DependencyGraph::REDUCTION_OR_READ)
            || (dg.dep_type_map.at(src).at(dst) == DependencyGraph::REDUCTION_OR_OUTPUT)
            || dg.is_anti_edge(dg.dep_type_map.at(src).at(dst))) {
            dg.dep_type_map[src][dst] = dg.g[*edges2];
        }
    }

    dg.fill_dep_stages_from_topo(topo_rst_logical, false, summary);

    // Compute the downward dependency chain by adding the number of stages required for each
    // indivual table based on last table allocation pass.
    dg.fill_dep_stages_from_topo(topo_rst_logical, true, summary);

    // Build min_stage
    // Min_stage for a table T is the minimum stage in which T (and any table that T is
    // control-dependent upon) can be placed. So, if table T2 has no match or action dependencies
    // but is control dependent on table T1, which -- because of its own match and action
    // dependencies -- must be placed in at least stage 4, then T2 and T1 will both have a
    // min_stage of 4.
    for (size_t i = 0; i < topo_rst_logical.size(); ++i) {
        for (const auto& vertex : topo_rst_logical[i]) {
            const IR::MAU::Table* table = dg.get_vertex(vertex);
            dg.stage_info[table].min_stage = i;
        }
    }


    // Compress the stages to take out the addition caused by control edges and anti edges,
    // but the min stage needs to be propagated through these edges
    for (size_t i = 1; i < topo_rst_logical.size(); i++) {
        for (const auto& vertex : topo_rst_logical[i]) {
            const auto* tbl = dg.get_vertex(vertex);
            int orig_stage = dg.stage_info[tbl].min_stage;
            int true_min_stage = 0;
            auto in_edges = boost::in_edges(vertex, dg.g);

            ordered_map<int, safe_vector<DependencyGraph::MinEdgeInfo>> min_edges_of_table;

            for (auto edge = in_edges.first; edge != in_edges.second; edge++) {
                auto src_vertex = boost::source(*edge, dg.g);
                const auto* src_table = dg.get_vertex(src_vertex);
                int src_vertex_stage = dg.stage_info[src_table].min_stage;
                int min_stage_from_src;
                DependencyGraph::dependencies_t dep = dg.g[*edge];
                if (dep == DependencyGraph::CONT_CONFLICT) continue;
                if (dep == DependencyGraph::ACTION_READ || dep == DependencyGraph::IXBAR_READ ||
                    dep == DependencyGraph::OUTPUT || tbl->getAnnotation("separate_gateway"_cs)) {
                    min_stage_from_src = src_vertex_stage + 1;
                } else if (dg.is_ctrl_edge(dep) ||
                           dep == DependencyGraph::CONT_CONFLICT ||
                           dep == DependencyGraph::REDUCTION_OR_READ ||
                           dep == DependencyGraph::REDUCTION_OR_OUTPUT ||
                           dg.is_anti_edge(dep)) {
                    min_stage_from_src = src_vertex_stage;
                } else {
                    BUG("Unhandled dependency");
                }
                true_min_stage = std::max(true_min_stage, min_stage_from_src);
                min_edges_of_table[min_stage_from_src].emplace_back(src_table, dep);
                BUG_CHECK(true_min_stage <= orig_stage, "stage should only decrease");
                // There shouldn't be any edges within a layer,
                // so starting from the lowest stage and moving out should be fine
            }
            dg.min_stage_edges[tbl] = min_edges_of_table[true_min_stage];
            dg.stage_info[tbl].min_stage = true_min_stage;
            LOG5("\tmin_stage for " << tbl->name << " : " << dg.stage_info[tbl].min_stage);
        }
    }

    if (LOGGING(3))
        dg.display_min_edges = true;

    // Compute for the final time the dependence graph, which will leave happens_after and
    // happens_before with stage orders
    dg.vertex_rst = topo_rst_logical;

    LOG4("CALC_TOPOLOGICAL_STAGE 7");

    calc_topological_stage();
    // Use this final computation to create the happens_physical maps
    for (auto kv : dg.happens_before_work_map) {
        auto& map = dg.happens_phys_before_map[kv.first];
        for (auto* tbl : kv.second)
            map.insert(tbl);
    }
    for (auto kv : dg.happens_after_work_map) {
        auto& map = dg.happens_phys_after_map[kv.first];
        for (auto* tbl : kv.second)
            map.insert(tbl);
    }

    // If A happens logically before B, and B happens physically before C, then A happens
    // physically before C.  The original maps created from these happens_work_maps did not
    // have this physical relation
    for (auto kv : dg.happens_after_work_map) {
        for (auto after_tbl : kv.second) {
            dg.happens_phys_after_map[kv.first].insert(
                dg.happens_logi_after_map[after_tbl].begin(),
                dg.happens_logi_after_map[after_tbl].end());
        }
    }

    for (auto kv : dg.happens_before_work_map) {
        for (auto before_tbl : kv.second) {
            dg.happens_phys_before_map[kv.first].insert(
                dg.happens_logi_before_map[before_tbl].begin(),
                dg.happens_logi_before_map[before_tbl].end());
        }
    }

    verify_dependence_graph();
    if (LOGGING(4))
        DependencyGraph::dump_viz(std::cout, dg);
    calc_max_min_stage();
    dg.finalized = true;
}

void FindDependencyGraph::calc_max_min_stage() {
    for (const auto& kv : dg.stage_info) {
        auto gress = kv.first->gress;
        if (kv.second.min_stage > dg.max_min_stage_per_gress[gress])
            dg.max_min_stage_per_gress[gress] = kv.second.min_stage;
    }
    dg.max_min_stage = (dg.max_min_stage_per_gress[0] > dg.max_min_stage_per_gress[1]) ?
        dg.max_min_stage_per_gress[0] : dg.max_min_stage_per_gress[1];
    dg.max_min_stage = (dg.max_min_stage > dg.max_min_stage_per_gress[2]) ? dg.max_min_stage :
        dg.max_min_stage_per_gress[2];
    LOG3("    Maximum stage number according to dependences: ");
    LOG3("      INGRESS: " << dg.max_min_stage_per_gress[INGRESS]);
    LOG3("      EGRESS: " << dg.max_min_stage_per_gress[EGRESS]);
}

void FindDependencyGraph::verify_dependence_graph() {
    typename DependencyGraph::Graph::edge_iterator out, out_end;
    for (boost::tie(out, out_end) = boost::edges(dg.g);
         out != out_end;
         ++out) {
        const IR::MAU::Table *t1 = dg.get_vertex(boost::source(*out, dg.g));
        const IR::MAU::Table *t2 = dg.get_vertex(boost::target(*out, dg.g));
        if ((t1->gress == EGRESS && t2->gress == INGRESS)
            || (t1->gress == INGRESS && t2->gress == EGRESS))
            BUG("Ingress table '%s' depends on egress table '%s'.", t1->name, t2->name); }
}

bool PrintPipe::preorder(const IR::BFN::Pipe *pipe) {
    LOG2(TableTree("ingress"_cs, pipe->thread[INGRESS].mau) <<
         TableTree("egress"_cs, pipe->thread[EGRESS].mau) <<
         TableTree("ghost"_cs, pipe->ghost_thread.ghost_mau) );
    return false;
}

FindDependencyGraph::FindDependencyGraph(const PhvInfo &phv,
                                         DependencyGraph &out,
                                         const BFN_Options *o,
                                         cstring dotFileName,
                                         cstring passCont,
                                         const TableSummary *s) :
        Logging::PassManager("table_dependency_graph"_cs, Logging::Mode::AUTO),
        dg(out), options(o), dotFile(dotFileName), passContext(passCont), summary(s) {
    addPasses({
        new NameToTableMapBuilder(dg),
        &mutex,
        &ntp,
        &con_paths,
        &ignore,
        new GatherReductionOrReqs(dg.red_info),
        new PrintPipe,
        new TableFindInjectedDependencies(phv, dg, fg, options, summary),
        new FindDataDependencyGraph(phv, dg, mutex, ignore),
        new DepStagesThruDomFrontier(ntp, dg, *this, s)
    });
}

Visitor::profile_t FindDependencyGraph::init_apply(const IR::Node *node) {
    auto rv = Logging::PassManager::init_apply(node);
    if (!passContext.isNullOrEmpty())
        LOG1("FindDependencyGraph : " << passContext << " : "
                << (summary ? summary->getActualStateStr() : " NA "));
    dg.clear();
    return rv;
}

void FindDependencyGraph::end_apply(const IR::Node *root) {
    finalize_dependence_graph();

    LOG2(dg);
    if (BackendOptions().create_graphs && dotFile != "") {
        auto pipeId = root->to<IR::BFN::Pipe>()->canon_id();
        auto graphsDir = BFNContext::get().getOutputDirectory("graphs"_cs, pipeId);
        std::ofstream dotStream(graphsDir + "/" + dotFile + ".dot", std::ios_base::out);
        DependencyGraph::dump_viz(dotStream, dg);
        Logging::Manifest::getManifest().addGraph(pipeId, cstring("table"), dotFile,
                                                  INGRESS);  // this should be both really!
    }
}

std::ostream &operator<<(std::ostream &out, DependencyGraph::dependencies_t deps) {
    static const char *bit_names[] = {
            "NONE", "CONTROL_ACTION", "CONTROL_COND_TRUE", "CONTROL_COND_FALSE",
            "CONTROL_TABLE_HIT", "CONTROL_TABLE_MISS", "CONTROL_DEFAULT_NEXT_TABLE",
            "IXBAR_READ", "ACTION_READ", "OUTPUT", "REDUCTION_OR_READ", "REDUCTION_OR_OUTPUT",
            "CONT_CONFLICT", "ANTI_EXIT", "ANTI_TABLE_READ", "ANTI_ACTION_READ",
            "ANTI_NEXT_TABLE_DATA", "ANTI_NEXT_TABLE_CONTROL", "ANTI_NEXT_TABLE_METADATA",
            "CONTROL_EXIT"
    };

    bool first = true;
    uint64_t bits = static_cast<uint64_t>(deps);
    for (unsigned i = 0; i < sizeof(bit_names)/sizeof(bit_names[0]); ++i) {
        if (bits & (1U << i)) {
            out << (first ? "" : "|") << bit_names[i];
            first = false; } }
    if (first || bits >= (1U << sizeof(bit_names)/sizeof(bit_names[0])))
        out << "<0x" << hex(bits) << ">";
    return out;
}


bool PrintDependencyGraph::preorder(const IR::BFN::Pipe *p) {
    vertex_names.clear();
    edge_names.clear();
    bitvec_to_char.clear();
    char_to_bitvec.clear();
    min_path_len = (int) Device::numStages() * 0.7;
    pipe_name = p->canon_name();
    DependencyGraph::Graph::vertex_iterator vertices, vertices_end;
    for (boost::tie(vertices, vertices_end) = boost::vertices(dg.g);
         vertices != vertices_end; ++vertices) {
        vertex_names.emplace(*vertices, dg.get_vertex(*vertices)->name);
    }

    DependencyGraph::Graph::edge_iterator edges, edges_end;
    for (boost::tie(edges, edges_end) = boost::edges(dg.g); edges != edges_end; ++edges) {
        auto src = boost::source(*edges, dg.g);
        auto dst = boost::target(*edges, dg.g);

        edge_names.emplace(*edges, vertex_names.at(src) + "->" + vertex_names.at(dst));
    }
    return false;
}

// encode dependencies between source and destination as characters
char PrintDependencyGraph::encode_dependencies(
        const DependencyGraph::Graph::vertex_descriptor &src_v,
        const DependencyGraph::Graph::vertex_descriptor &dst_v) {
    bitvec deps;
    typename DependencyGraph::Graph::out_edge_iterator out, end;
    for (boost::tie(out, end) = boost::out_edges(src_v, dg.g); out != end; ++out) {
        if (boost::target(*out, dg.g) == dst_v) {
            deps |= dg.g[*out];
        }
    }
    if (bitvec_to_char.count(deps) == 0) {
        char c = 'A' + bitvec_to_char.size();
        if (bitvec_to_char.size() >= 26) {
            c = 'a' + bitvec_to_char.size() - 26;
        }
        bitvec_to_char.emplace(deps, c);
        char_to_bitvec.emplace(c, deps);
    }
    return bitvec_to_char.at(deps);
}

// loop through each bit in bitvec, if it is set, print the corresponding
// DependencyGraph::dependencies_t value
std::string PrintDependencyGraph::print_dependencies(bitvec deps) {
    std::stringstream str;
    // Must be consistent with dependencies_t
    for (unsigned i = 1; i < 19; ++i) {
        if (deps & bitvec(1U << i)) {
            str << " " << dep_types(DependencyGraph::dependencies_t(1U << i));
        }
    }
    return str.str();
}

std::stringstream PrintDependencyGraph::print_graph(const DependencyGraph& dg) {
    std::stringstream ss;
    // sort vertices by gress, stage, logical_id
    std::vector<DependencyGraph::Graph::vertex_descriptor> vertices;
    DependencyGraph::Graph::vertex_iterator vertices_it, vertices_end;
    for (boost::tie(vertices_it, vertices_end) = boost::vertices(dg.g);
         vertices_it != vertices_end; ++vertices_it) {
        vertices.push_back(*vertices_it);
    }
    std::sort(vertices.begin(), vertices.end(), [&](auto a, auto b) {
        auto a_vertex = dg.get_vertex(a);
        auto b_vertex = dg.get_vertex(b);
        if (a_vertex->thread() != b_vertex->thread())
            return a_vertex->thread() < b_vertex->thread();
        if (a_vertex->stage() != b_vertex->stage())
            return a_vertex->stage() < b_vertex->stage();
        return a_vertex->logical_id < b_vertex->logical_id;
    });

    // print vertices matrix and encode dependencies with ASCII chars.
    int row_stage_separator = 0;
    ss << "#pipeline " << pipe_name << std::endl;
    ss << "#stage " << row_stage_separator << std::endl;
    for (auto& src_v : vertices) {
        if (row_stage_separator != dg.get_vertex(src_v)->stage()) {
            row_stage_separator = dg.get_vertex(src_v)->stage();
            ss << "#stage " << row_stage_separator << std::endl;
        }
        int column_stage_separator = 0;
        for (auto& dst_v : vertices) {
            // print a space to indicate a new stage
            if (column_stage_separator != dg.get_vertex(dst_v)->stage()) {
                ss << " ";
                column_stage_separator = dg.get_vertex(dst_v)->stage();
            }
            // if there is not edge between src_v and dst_v, print '-'
            if (src_v == dst_v) {  // print '^' on diagonal
                ss << "^";
            } else if (!boost::edge(dst_v, src_v, dg.g).second) {
                ss << "-";
            } else {
                // encode dependencies between src_v and dst_v
                // note that we would like to print the dependencies in the
                // lower-left of the matrix, hence the inverted order of src_v
                // and dst_v
                auto deps = encode_dependencies(dst_v, src_v);
                ss << deps;
            }
        }
        auto t = dg.get_vertex(src_v);
        ss << " " << vertex_names.at(src_v) << "("
            << dg.min_stage(t) << ","
           << dg.max_stage(t) << ") : " << t->gress;
        ss << std::endl;
    }

    // print dependencies encoded with ASCII chars
    ss << "#dependencies" << std::endl;
    for (auto& kv : char_to_bitvec)
        ss << kv.first << " : " << print_dependencies(kv.second) << std::endl;

    return ss;
}

void PrintDependencyGraph::reverse_dfs(const IR::MAU::Table *tbl,
                                       std::stack<const IR::MAU::Table*>& chain,
                                       std::vector<std::vector<const IR::MAU::Table*>>& crit_chains,
                                       std::map<const IR::MAU::Table*, bool>& visited_tbls,
                                       int last_stg) {
    chain.push(tbl);
    LOG7(Log::indent << "reverse_dfs table: " << tbl->name);
    BUG_CHECK(dg.stage_info.count(tbl), "Table %1% not in stage_info  ...? ", tbl->name);

    if (dg.min_stage_edges.count(tbl)) {
        std::set<const IR::MAU::Table*> prev_tbls;
        for (auto [p_tbl, dep] : dg.min_stage_edges.at(tbl)) {
            if (!visited_tbls[tbl] && !(prev_tbls.count(p_tbl)))
                reverse_dfs(p_tbl, chain, crit_chains, visited_tbls, last_stg);
            prev_tbls.insert(p_tbl);
        }
    }

    BUG_CHECK(dg.stage_info.count(tbl), "No stage info for %1% ?", tbl->name);

    int pipe_size = last_stg - dg.stage_info.at(tbl).min_stage;
    LOG7(Log::indent << "pipe_size: " << pipe_size << Log::unindent << Log::unindent);

    if (pipe_size > min_path_len) {
        std::vector<const IR::MAU::Table*> crit_chain;
        std::stack<const IR::MAU::Table*> chain_stack(chain);
        while (!chain_stack.empty()) {
            crit_chain.push_back(chain_stack.top());
            chain_stack.pop();
        }
        crit_chains.push_back(crit_chain);
    }

    visited_tbls[tbl] = true;
    chain.pop();
}



void PrintDependencyGraph::print_critical_chains(const DependencyGraph& dg) {
    std::list<std::vector<const IR::MAU::Table*>> chains;
    std::vector<std::vector<const IR::MAU::Table*>> crit_chains;
    std::map<int, std::vector<const IR::MAU::Table*>> tablesPerMinStage;
    std::map<const IR::MAU::Table*, bool> visited_tbls;
    LOG6("Looking for Table dependency chains wih minimum length of " << min_path_len <<
         Log::indent);

    // First map tables to their min stages
    for (const auto& kv : dg.stage_info) {
        auto *tbl = kv.first;
        auto info = kv.second;
        tablesPerMinStage[info.min_stage].push_back(tbl);
        LOG7("Map table " << tbl->name << " to stage " << info.min_stage);
        visited_tbls[tbl] = false;
    }

    // Start traversal from tables with MAX min_stage. Use reverse_dfs()
    // and min_stage_edges to build vectors of table chains that are
    // at least as long as 70% of the device pipeline length

    std::stack<const IR::MAU::Table*> cur_chain;
    bool stop_search = false;

    for (auto rItr = tablesPerMinStage.rbegin(); rItr!=tablesPerMinStage.rend(); ++rItr) {
        if (stop_search) break;
        // Iterate over the tables in stage stg
        for (auto *tbl : rItr->second) {
            BUG_CHECK(dg.stage_info.count(tbl), "B. No stage_info for %1%", tbl->name);

            auto info = dg.stage_info.at(tbl);
            LOG7("table: " << tbl->name << " min_stage: " << info.min_stage <<
                 " dep_stages: " << info.dep_stages);

            // No need to do something if table has been already visited
            if (visited_tbls[tbl]) continue;

            if (info.min_stage < min_path_len) {
                stop_search = true;
                LOG7("\t STOPPING SEARCH ...");
                break;
            } else {
                if (visited_tbls[tbl]) continue;
                if (!dg.min_stage_edges.count(tbl)) continue;
                int last_stg = dg.stage_info.at(tbl).min_stage;
                // Start new table chain
                reverse_dfs(tbl, cur_chain, crit_chains, visited_tbls, last_stg);
            }
        }
    }

    LOG_FEATURE("table_dependency_summary", 3, Log::unindent <<
                "*** Table dependency chains (of minimum length " << min_path_len << ") ***");
    int c_num = 1;
    for (const auto &vec : crit_chains) {
        std::stringstream ss;
        int t_num = 0;
        gress_t gress = GHOST;
        for (const auto* t : vec) {
            if (t_num) {
                ss << " -> ";
                BUG_CHECK(gress == t->gress, "Formed critical chain with different-gress tables?");
            } else {
                gress = t->gress;
                ss << "\n" << c_num << ". ";
            }
            BUG_CHECK(dg.stage_info.count(t), "C. No stage_info for %1%", t->name);
            ss << t->name << "(" << dg.stage_info.at(t).min_stage << ")" ;
            t_num++;
        }
        ss << " (" << gress << ")";
        LOG_FEATURE("table_dependency_summary", 3, ss);
        c_num++;
    }
}

void PrintDependencyGraph::end_apply(const IR::Node *) {
    LOG_FEATURE("table_dependency_summary", 3, print_graph(dg));
    print_critical_chains(dg);
}
