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

#include "bf-p4c/phv/analysis/dominator_tree.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dominator_tree.hpp>

#include "bf-p4c/device.h"

using namespace boost;

void BuildDominatorTree::setupDomTree() {
    // Generate dominator tree for each gress.
    for (const auto &entry : flowGraph) {
        const auto &gress = entry.first;
        const auto &fg = entry.second;

        ordered_map<int, const IR::MAU::Table *> indexMap;
        // If the flow graph is empty (e.g. no egress tables), no need to build the dominator map.
        // However, we still create an ImmediateDominatorMap for the particular gress.
        if (fg.emptyFlowGraph) {
            iDominator.emplace(gress, new ImmediateDominatorMap());
            continue;
        }
        BUG_CHECK(fg.gress, "Gress not assigned for flow graph");
        BUG_CHECK(*fg.gress == gress, "FindFlowGraphs resulted in an inconsistent data structure");
        generateIndexToTableMap(fg, indexMap);
        iDominator.emplace(gress, new ImmediateDominatorMap());
        generateDominatorTree(fg, indexMap, *(iDominator.at(gress)));
    }

    if (LOGGING(1)) {
        for (gress_t gress : Range(INGRESS, GHOST)) {
            if (iDominator.count(gress) == 0) continue;
            LOG1("\tPrinting dominator tree for " << gress);
            printDominatorTree(*(iDominator.at(gress)));
        }
    }
}

bool BuildDominatorTree::preorder(const IR::BFN::Pipe *pipe) {
    FindFlowGraphs find_flow_graphs(flowGraph);
    pipe->apply(find_flow_graphs);

    // Generate dominator tree for each gress.
    setupDomTree();

    return true;
}

bool BuildDominatorTree::preorder(const IR::MAU::Table *tbl) {
    BUG_CHECK(!tbl->is_always_run_action(),
              "BuildDomintorTree cannot be run after always-run action tables are inserted. ARA "
              "table: %1%",
              tbl->name);
    return false;
}

Visitor::profile_t BuildDominatorTree::init_apply(const IR::Node *root) {
    flowGraph.clear();
    for (auto idom : iDominator) idom.second->clear();
    iDominator.clear();
    return Inspector::init_apply(root);
}

void BuildDominatorTree::generateIndexToTableMap(
    const FlowGraph &fg, ordered_map<int, const IR::MAU::Table *> &indexMap) const {
    graph_traits<G>::vertex_iterator uItr, uEnd;
    // For each vertex, create a map of the index of the vertex to the table that vertex represents.
    // This is useful for translating the results of the boost dominator algorithm (which operates
    // on the vertex indices) to the associated table pointers.
    for (boost::tie(uItr, uEnd) = vertices(fg.g); uItr != uEnd; ++uItr)
        indexMap[boost::get(boost::vertex_index, fg.g)[*uItr]] =
            boost::get(boost::vertex_table, fg.g)[*uItr];
    if (!LOGGING(4)) return;
    LOG4("      Printing index to table names");
    for (auto kv : indexMap)
        if (kv.second != nullptr) LOG4("        " << kv.first << "\t:\t" << kv.second->name);
}

void BuildDominatorTree::generateDominatorTree(
    const FlowGraph &fg, const ordered_map<int, const IR::MAU::Table *> &indexToTableMap,
    ImmediateDominatorMap &iDom) {
    // idom is a mapping from index of a vertex to the index of its immediate dominator vertex.
    ordered_map<int, int> idom;

    // Standard boost dominator tree analysis.
    std::vector<Vertex> domTreePredVector;
    IndexMap indexMap(boost::get(vertex_index, fg.g));
    graph_traits<G>::vertex_iterator uItr, uEnd;
    domTreePredVector = std::vector<Vertex>(num_vertices(fg.g), graph_traits<G>::null_vertex());
    PredMap domTreePredMap = make_iterator_property_map(domTreePredVector.begin(), indexMap);
    lengauer_tarjan_dominator_tree(fg.g, vertex(1, fg.g), domTreePredMap);

    // We use the map idom to first interpret the results of the dominator analysis. This allows us
    // to get the dominator information without needing to handle the special case of the SINK node
    // (which does not have any table pointer associated with it) and the SOURCE node (which has a
    // NULL vertex as the immediate dominator).
    for (boost::tie(uItr, uEnd) = vertices(fg.g); uItr != uEnd; ++uItr) {
        if (boost::get(domTreePredMap, *uItr) != graph_traits<G>::null_vertex()) {
            idom[boost::get(indexMap, *uItr)] =
                boost::get(indexMap, boost::get(domTreePredMap, *uItr));
            LOG4("\t\tSetting dominator for "
                 << boost::get(indexMap, *uItr) << " to "
                 << boost::get(indexMap, boost::get(domTreePredMap, *uItr)));
        } else {
            idom[boost::get(indexMap, *uItr)] = -1;
            LOG4("\t\tSetting dominator for " << boost::get(indexMap, *uItr) << " to -1");
        }
    }

    // This is where we translate the data in idom and translate it into a map between table
    // pointers. If the node is a source node (its immediate dominator is the NULL vertex), we
    // assign the dominator of that node to itself. We also verify that the sink node was not
    // assigned to be an immediate dominator to any node.
    for (auto kv : idom) {
        if (kv.second == -1) {
            iDom[indexToTableMap.at(kv.first)] = indexToTableMap.at(kv.first);
        } else if (kv.second == 0) {
            BUG("Sink node cannot be the immediate dominator");
        } else {
            if (!indexToTableMap.count(kv.second))
                BUG("Unknown table with index %1% not found", kv.second);
            iDom[indexToTableMap.at(kv.first)] = indexToTableMap.at(kv.second);
        }
    }
}

void BuildDominatorTree::printDominatorTree(const ImmediateDominatorMap &idom) const {
    for (auto kv : idom) {
        std::string idominator;
        std::string source;
        if (kv.second == kv.first)
            idominator = "SOURCE";
        else
            idominator = kv.second->name;
        if (kv.first == 0)
            source = "SINK";
        else
            source = kv.first->name;
        LOG1("\t\t" << boost::format("%=25s") % source << "\t:\t"
                    << boost::format("%=25s") % idominator);
    }
}

std::optional<const IR::MAU::Table *> BuildDominatorTree::getImmediateDominator(
    const IR::MAU::Table *t, gress_t gress) const {
    cstring tableName = (t == NULL) ? "deparser"_cs : t->name;
    BUG_CHECK(iDominator.count(gress) > 0, "Invalid gress %1% for table %2%", gress, tableName);
    const ImmediateDominatorMap *iDom = iDominator.at(gress);
    if (!iDom->count(t)) return std::nullopt;
    return iDom->at(t);
}

std::optional<const IR::MAU::Table *> BuildDominatorTree::getNonGatewayImmediateDominator(
    const IR::MAU::Table *t, gress_t gress) const {
    cstring tableName = (t == NULL) ? "deparser"_cs : t->name;
    BUG_CHECK(iDominator.count(gress) > 0, "Invalid gress %1% for table %2%", gress, tableName);
    auto dom = getImmediateDominator(t, gress);
    if (!dom) return std::nullopt;
    // If the table is not a gateway, then return the immediate dominator itself.
    if (!((*dom)->conditional_gateway_only())) return (*dom);
    // If the table is the same as its dominator then we are at the source node and if the source
    // node is a gateway, then return std::nullopt.
    if ((*dom)->conditional_gateway_only() && t == *dom) return std::nullopt;
    return getNonGatewayImmediateDominator(*dom, gress);
}

bool BuildDominatorTree::strictlyDominates(const IR::BFN::Unit *u1, const IR::BFN::Unit *u2) const {
    if (u1 == u2) return false;
    bool isParser1 = u1->is<IR::BFN::Parser>() || u1->is<IR::BFN::ParserState>();
    if (isParser1) return true;
    bool isParser2 = u2->is<IR::BFN::Parser>() || u2->is<IR::BFN::ParserState>();
    // Parser can never be dominated by a table or deparser.
    if (isParser2) return false;
    const auto *t1 = u1->to<IR::MAU::Table>();
    const auto *t2 = u2->to<IR::MAU::Table>();
    return strictlyDominates(t1, t2);
}

bool BuildDominatorTree::strictlyDominates(const IR::MAU::Table *t1,
                                           const IR::MAU::Table *t2) const {
    if (t1 == t2) return false;
    // nullptr is passed if the unit is the deparser, and deparser is always strictly dominated by
    // tables.
    if (t1 == nullptr) return false;
    const ImmediateDominatorMap *iDom = iDominator.at(t1->gress);
    if (t2 == nullptr) {
        // TODO: Remove this device-dependent dominator tree condition. This is needed currently to
        // get over fitting issues.
        if (Device::currentDevice() != Device::TOFINO) {
            // Figure out the dominator for the deparser.
            auto dom = getNonGatewayImmediateDominator(t2, t1->gress);
            if (!dom) return false;
            return strictlyDominates(t1, *dom);
        } else if (Device::currentDevice() == Device::TOFINO) {
            return true;
        }
    }
    if (t1->gress != t2->gress) return false;
    // For the source node, immediate dominator is the same as the node. Therefore, loop until we
    // reach the source node.
    while (t2 != iDom->at(t2)) {
        t2 = iDom->at(t2);
        if (t2 == t1) return true;
    }
    return false;
}

const std::vector<const IR::MAU::Table *> BuildDominatorTree::getAllDominators(
    const IR::MAU::Table *t, gress_t gress) const {
    std::vector<const IR::MAU::Table *> rv;
    const ImmediateDominatorMap *iDom = iDominator.at(gress);
    BUG_CHECK(iDom->count(t) > 0, "Table '%s' not found in ImmediateDominatorMap", t->name);
    while (t != iDom->at(t)) {
        t = iDom->at(t);
        rv.push_back(t);
    }
    return rv;
}

const IR::MAU::Table *BuildDominatorTree::getNonGatewayGroupDominator(
    ordered_set<const IR::MAU::Table *> &tables) const {
    // Validate that all tables are of the same gress.
    std::optional<gress_t> gress = std::nullopt;
    for (const auto *t : tables) {
        if (!gress) {
            gress = t->gress;
            continue;
        }

        if (*gress != t->gress)
            BUG("Call to getNonGatewayGroupDominator with tables of different gresses.");
    }

    // Find all the nodes from the given table to the source.
    ordered_map<const IR::MAU::Table *, std::vector<const IR::MAU::Table *>> pathsToSource;
    std::optional<unsigned> minDepth = std::nullopt;
    for (const auto *t : tables) {
        pathsToSource[t] = getAllDominators(t, gress.value());
        LOG3("\t\t\tTable " << t->name << " is at depth " << pathsToSource[t].size()
                            << " in the "
                               "dominator tree.");
        if (minDepth == std::nullopt || pathsToSource[t].size() < *minDepth) {
            minDepth = pathsToSource[t].size();
            LOG4("\t\t\t  Setting minDepth to " << *minDepth);
        }
    }
    if (minDepth) LOG3("\t\t  Min depth: " << minDepth.value());

    // Trim all the nodes that are greater in depth than the minimum depth. Note that the position
    // of table pointers in the vector are reversed; i.e. the 0th position in the vector is
    // occupied by the immediate dominator, or the node deepest in the path from the source to the
    // dominator node in question.
    for (const auto *t : tables) {
        LOG4("\t\t\tReducing the size paths for table " << t->name);
        unsigned tableDepth = pathsToSource[t].size();
        while (tableDepth > minDepth.value()) {
            std::stringstream ss;
            ss << "\t\t\t\tErasing " << pathsToSource[t].at(0)->name << "; new table depth: ";
            pathsToSource[t].erase(pathsToSource[t].begin());
            tableDepth = pathsToSource[t].size();
            ss << tableDepth;
            LOG4(ss.str());
        }
        if (minDepth)
            LOG4("\t\t\t  minDepth: " << minDepth.value()
                                      << ", new  depth: " << pathsToSource[t].size());
        BUG_CHECK(minDepth && pathsToSource[t].size() == *minDepth,
                  "Paths for table %1% (%2%)"
                  " not reduced"
                  " to the min depth %3%",
                  t->name, pathsToSource[t].size(), *minDepth);
    }

    // Starting from the minDepth level, keep going up one level at a time and see if we encounter
    // the same table. If we do encounter the same table, then that table is the group dominator.
    // Return the non gateway dominator for that group dominator.
    for (unsigned i = 0; minDepth && i < *minDepth; ++i) {
        std::optional<const IR::MAU::Table *> dom = std::nullopt;
        bool foundCommonAncestor = true;
        LOG4("\t\t\t  i = " << i);
        for (const auto *t : tables) {
            if (!dom) {
                dom = pathsToSource[t].at(i);
                LOG4("\t\t\t\tNew table encountered: " << (*dom)->name);
                continue;
            }
            if (dom && *dom != pathsToSource[t].at(i)) {
                LOG4("\t\t\t\t  Found a different table: " << pathsToSource[t].at(i)->name);
                foundCommonAncestor = false;
                break;
            }
        }
        if (foundCommonAncestor) {
            LOG3("\t\t\t  Found common ancestor: " << (*dom)->name);
            if ((*dom)->conditional_gateway_only()) {
                auto rv = getNonGatewayImmediateDominator(*dom, (*dom)->gress);
                if (!rv) return nullptr;
                return *rv;
            }
            return *dom;
        }
    }
    return nullptr;
}

cstring BuildDominatorTree::hasImmediateDominator(gress_t g, cstring t) const {
    BUG_CHECK(iDominator.count(g) > 0, "Invalid gress %1% for unit %2%", g, t);
    const ImmediateDominatorMap *iDom = iDominator.at(g);
    for (auto kv : *iDom) {
        // If it is a sink node, then check corresponding to the nullptr entry in the dominator map.
        if (kv.first == nullptr && t == "SINK") return kv.second->name;
        if (kv.first == nullptr) continue;
        if (kv.first->name != t) continue;
        return kv.second->name;
    }
    return cstring();
}

bool BuildDominatorTree::strictlyDominates(cstring t1, cstring t2, gress_t gress) const {
    const ImmediateDominatorMap *iDom = iDominator.at(gress);
    const IR::MAU::Table *tbl1 = nullptr;
    const IR::MAU::Table *tbl2 = nullptr;
    for (auto kv : *iDom) {
        if (kv.first == nullptr) continue;
        if (kv.first->name == t1) tbl1 = kv.first;
        if (kv.first->name == t2) tbl2 = kv.first;
    }
    return strictlyDominates(tbl1, tbl2);
}

bool BuildDominatorTree::isDominator(cstring t1, gress_t gress, cstring t2) const {
    const ImmediateDominatorMap *iDom = iDominator.at(gress);
    const IR::MAU::Table *tbl1 = nullptr;
    const IR::MAU::Table *tbl2 = nullptr;
    for (auto kv : *iDom) {
        if (kv.first == nullptr) continue;
        if (kv.first->name == t1) tbl1 = kv.first;
        if (kv.first->name == t2) tbl2 = kv.first;
    }
    if (!tbl1) return false;
    if (!tbl2) return false;
    const auto dominators = getAllDominators(tbl1, gress);
    return (std::find(dominators.begin(), dominators.end(), tbl2) != dominators.end());
}
